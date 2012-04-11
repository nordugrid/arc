  // -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <sstream>

#include <glibmm.h>

#include <arc/StringConv.h>
#include <arc/UserConfig.h>
#include <arc/client/ExecutionTarget.h>
#include <arc/client/Job.h>
#include <arc/client/JobDescription.h>
#include <arc/message/MCC.h>

#include "SubmitterPluginEMIES.h"
#include "EMIESClient.h"

namespace Arc {

  Logger SubmitterPluginEMIES::logger(Logger::getRootLogger(), "SubmitterPlugin.EMIES");

  bool SubmitterPluginEMIES::isEndpointNotSupported(const std::string& endpoint) const {
    const std::string::size_type pos = endpoint.find("://");
    return pos != std::string::npos && lower(endpoint.substr(0, pos)) != "http" && lower(endpoint.substr(0, pos)) != "https";
  }
  
  EMIESClient* SubmitterPluginEMIES::acquireClient(const URL& url) {
    std::map<URL, EMIESClient*>::const_iterator url_it = clients.find(url);
    if ( url_it != clients.end() ) {
      return url_it->second;
    } else {
      MCCConfig cfg;
      usercfg.ApplyToConfig(cfg);
      EMIESClient* ac = new EMIESClient(url, cfg, usercfg.Timeout());
      return clients[url] = ac;
    }
  }

  bool SubmitterPluginEMIES::releaseClient(const URL& url) {
    return true;
  }

  bool SubmitterPluginEMIES::deleteAllClients() {
    std::map<URL, EMIESClient*>::iterator it;
    for (it = clients.begin(); it != clients.end(); it++) {
        if ((*it).second != NULL) delete (*it).second;
    }
    return true;
  }

  bool SubmitterPluginEMIES::Submit(const JobDescription& jobdesc,
                             const ExecutionTarget& et, Job& job) {
    // TODO: this is multi step process. So having retries would be nice.

    URL url(et.ComputingEndpoint->URLString);

    EMIESClient* ac = acquireClient(url);

    JobDescription preparedjobdesc(jobdesc);

    if (!preparedjobdesc.Prepare(et)) {
      logger.msg(INFO, "Failed preparing job description to target resources");
      releaseClient(url);
      return false;
    }

    std::string product;
    if (!preparedjobdesc.UnParse(product, "emies:adl")) {
      logger.msg(INFO, "Unable to submit job. Job description is not valid in the %s format", "emies:adl");
      releaseClient(url);
      return false;
    }

    EMIESJob jobid;
    EMIESJobState jobstate;
    if (!ac->submit(product, jobid, jobstate, url.Protocol() == "https")) {
      logger.msg(INFO, "Failed to submit job description");
      releaseClient(url);
      return false;
    }

    if (!jobid) {
      logger.msg(INFO, "No valid job identifier returned by EMI ES");
      releaseClient(url);
      return false;
    }

    if(!jobid.manager) jobid.manager = url;

    // Check if we have anything to upload. Otherwise there is no need to wait.
    bool have_uploads = false;
    for(std::list<InputFileType>::const_iterator it =
          preparedjobdesc.DataStaging.InputFiles.begin();
          it != preparedjobdesc.DataStaging.InputFiles.end(); ++it) {
      if((!it->Sources.empty()) && (it->Sources.front().Protocol() == "file")) {
        have_uploads = true;
        break;
      };
    };

    if(have_uploads) for(;;) {
      // TODO: implement timeout
      if(jobstate.HasAttribute("CLIENT-STAGEIN-POSSIBLE")) break;
      if(jobstate.state == "TERMINAL") {
        logger.msg(INFO, "Job failed on service side");
        releaseClient(url);
        return false;
      }
      // If service jumped over stageable state client probably does not
      // have to send anything.
      if((jobstate.state != "ACCEPTED") && (jobstate.state != "PREPROCESSING")) break;
      sleep(5);
      if(!ac->stat(jobid, jobstate)) {
        logger.msg(INFO, "Failed to obtain state of job");
        releaseClient(url);
        return false;
      }
    }

    if(have_uploads) {
      if(!jobstate.HasAttribute("CLIENT-STAGEIN-POSSIBLE")) {
        logger.msg(INFO, "Failed to wait for job to allow stage in");
        releaseClient(url);
        return false;
      }
      URL stageurl(jobid.stagein);
      if(!stageurl) {
        // Try to obtain it from job info
        Job tjob;
        std::string stagein;
        std::string stageout;
        std::string session;
        if((!ac->info(jobid, tjob, stagein, stageout, session)) ||
           stagein.empty() ||
           (!(stageurl = stagein))) {
          logger.msg(INFO, "Failed to obtain valid stagein URL for input files: %s", stagein);
          releaseClient(url);
          return false;
        }
      }
      if (!PutFiles(preparedjobdesc, stageurl)) {
        logger.msg(INFO, "Failed uploading local input files");
        releaseClient(url);
        return false;
      }
    }

    // It is not clear how service is implemented. So notifying should not harm.
    if (!ac->notify(jobid)) {
      logger.msg(INFO, "Failed to notify service");
      releaseClient(url);
      return false;
    }

    // URL-izing job id
    URL jobidu(jobid.manager);
    jobidu.AddOption("emiesjobid",jobid.id,true);

    jobid.ToXML().GetXML(job.IDFromEndpoint);

    AddJobDetails(preparedjobdesc, jobidu, et.ComputingService->Cluster, job);

    releaseClient(url);
    return true;
  }
} // namespace Arc
