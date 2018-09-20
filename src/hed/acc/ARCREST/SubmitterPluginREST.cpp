// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <sstream>

#include <sys/stat.h>

#include <glibmm.h>

#include <arc/XMLNode.h>
#include <arc/CheckSum.h>
#include <arc/StringConv.h>
#include <arc/UserConfig.h>
#include <arc/compute/ExecutionTarget.h>
#include <arc/compute/Job.h>
#include <arc/compute/JobDescription.h>
#include <arc/compute/SubmissionStatus.h>
#include <arc/message/MCC.h>
#include <arc/delegation/DelegationInterface.h>

#include "SubmitterPluginREST.h"
//#include "AREXClient.h"

namespace Arc {

  Logger SubmitterPluginREST::logger(Logger::getRootLogger(), "SubmitterPlugin.REST");

  bool SubmitterPluginREST::isEndpointNotSupported(const std::string& endpoint) const {
    const std::string::size_type pos = endpoint.find("://");
    return pos != std::string::npos && lower(endpoint.substr(0, pos)) != "http" && lower(endpoint.substr(0, pos)) != "https";
  }

  bool SubmitterPluginREST::GetDelegation(Arc::URL url, std::string& delegationId) const {
    std::string delegationRequest;
    Arc::MCCConfig cfg;
    usercfg->ApplyToConfig(cfg);
    std::string delegationPath = url.Path();
    if(!delegationId.empty()) delegationPath = delegationPath+"/"+delegationId;
    Arc::ClientHTTP client(cfg, url);
    {
      Arc::PayloadRaw request;
      Arc::PayloadRawInterface* response(NULL);
      Arc::HTTPClientInfo info;
      Arc::MCC_Status res = client.process(std::string("GET"), delegationPath, &request, &info, &response);
      if((!res) || (info.code != 200) || (info.reason.empty()) || (!response)) {
        delete response;
        return false;
      }
      delegationId = info.reason;
      for(unsigned int n = 0;response->Buffer(n);++n) {
        delegationRequest.append(response->Buffer(n),response->BufferSize(n));
      }
      delete response;
    }
    {
      DelegationProvider* deleg(NULL);
      if (!cfg.credential.empty()) {
        deleg = new DelegationProvider(cfg.credential);
      }
      else {
        const std::string& cert = (!cfg.proxy.empty() ? cfg.proxy : cfg.cert);
        const std::string& key  = (!cfg.proxy.empty() ? cfg.proxy : cfg.key);
        if (key.empty() || cert.empty()) return false;
        deleg = new DelegationProvider(cert, key);
      }
      std::string delegationResponse = deleg->Delegate(delegationRequest);
      delete deleg;

      Arc::PayloadRaw request;
      request.Insert(delegationResponse.c_str(),0,delegationResponse.length());
      Arc::PayloadRawInterface* response(NULL);
      Arc::HTTPClientInfo info;
      Arc::MCC_Status res = client.process(std::string("PUT"), url.Path()+"/"+delegationId, &request, &info, &response);
      delete response;
      if((!res) || (info.code != 200) || (!response)) return false;
    }
    return true;
  }

  bool SubmitterPluginREST::AddDelegation(std::string& product, std::string const& delegationId) {
    Arc::XMLNode job(product);
    if(!job) return false;
    NS ns;
    ns["adl"] = "http://www.eu-emi.eu/es/2010/12/adl";
    ns["nordugrid-adl"] = "http://www.nordugrid.org/es/2011/12/nordugrid-adl";
    job.Namespaces(ns);
    // Inserting delegation id into job desription - ADL specific
    XMLNodeList sources = job.Path("DataStaging/InputFile/Source");
    for(XMLNodeList::iterator item = sources.begin();item!=sources.end();++item) {
      item->NewChild("esadl:DelegationID") = delegationId;
    };
    XMLNodeList targets = job.Path("DataStaging/OutputFile/Target");
    for(XMLNodeList::iterator item = targets.begin();item!=targets.end();++item) {
      item->NewChild("esadl:DelegationID") = delegationId;
    };
    job["DataStaging"].NewChild("nordugrid-adl:DelegationID") = delegationId;
    job.GetXML(product);
    return true;
  }

