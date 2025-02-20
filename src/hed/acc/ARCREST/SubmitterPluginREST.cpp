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

  bool SubmitterPluginREST::GetDelegationToken(const UserConfig& usercfg, Arc::URL url, std::string& delegationId) {
    Arc::MCCConfig cfg;
    usercfg.ApplyToConfig(cfg);
    Arc::ClientHTTP client(cfg, url);
    std::string delegationPath;
    if(delegationId.empty()) {
      url.AddHTTPOption("action","new");
      url.AddHTTPOption("type","jwt");
      std::multimap<std::string, std::string> attrs;
      attrs.insert(std::make_pair("x-token-delegation",usercfg.OToken()));
      Arc::PayloadRaw request;
      Arc::PayloadRawInterface* response(NULL);
      Arc::HTTPClientInfo info;
      Arc::MCC_Status res = client.process(std::string("POST"), url.FullPath(), attrs, &request, &info, &response);
      if(!res) {
        logger.msg(VERBOSE, "Failed to communicate to delegation endpoint.");
        delete response;
        return false;
      }
      if(info.code != 201) {
        logger.msg(VERBOSE, "Unexpected response code from delegation endpoint - %u",info.code);
        if(response)
          logger.msg(DEBUG, "Response: %s", std::string(response->Buffer(0),response->BufferSize(0)));
        delete response;
        return false;
      }
      delete response;
      delegationPath = info.location.Path();
      std::string::size_type id_pos = delegationPath.rfind('/');
      if(id_pos == std::string::npos) {
        logger.msg(INFO, "Unexpected delegation location from delegation endpoint - %s.",info.location.fullstr());
        return false;
      }
      delegationId = delegationPath.substr(id_pos+1);
    } else {
      url.ChangePath(url.Path() + "/" + delegationId);
      url.AddHTTPOption("action","renew");
      url.AddHTTPOption("type","jwt");
      std::multimap<std::string, std::string> attrs;
      attrs.insert(std::make_pair("x-token-delegation",usercfg.OToken()));
      delegationPath = url.Path();
      Arc::PayloadRaw request;
      Arc::PayloadRawInterface* response(NULL);
      Arc::HTTPClientInfo info;
      Arc::MCC_Status res = client.process(std::string("POST"), url.FullPath(), attrs, &request, &info, &response);
      if(!res) {
        logger.msg(VERBOSE, "Failed to communicate to delegation endpoint.");
        delete response;
        return false;
      }
      if(info.code != 201) {
        logger.msg(VERBOSE, "Unexpected response code from delegation endpoint - %u",info.code);
        if(response)
          logger.msg(DEBUG, "Response: %s", std::string(response->Buffer(0),response->BufferSize(0)));
        delete response;
        return false;
      }
      if(!response) {
        logger.msg(VERBOSE, "Missing response from delegation endpoint.");
        delete response;
        return false;
      }
      delete response;
    }
    return true;
  }

  bool SubmitterPluginREST::GetDelegationX509(const UserConfig& usercfg, Arc::URL url, std::string& delegationId) {
    std::string delegationRequest;
    Arc::MCCConfig cfg;
    usercfg.ApplyToConfig(cfg);
    Arc::ClientHTTP client(cfg, url);
    std::string delegationPath;
    if(delegationId.empty()) {
      url.AddHTTPOption("action","new");
      // url.AddHTTPOption("type","x509"); - default option in 1.1 and not supported in 1.0
      Arc::PayloadRaw request;
      Arc::PayloadRawInterface* response(NULL);
      Arc::HTTPClientInfo info;
      Arc::MCC_Status res = client.process(std::string("POST"), url.FullPath(), &request, &info, &response);
      if(!res) {
        logger.msg(VERBOSE, "Failed to communicate to delegation endpoint.");
        delete response;
        return false;
      }
      if(info.code != 201) {
        logger.msg(VERBOSE, "Unexpected response code from delegation endpoint - %u",info.code);
        if(response)
          logger.msg(DEBUG, "Response: %s", std::string(response->Buffer(0),response->BufferSize(0)));
        delete response;
        return false;
      }
      if(!response) {
        logger.msg(VERBOSE, "Missing response from delegation endpoint.");
        delete response;
        return false;
      }
      for(unsigned int n = 0;response->Buffer(n);++n) delegationRequest.append(response->Buffer(n),response->BufferSize(n));
      delete response;
      delegationPath = info.location.Path();
      std::string::size_type id_pos = delegationPath.rfind('/');
      if(id_pos == std::string::npos) {
        logger.msg(INFO, "Unexpected delegation location from delegation endpoint - %s.",info.location.fullstr());
        return false;
      }
      delegationId = delegationPath.substr(id_pos+1);
    } else {
      url.ChangePath(url.Path() + "/" + delegationId);
      url.AddHTTPOption("action","renew");
      delegationPath = url.Path();
      Arc::PayloadRaw request;
      Arc::PayloadRawInterface* response(NULL);
      Arc::HTTPClientInfo info;
      Arc::MCC_Status res = client.process(std::string("POST"), url.FullPath(), &request, &info, &response);
      if(!res) {
        logger.msg(VERBOSE, "Failed to communicate to delegation endpoint.");
        delete response;
        return false;
      }
      if(info.code != 201) {
        logger.msg(VERBOSE, "Unexpected response code from delegation endpoint - %u",info.code);
        if(response)
          logger.msg(DEBUG, "Response: %s", std::string(response->Buffer(0),response->BufferSize(0)));
        delete response;
        return false;
      }
      if(!response) {
        logger.msg(VERBOSE, "Missing response from delegation endpoint.");
        delete response;
        return false;
      }
      for(unsigned int n = 0;response->Buffer(n);++n) delegationRequest.append(response->Buffer(n),response->BufferSize(n));
      delete response;
    }

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
    delete deleg; deleg = NULL;

    Arc::PayloadRaw request;
    request.Insert(delegationResponse.c_str(),0,delegationResponse.length());
    Arc::PayloadRawInterface* response(NULL);
    Arc::HTTPClientInfo info;
    Arc::MCC_Status res = client.process(std::string("PUT"), delegationPath, &request, &info, &response);
    delete response; response = NULL;
    if(!res) {
      logger.msg(INFO, "Failed to communicate to delegation endpoint.");
      return false;
    }
    if (info.code != 200) {
      logger.msg(INFO, "Unexpected response code from delegation endpoint: %u, %s.", info.code, info.reason);
      return false;
    }
    return true;
  }

  bool SubmitterPluginREST::AddDelegation(std::string& product, std::string const& delegationId) {
    // Add delegation directly into JobDescription
    Arc::XMLNode job(product);
    if(!job) return false;
    NS ns;
    ns["adl"] = "http://www.eu-emi.eu/es/2010/12/adl";
    ns["nordugrid-adl"] = "http://www.nordugrid.org/es/2011/12/nordugrid-adl";
    job.Namespaces(ns);
    // Inserting delegation id into job desription - ADL specific
    XMLNodeList sources = job.Path("DataStaging/InputFile/Source");
    for(XMLNodeList::iterator item = sources.begin();item!=sources.end();++item) {
      item->NewChild("adl:DelegationID") = delegationId;
    };
    XMLNodeList targets = job.Path("DataStaging/OutputFile/Target");
    for(XMLNodeList::iterator item = targets.begin();item!=targets.end();++item) {
      item->NewChild("adl:DelegationID") = delegationId;
    };
    job["DataStaging"].NewChild("nordugrid-adl:DelegationID") = delegationId;
    job.GetXML(product);
    return true;
  }

  static bool urlisinsecure(Arc::URL const & url) {
    std::string protocol = url.Protocol();
    return protocol.empty() || (protocol == "http") || (protocol == "ftp") || (protocol == "ldap");
  }

  SubmissionStatus SubmitterPluginREST::PostProcessInternal(Arc::URL const& baseUrl, Arc::URL const& submissionUrl, std::string const& delegationId,
                                                            JobDescription const& jobdesc, Arc::XMLNode job_item, EntityConsumer<Job>& jc) {
    SubmissionStatus retval;

    std::string code = job_item["status-code"];
    std::string reason = job_item["reason"];
    std::string id = job_item["id"];
    std::string state = job_item["state"];
    if((code != "201") || id.empty()) {
      logger.msg(INFO, "Failed to submit all jobs: %s %s", code, reason);
      retval |= SubmissionStatus::DESCRIPTION_NOT_SUBMITTED;
      retval |= SubmissionStatus::ERROR_FROM_ENDPOINT;
      return retval;
    }
    URL jobid(submissionUrl);
    jobid.RemoveHTTPOption("action");
    jobid.ChangePath(jobid.Path()+"/"+id);
    URL sessionUrl = jobid;
    sessionUrl.ChangePath(sessionUrl.Path()+"/session");
    // compensate for time between request and response on slow networks
    sessionUrl.AddOption("encryption=optional",false);
    // TODO: implement multi job PutFiles or run multiple in parallel
    if (!PutFiles(jobdesc, sessionUrl)) {
      logger.msg(INFO, "Failed uploading local input files");
      retval |= SubmissionStatus::DESCRIPTION_NOT_SUBMITTED;
      retval |= SubmissionStatus::ERROR_FROM_ENDPOINT;
      // TODO: send job cancel request to let server know files are not coming
      return retval;
    }

    Job j;
    AddJobDetails(jobdesc, j);
    // Proposed mandatory attributes for ARC 3.0
    j.JobID = jobid.fullstr();
    j.ServiceInformationURL = baseUrl;
    j.ServiceInformationInterfaceName = "org.nordugrid.arcrest";
    j.JobStatusURL = baseUrl;
    j.JobStatusInterfaceName = "org.nordugrid.arcrest";
    j.JobManagementURL = baseUrl;
    j.JobManagementInterfaceName = "org.nordugrid.arcrest";
    j.IDFromEndpoint = id;
    j.DelegationID.push_back(delegationId);
    j.LogDir = "/diagnose";
      
    jc.addEntity(j);

    return retval;
  }

  SubmissionStatus SubmitterPluginREST::SubmitInternal(const std::list<JobDescription>& jobdescs,
                                           const ExecutionTarget* et, const std::string& endpoint,
                         EntityConsumer<Job>& jc, std::list<const JobDescription*>& notSubmitted) {
    // TODO: autoversion
    URL url(et ?
            URL(et->ComputingEndpoint->URLString) :
            URL((endpoint.find("://") == std::string::npos ? "https://" : "") + endpoint, false, 443, "/arex"));

    Arc::URL delegationX509Url(url);
    Arc::URL delegationTokenUrl(url);
    delegationX509Url.ChangePath(delegationX509Url.Path()+"/rest/1.0/delegations");
    delegationX509Url.AddHTTPOption("action","new");
    delegationTokenUrl.ChangePath(delegationTokenUrl.Path()+"/rest/1.1/delegations"); // Token delegation appears in 1.1 version
    delegationTokenUrl.AddHTTPOption("action","new");

    SubmissionStatus retval;
    std::string delegationId;
    if(jobdescs.empty()) 
      return retval;

    int instances_min = 1;
    int instances_max = 1;
    std::string fullProduct;
    if(jobdescs.size() > 1) fullProduct = "<ActivityDescriptions>";
    std::list< std::pair<JobDescription,std::list<JobDescription>::const_iterator> > preparedjobdescs;
    for (std::list<JobDescription>::const_iterator it = jobdescs.begin(); it != jobdescs.end(); ++it) {
      JobDescription preparedjobdesc(*it);
  
      if (!(et?preparedjobdesc.Prepare(*et):preparedjobdesc.Prepare())) {
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

      if((jobdescs.size() > 1) && ((preparedjobdesc.InstancesMin > 1) || (preparedjobdesc.InstancesMax > 1))) {
        logger.msg(INFO, "Can't submit multiple instances for multiple job descriptions. Not implemented yet.");
        notSubmitted.push_back(&*it);
        retval |= SubmissionStatus::DESCRIPTION_NOT_SUBMITTED;
        continue;
      }
      if(preparedjobdesc.InstancesMin > 1) instances_min = preparedjobdesc.InstancesMin;
      if(preparedjobdesc.InstancesMax > 1) instances_max = preparedjobdesc.InstancesMax;

      if(delegationId.empty()) {
        if(preparedjobdesc.X509Delegation) {
          if(!GetDelegationX509(*usercfg, delegationX509Url, delegationId)) {
            logger.msg(INFO, "Unable to submit jobs. Failed to delegate X.509 credentials.");
            notSubmitted.push_back(&*it);
            retval |= SubmissionStatus::DESCRIPTION_NOT_SUBMITTED;
            continue;
          }
	} else if(preparedjobdesc.TokenDelegation) {
          if(!GetDelegationToken(*usercfg, delegationTokenUrl, delegationId)) {
            logger.msg(INFO, "Unable to submit jobs. Failed to delegate token.");
            notSubmitted.push_back(&*it);
            retval |= SubmissionStatus::DESCRIPTION_NOT_SUBMITTED;
            continue;
          }
        }
      }

      if(!delegationId.empty()) {
        if(!AddDelegation(product, delegationId)) {
          logger.msg(INFO, "Unable to submit job. Failed to assign delegation to job description.");
          notSubmitted.push_back(&*it);
          retval |= SubmissionStatus::DESCRIPTION_NOT_SUBMITTED;
          continue;
        };
      };

      fullProduct += product;
      preparedjobdescs.push_back(std::make_pair(preparedjobdesc,it));
    };
    if(jobdescs.size() > 1) fullProduct = "</ActivityDescriptions>";
    if (preparedjobdescs.empty()) {
      return retval;
    }

    Arc::URL submissionUrl(url);
    submissionUrl.AddHTTPOption("action","new");
    if(instances_max > 1) {
      submissionUrl.ChangePath(submissionUrl.Path()+"/rest/1.1/jobs");
      if(instances_min > 1) submissionUrl.AddHTTPOption("instances_min",Arc::tostring(instances_min));
      submissionUrl.AddHTTPOption("instances",Arc::tostring(instances_max));
    } else {
      submissionUrl.ChangePath(submissionUrl.Path()+"/rest/1.0/jobs");
    }

    Arc::MCCConfig cfg;
    usercfg->ApplyToConfig(cfg);
    Arc::ClientHTTP client(cfg, submissionUrl);
    Arc::PayloadRaw request;
    request.Insert(fullProduct.c_str(),0,fullProduct.length());
    Arc::PayloadRawInterface* response(NULL);
    Arc::HTTPClientInfo info;
    std::multimap<std::string,std::string> attributes;
    attributes.insert(std::pair<std::string, std::string>("Accept", "text/xml"));
    submissionUrl.RemoveHTTPOption("instances_min");
    submissionUrl.RemoveHTTPOption("instances");

    // TODO: paging, size limit
    Arc::MCC_Status res = client.process(std::string("POST"), attributes, &request, &info, &response);
    if(!res || !response) {
      logger.msg(INFO, "Failed to submit all jobs.");
      for (std::list<JobDescription>::const_iterator it = jobdescs.begin(); it != jobdescs.end(); ++it) {
        notSubmitted.push_back(&*it);
      }
      retval |= SubmissionStatus::DESCRIPTION_NOT_SUBMITTED;
      retval |= SubmissionStatus::ERROR_FROM_ENDPOINT;
      delete response;
      return retval;
    }
    if(info.code != 201) {
      logger.msg(INFO, "Failed to submit all jobs: %u %s", info.code, info.reason);
      logger.msg(DEBUG, "Response: %s", std::string(response->Buffer(0),response->BufferSize(0)));
      for (std::list<JobDescription>::const_iterator it = jobdescs.begin(); it != jobdescs.end(); ++it) {
        notSubmitted.push_back(&*it);
      }
      retval |= SubmissionStatus::DESCRIPTION_NOT_SUBMITTED;
      retval |= SubmissionStatus::ERROR_FROM_ENDPOINT;
      delete response;
      return retval;
    }
    Arc::XMLNode jobs_list(response->Content());
    delete response; response = NULL;
    if(!jobs_list || (jobs_list.Name() != "jobs")) {
      logger.msg(INFO, "Failed to submit all jobs: %s", info.reason);
      for (std::list<JobDescription>::const_iterator it = jobdescs.begin(); it != jobdescs.end(); ++it) {
        notSubmitted.push_back(&*it);
      }
      retval |= SubmissionStatus::DESCRIPTION_NOT_SUBMITTED;
      retval |= SubmissionStatus::ERROR_FROM_ENDPOINT;
      return retval;
    }
    Arc::XMLNode job_item = jobs_list["job"];
    if(jobdescs.size() > 1) {
      for (std::list< std::pair<JobDescription,std::list<JobDescription>::const_iterator> >::const_iterator it = preparedjobdescs.begin(); it != preparedjobdescs.end(); ++it) {
        if(!job_item) { // no more jobs returned 
          retval |= SubmissionStatus::DESCRIPTION_NOT_SUBMITTED;
          for (; it != preparedjobdescs.end(); ++it) notSubmitted.push_back(&(*(it->second)));
          break;
        }
        SubmissionStatus retval2 = PostProcessInternal(url, submissionUrl, delegationId, it->first, job_item, jc);
        ++job_item;
        if(!retval2) {
          notSubmitted.push_back(&(*(it->second)));
          retval |= retval2;
          continue;
        }
      }
    } else {
      std::list< std::pair<JobDescription,std::list<JobDescription>::const_iterator> >::const_iterator it = preparedjobdescs.begin();
      if(!job_item) { // no jobs returned 
        retval |= SubmissionStatus::DESCRIPTION_NOT_SUBMITTED;
        notSubmitted.push_back(&(*(it->second)));
      } else {
        bool is_submitted = false;
        while(job_item) {
          SubmissionStatus retval2 = PostProcessInternal(url, submissionUrl, delegationId, it->first, job_item, jc);
          ++job_item;
          if(!retval2) {
            retval |= retval2;
          } else {
            is_submitted = true;
          }
        }
        if(!is_submitted) notSubmitted.push_back(&(*(it->second)));
      }
    }
  
    return retval;
  }

  SubmissionStatus SubmitterPluginREST::Submit(const std::list<JobDescription>& jobdescs, const std::string& endpoint, EntityConsumer<Job>& jc, std::list<const JobDescription*>& notSubmitted) {
    return SubmitInternal(jobdescs, NULL, endpoint, jc, notSubmitted);
  }

  SubmissionStatus SubmitterPluginREST::Submit(const std::list<JobDescription>& jobdescs, const ExecutionTarget& et, EntityConsumer<Job>& jc, std::list<const JobDescription*>& notSubmitted) {
    return SubmitInternal(jobdescs, &et, "", jc, notSubmitted);
  }

} // namespace Arc
