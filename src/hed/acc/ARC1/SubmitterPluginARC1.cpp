// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <sstream>

#include <sys/stat.h>

#include <glibmm.h>

#include <arc/CheckSum.h>
#include <arc/StringConv.h>
#include <arc/UserConfig.h>
#include <arc/client/ExecutionTarget.h>
#include <arc/client/Job.h>
#include <arc/client/JobDescription.h>
#include <arc/message/MCC.h>

#include "SubmitterPluginARC1.h"
#include "AREXClient.h"

namespace Arc {

  Logger SubmitterPluginARC1::logger(Logger::getRootLogger(), "SubmitterPlugin.ARC1");

  bool SubmitterPluginARC1::isEndpointNotSupported(const std::string& endpoint) const {
    const std::string::size_type pos = endpoint.find("://");
    return pos != std::string::npos && lower(endpoint.substr(0, pos)) != "http" && lower(endpoint.substr(0, pos)) != "https";
  }

  AREXClient* SubmitterPluginARC1::acquireClient(const URL& url, bool arex_features) {
    std::map<URL, AREXClient*>::const_iterator url_it = clients.find(url);
    if ( url_it != clients.end() ) {
      // If AREXClient is already existing for the
      // given URL then return with that
      return url_it->second;
    } else {
      // Else create a new one and return with that
      MCCConfig cfg;
      usercfg.ApplyToConfig(cfg);
      AREXClient* ac = new AREXClient(url, cfg, usercfg.Timeout(), arex_features);
      return clients[url] = ac;
    }
  }

  bool SubmitterPluginARC1::releaseClient(const URL& url) {
    return true;
  }

  bool SubmitterPluginARC1::deleteAllClients() {
    std::map<URL, AREXClient*>::iterator it;
    for (it = clients.begin(); it != clients.end(); it++) {
        if ((*it).second != NULL) delete (*it).second;
    }
    return true;
  }

  bool SubmitterPluginARC1::Submit(const std::list<JobDescription>& jobdescs, const ExecutionTarget& et, std::list<Job>& jobs, std::list<const JobDescription*>& notSubmitted) {
    URL url(et.ComputingEndpoint->URLString);
    bool arex_features = et.ComputingService->Type == "org.nordugrid.execution.arex";
    AREXClient* ac = acquireClient(url, arex_features);

    bool ok = true;
    for (std::list<JobDescription>::const_iterator it = jobdescs.begin(); it != jobdescs.end(); ++it) {
      JobDescription preparedjobdesc(*it);
  
      if (arex_features && !preparedjobdesc.Prepare(et)) {
        logger.msg(INFO, "Failed to prepare job description to target resources");
        notSubmitted.push_back(&*it);
        ok = false;
        continue;
      }
  
      // !! TODO: For regular BES ordinary JSDL is needed - keeping nordugrid:jsdl so far
      std::string product;
      if (!preparedjobdesc.UnParse(product, "nordugrid:jsdl")) {
        logger.msg(INFO, "Unable to submit job. Job description is not valid in the %s format", "nordugrid:jsdl");
        notSubmitted.push_back(&*it);
        ok = false;
        continue;
      }

      std::string idFromEndpoint;
      if (!ac->submit(product, idFromEndpoint, arex_features && (url.Protocol() == "https"))) {
        notSubmitted.push_back(&*it);
        ok = false;
        continue;
      }
  
      if (idFromEndpoint.empty()) {
        logger.msg(INFO, "No job identifier returned by BES service");
        notSubmitted.push_back(&*it);
        ok = false;
        continue;
      }
  
      XMLNode activityIdentifier(idFromEndpoint);
      URL jobid;
      if (activityIdentifier["ReferenceParameters"]["a-rex:JobID"]) { // Service seems to be A-REX. Extract job ID, and upload files.
        jobid = URL((std::string)(activityIdentifier["ReferenceParameters"]["JobSessionDir"]));
    
        if (!PutFiles(preparedjobdesc, jobid)) {
          logger.msg(INFO, "Failed uploading local input files");
          notSubmitted.push_back(&*it);
          ok = false;
          continue;
        }
      } else {
        if (activityIdentifier["Address"]) {
          jobid = URL((std::string)activityIdentifier["Address"]);
        } else {
          jobid = url;
        }
        Time t;
        // Since BES doesn't specify a simple unique ID, but rather an EPR, a unique non-reproduceable (to arcsync) job ID is create below.
        jobid.ChangePath(jobid.Path() + "/BES" + tostring(t.GetTime()) + tostring(t.GetTimeNanosec()));
      }
    
      jobs.push_back(Job());
      jobs.back().IDFromEndpoint = idFromEndpoint;
  
      if (activityIdentifier["ReferenceParameters"]["a-rex:JobID"]) jobs.back().InterfaceName = "org.nordugrid.xbes";

      AddJobDetails(preparedjobdesc, jobid, et.ComputingService->Cluster, jobs.back());
    }
  
    releaseClient(url);
    return ok;
  }

  bool SubmitterPluginARC1::Migrate(const URL& jobid, const JobDescription& jobdesc,
                             const ExecutionTarget& et,
                             bool forcemigration, Job& job) {
    URL url(et.ComputingEndpoint->URLString);

    AREXClient* ac = acquireClient(url);

    std::string idstr;
    AREXClient::createActivityIdentifier(jobid, idstr);

    JobDescription preparedjobdesc(jobdesc);

    // Modify the location of local files and files residing in a old session directory.
    for (std::list<InputFileType>::iterator it = preparedjobdesc.DataStaging.InputFiles.begin();
         it != preparedjobdesc.DataStaging.InputFiles.end(); it++) {
      if (!it->Sources.front() || it->Sources.front().Protocol() == "file") {
        it->Sources.front() = URL(jobid.str() + "/" + it->Name);
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
            it->Sources.front() = URL(jobid.str() + "/" + it->Name);
            break;
          }
        }
      }
    }

    if (!preparedjobdesc.Prepare(et)) {
      logger.msg(INFO, "Failed adapting job description to target resources");
      releaseClient(url);
      return false;
    }

    // Add ActivityOldID.
    preparedjobdesc.Identification.ActivityOldID.push_back(jobid.str());

    std::string product;
    if (!preparedjobdesc.UnParse(product, "nordugrid:jsdl")) {
      logger.msg(INFO, "Unable to migrate job. Job description is not valid in the %s format", "nordugrid:jsdl");
      releaseClient(url);
      return false;
    }

    std::string sNewjobid;
    if (!ac->migrate(idstr, product, forcemigration, sNewjobid,
                    url.Protocol() == "https")) {
      releaseClient(url);
      return false;
    }

    if (sNewjobid.empty()) {
      logger.msg(INFO, "No job identifier returned by A-REX");
      releaseClient(url);
      return false;
    }

    XMLNode xNewjobid(sNewjobid);
    URL newjobid((std::string)(xNewjobid["ReferenceParameters"]["JobSessionDir"]));

    if (!PutFiles(preparedjobdesc, newjobid)) {
      logger.msg(INFO, "Failed uploading local input files");
      releaseClient(url);
      return false;
    }

    AddJobDetails(preparedjobdesc, newjobid, et.ComputingService->Cluster, job);

    releaseClient(url);
    return true;
  }
} // namespace Arc