  SubmissionStatus SubmitterPluginREST::Submit(const std::list<JobDescription>& jobdescs, const std::string& endpoint, EntityConsumer<Job>& jc, std::list<const JobDescription*>& notSubmitted) {
    URL url((endpoint.find("://") == std::string::npos ? "https://" : "") + endpoint, false, 443, "/arex");

    Arc::URL submissionUrl(url);
    Arc::URL delegationUrl(url);
    submissionUrl.ChangePath(submissionUrl.Path()+"/*new");
    delegationUrl.ChangePath(delegationUrl.Path()+"/*deleg");

    SubmissionStatus retval;
    for (std::list<JobDescription>::const_iterator it = jobdescs.begin(); it != jobdescs.end(); ++it) {
      JobDescription preparedjobdesc(*it);
  
      if (!preparedjobdesc.Prepare()) {
        logger.msg(INFO, "Failed to prepare job description");
        notSubmitted.push_back(&*it);
        retval |= SubmissionStatus::DESCRIPTION_NOT_SUBMITTED;
        continue;
      }

      std::string product;
      JobDescriptionResult ures = preparedjobdesc.UnParse(product, "emies:adl");
      if (!ures) {
        logger.msg(INFO, "Unable to submit job. Job description is not valid in the %s format: %s", "emies:adl", ures.str());
        notSubmitted.push_back(&*it);
        retval |= SubmissionStatus::DESCRIPTION_NOT_SUBMITTED;
        continue;
      }

      std::string delegationId;
      if(!GetDelegation(delegationUrl, delegationId)) {
        logger.msg(INFO, "Unable to submit job. Failed to delegate credentials.");
        notSubmitted.push_back(&*it);
        retval |= SubmissionStatus::DESCRIPTION_NOT_SUBMITTED;
        continue;
      };
      if(!AddDelegation(product, delegationId)) {
        logger.msg(INFO, "Unable to submit job. Failed to assign delegation to job description.");
        notSubmitted.push_back(&*it);
        retval |= SubmissionStatus::DESCRIPTION_NOT_SUBMITTED;
        continue;
      };

      Arc::MCCConfig cfg;
      usercfg->ApplyToConfig(cfg);
      Arc::ClientHTTP client(cfg, submissionUrl);
      Arc::PayloadRaw request;
      request.Insert(product.c_str(),0,product.length());
      Arc::PayloadRawInterface* response(NULL);
      Arc::HTTPClientInfo info;
      Arc::MCC_Status res = client.process(std::string("PUT"), &request, &info, &response);
      if(!res) {
        notSubmitted.push_back(&*it);
        retval |= SubmissionStatus::DESCRIPTION_NOT_SUBMITTED;
        retval |= SubmissionStatus::ERROR_FROM_ENDPOINT;
        continue;
      }
      if((info.code != 200) || (info.reason.empty())) {
        retval |= SubmissionStatus::DESCRIPTION_NOT_SUBMITTED;
        retval |= SubmissionStatus::ERROR_FROM_ENDPOINT;
        continue;
      }
      URL jobid(url);
      jobid.ChangePath(jobid.Path()+"/"+info.reason);
      URL sessionurl = jobid;

      // compensate for time between request and response on slow networks
      sessionurl.AddOption("encryption=optional",false);
      if (!PutFiles(preparedjobdesc, sessionurl)) {
        logger.msg(INFO, "Failed uploading local input files");
        notSubmitted.push_back(&*it);
        retval |= SubmissionStatus::DESCRIPTION_NOT_SUBMITTED;
        retval |= SubmissionStatus::ERROR_FROM_ENDPOINT;
        continue;
      }
    
      Job j;
      
      AddJobDetails(preparedjobdesc, j);
      
      // Proposed mandatory attributes for ARC 3.0
      j.JobID = jobid.fullstr();
      j.ServiceInformationURL = url;
      j.ServiceInformationInterfaceName = "org.nordugrid.arcrest";
      j.JobStatusURL = url;
      j.JobStatusInterfaceName = "org.nordugrid.arcrest";
      j.JobManagementURL = url;
      j.JobManagementInterfaceName = "org.nordugrid.arcrest";
      j.IDFromEndpoint = info.reason;
      j.DelegationID.push_back(delegationId);
      
      jc.addEntity(j);
    }
  
    return retval;
  }

  SubmissionStatus SubmitterPluginREST::Submit(const std::list<JobDescription>& jobdescs, const ExecutionTarget& et, EntityConsumer<Job>& jc, std::list<const JobDescription*>& notSubmitted) {
    URL url(et.ComputingEndpoint->URLString);

    Arc::URL submissionUrl(url);
    Arc::URL delegationUrl(url);
    submissionUrl.ChangePath(submissionUrl.Path()+"/*new");
    delegationUrl.ChangePath(delegationUrl.Path()+"/*deleg");

    SubmissionStatus retval;
    for (std::list<JobDescription>::const_iterator it = jobdescs.begin(); it != jobdescs.end(); ++it) {
      JobDescription preparedjobdesc(*it);
  
      if (!preparedjobdesc.Prepare(et)) {
        logger.msg(INFO, "Failed to prepare job description");
        notSubmitted.push_back(&*it);
        retval |= SubmissionStatus::DESCRIPTION_NOT_SUBMITTED;
        continue;
      }

      // !! TODO: For regular BES ordinary JSDL is needed - keeping nordugrid:jsdl so far
      std::string product;
      JobDescriptionResult ures = preparedjobdesc.UnParse(product, "emies:adl");
      if (!ures) {
        logger.msg(INFO, "Unable to submit job. Job description is not valid in the %s format: %s", "emies:adl", ures.str());
        notSubmitted.push_back(&*it);
        retval |= SubmissionStatus::DESCRIPTION_NOT_SUBMITTED;
        continue;
      }

      std::string delegationId;
      if(!GetDelegation(delegationUrl, delegationId)) {
        logger.msg(INFO, "Unable to submit job. Failed to delegate credentials.");
        notSubmitted.push_back(&*it);
        retval |= SubmissionStatus::DESCRIPTION_NOT_SUBMITTED;
        continue;
      };
      if(!AddDelegation(product, delegationId)) {
        logger.msg(INFO, "Unable to submit job. Failed to assign delegation to job description.");
        notSubmitted.push_back(&*it);
        retval |= SubmissionStatus::DESCRIPTION_NOT_SUBMITTED;
        continue;
      };

      Arc::MCCConfig cfg;
      usercfg->ApplyToConfig(cfg);
      Arc::ClientHTTP client(cfg, submissionUrl);
      Arc::PayloadRaw request;
      request.Insert(product.c_str(),0,product.length());
      Arc::PayloadRawInterface* response(NULL);
      Arc::HTTPClientInfo info;
      Arc::MCC_Status res = client.process(std::string("PUT"), &request, &info, &response);
      if(!res) {
        notSubmitted.push_back(&*it);
        retval |= SubmissionStatus::DESCRIPTION_NOT_SUBMITTED;
        retval |= SubmissionStatus::ERROR_FROM_ENDPOINT;
        continue;
      }
      if((info.code != 200) || (info.reason.empty())) {
        retval |= SubmissionStatus::DESCRIPTION_NOT_SUBMITTED;
        retval |= SubmissionStatus::ERROR_FROM_ENDPOINT;
        continue;
      }
      URL jobid(url);
      jobid.ChangePath(jobid.Path()+"/"+info.reason);
      URL sessionurl = jobid;

      // compensate for time between request and response on slow networks
      sessionurl.AddOption("encryption=optional",false);
      if (!PutFiles(preparedjobdesc, sessionurl)) {
        logger.msg(INFO, "Failed uploading local input files");
        notSubmitted.push_back(&*it);
        retval |= SubmissionStatus::DESCRIPTION_NOT_SUBMITTED;
        retval |= SubmissionStatus::ERROR_FROM_ENDPOINT;
        continue;
      }
    
      Job j;
      
      AddJobDetails(preparedjobdesc, j);
      
      // Proposed mandatory attributes for ARC 3.0
      j.JobID = jobid.fullstr();
      j.ServiceInformationURL = url;
      j.ServiceInformationInterfaceName = "org.nordugrid.arcrest";
      j.JobStatusURL = url;
      j.JobStatusInterfaceName = "org.nordugrid.arcrest";
      j.JobManagementURL = url;
      j.JobManagementInterfaceName = "org.nordugrid.arcrest";
      j.IDFromEndpoint = info.reason;
      j.DelegationID.push_back(delegationId);
      
      jc.addEntity(j);
    }
  
    return retval;
  }

  bool SubmitterPluginREST::Migrate(const std::string& jobid, const JobDescription& jobdesc,
                             const ExecutionTarget& et,
                             bool forcemigration, Job& job) {
    /*
    URL url(et.ComputingEndpoint->URLString);

    AutoPointer<AREXClient> ac(clients.acquire(url,true));

    std::string idstr;
    AREXClient::createActivityIdentifier(jobid, idstr);

    JobDescription preparedjobdesc(jobdesc);

    // Modify the location of local files and files residing in a old session directory.
    for (std::list<InputFileType>::iterator it = preparedjobdesc.DataStaging.InputFiles.begin();
         it != preparedjobdesc.DataStaging.InputFiles.end(); it++) {
      if (!it->Sources.front() || it->Sources.front().Protocol() == "file") {
        it->Sources.front() = URL(jobid + "/" + it->Name);
      }
      else {
        // URL is valid, and not a local file. Check if the source reside at a
        // old job session directory.
        const size_t foundRSlash = it->Sources.front().str().rfind('/');
        if (foundRSlash == std::string::npos) continue;

        const std::string uriPath = it->Sources.front().str().substr(0, foundRSlash);
        // Check if the input file URI is pointing to a old job session directory.
        for (std::list<std::string>::const_iterator itAOID = preparedjobdesc.Identification.ActivityOldID.begin();
             itAOID != preparedjobdesc.Identification.ActivityOldID.end(); itAOID++) {
          if (uriPath == *itAOID) {
            it->Sources.front() = URL(jobid + "/" + it->Name);
            break;
          }
        }
      }
    }

    if (!preparedjobdesc.Prepare(et)) {
      logger.msg(INFO, "Failed adapting job description to target resources");
      clients.release(ac.Release());
      return false;
    }

    // Add ActivityOldID.
    preparedjobdesc.Identification.ActivityOldID.push_back(jobid);

    std::string product;
    JobDescriptionResult ures = preparedjobdesc.UnParse(product, "nordugrid:jsdl");
    if (!ures) {
      logger.msg(INFO, "Unable to migrate job. Job description is not valid in the %s format: %s", "nordugrid:jsdl", ures.str());
      clients.release(ac.Release());
      return false;
    }

    std::string sNewjobid;
    if (!ac->migrate(idstr, product, forcemigration, sNewjobid,
                    url.Protocol() == "https")) {
      clients.release(ac.Release());
      return false;
    }

    if (sNewjobid.empty()) {
      logger.msg(INFO, "No job identifier returned by A-REX");
      clients.release(ac.Release());
      return false;
    }

    XMLNode xNewjobid(sNewjobid);
    URL newjobid((std::string)(xNewjobid["ReferenceParameters"]["JobSessionDir"]));

    URL sessionurl = newjobid;
    sessionurl.AddOption("threads=3",false);
    sessionurl.AddOption("encryption=optional",false);
    sessionurl.AddOption("httpputpartial=yes",false); // for A-REX
    sessionurl.AddOption("blocksize=5242880",true);

    if (!PutFiles(preparedjobdesc, sessionurl)) {
      logger.msg(INFO, "Failed uploading local input files");
      clients.release(ac.Release());
      return false;
    }

    AddJobDetails(preparedjobdesc, job);
    
    // Proposed mandatory attributes for ARC 3.0
    job.JobID = newjobid;
    job.ServiceInformationURL = url;
    job.ServiceInformationInterfaceName = "org.nordugrid.wsrfglue2";
    job.JobStatusURL = url;
    job.JobStatusInterfaceName = "org.nordugrid.xbes";
    job.JobManagementURL = url;
    job.JobManagementInterfaceName = "org.nordugrid.xbes";
    job.IDFromEndpoint = (std::string)xNewjobid["ReferenceParameters"]["a-rex:JobID"];
    job.StageInDir = sessionurl;
    job.StageOutDir = sessionurl;
    job.SessionDir = sessionurl;

    clients.release(ac.Release());
    return true;
    */
    return false;
  }
} // namespace Arc
