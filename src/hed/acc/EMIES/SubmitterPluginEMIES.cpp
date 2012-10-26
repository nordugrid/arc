  // -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <unistd.h>
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
#include "JobStateEMIES.h"

namespace Arc {

  Logger SubmitterPluginEMIES::logger(Logger::getRootLogger(), "SubmitterPlugin.EMIES");

  bool SubmitterPluginEMIES::isEndpointNotSupported(const std::string& endpoint) const {
    const std::string::size_type pos = endpoint.find("://");
    return pos != std::string::npos && lower(endpoint.substr(0, pos)) != "http" && lower(endpoint.substr(0, pos)) != "https";
  }
  
  bool SubmitterPluginEMIES::Submit(const std::list<JobDescription>& jobdescs, const std::string& endpoint, EntityConsumer<Job>& jc, std::list<const JobDescription*>& notSubmitted, const URL& jobInformationEndpoint) {
    // TODO: this is multi step process. So having retries would be nice.

    URL iurl;
    URL durl;
    URL url(endpoint);

    bool ok = true;
    for (std::list<JobDescription>::const_iterator it = jobdescs.begin(); it != jobdescs.end(); ++it) {
      bool job_ok = true;

      JobDescription preparedjobdesc(*it);
  
      if (!preparedjobdesc.Prepare()) {
        logger.msg(INFO, "Failed preparing job description");
        notSubmitted.push_back(&*it);
        ok = false;
        continue;
      }

      EMIESJob jobid;
      if(!SubmitterPluginEMIES::submit(preparedjobdesc,url,iurl,durl,jobid)) {
        notSubmitted.push_back(&*it);
        ok = false;
        continue;
      }
 
      URL jobidu(jobid.manager.str() + "/" + jobid.id);

      Job j;
      j.IDFromEndpoint = jobid.ToXML();
      AddJobDetails(preparedjobdesc, jobidu, jobInformationEndpoint, j);
      jc.addEntity(j);
    }

    return ok;
  }

  bool SubmitterPluginEMIES::Submit(const std::list<JobDescription>& jobdescs, const ExecutionTarget& et, EntityConsumer<Job>& jc, std::list<const JobDescription*>& notSubmitted) {
    // TODO: this is multi step process. So having retries would be nice.

    // Submission to EMI ES involves delegation. Delegation may happen through 
    // separate service. Currently existing framework does not provide possibility
    // to collect this information. So service is re-queried again here.
    URL iurl;
    iurl = et.ComputingService->OriginalEndpoint.URLString;
    if(!iurl) iurl = et.ComputingService->Cluster;
    
    URL durl;

    URL url(et.ComputingEndpoint->URLString);

    bool ok = true;
    for (std::list<JobDescription>::const_iterator it = jobdescs.begin(); it != jobdescs.end(); ++it) {
      bool job_ok = true;

      JobDescription preparedjobdesc(*it);
  
      if (!preparedjobdesc.Prepare(et)) {
        logger.msg(INFO, "Failed preparing job description to target resources");
        notSubmitted.push_back(&*it);
        ok = false;
        continue;
      }
  
      EMIESJob jobid;
      if(!SubmitterPluginEMIES::submit(preparedjobdesc,url,iurl,durl,jobid)) {
        notSubmitted.push_back(&*it);
        ok = false;
        continue;
      }
  
      // URL-izing job id
      URL jobidu(jobid.manager.str() + "/" + jobid.id);
      
      Job j;
      j.IDFromEndpoint = jobid.ToXML();
      AddJobDetails(preparedjobdesc, jobidu, et.ComputingService->Cluster, j);
      jc.addEntity(j);
    }

    return ok;
  }


  bool SubmitterPluginEMIES::submit(const JobDescription& preparedjobdesc, const URL& url, const URL& iurl, URL& durl, EMIESJob& jobid) {

    bool job_ok = true;
    std::string product;
    if (!preparedjobdesc.UnParse(product, "emies:adl")) {
      logger.msg(INFO, "Unable to submit job. Job description is not valid in the %s format", "emies:adl");
      return false;
    }

    if(iurl && !durl) {
      AutoPointer<EMIESClient> ac(clients.acquire(iurl));
      std::list<URL> activitycreation;
      std::list<URL> activitymanagememt;
      std::list<URL> activityinfo;
      std::list<URL> resourceinfo;
      std::list<URL> delegation;
      if(ac->sstat(activitycreation,activitymanagememt,activityinfo,resourceinfo,delegation)) {
        for(std::list<URL>::iterator d = delegation.begin(); d !=delegation.end(); ++d) {
          if(d->Protocol() == "https") { // http?
            durl = *d;
            break;
          }
        }
      }
      clients.release(ac.Release());
    }

    std::string delegation_id;
    if(durl) {
      AutoPointer<EMIESClient> ac(clients.acquire(durl));
      delegation_id = ac->delegation();
      clients.release(ac.Release());
    }

    AutoPointer<EMIESClient> ac(clients.acquire(url));
    EMIESJobState jobstate;
    if (!ac->submit(product, jobid, jobstate, delegation_id)) {
      logger.msg(INFO, "Failed to submit job description");
      return false;
    }
  
      if (!jobid) {
        logger.msg(INFO, "No valid job identifier returned by EMI ES");
        return false;
      }
  
      if(!jobid.manager) jobid.manager = url;
  
      // Check if we have anything to upload. Otherwise there is no need to wait.
      bool have_uploads = false;
      for(std::list<InputFileType>::const_iterator itIF =
            preparedjobdesc.DataStaging.InputFiles.begin();
            itIF != preparedjobdesc.DataStaging.InputFiles.end(); ++itIF) {
        if((!itIF->Sources.empty()) && (itIF->Sources.front().Protocol() == "file")) {
          have_uploads = true;
          break;
        };
      };
  
      if(have_uploads) {
        // Wait for job to go into proper state
        for(;;) {
          // TODO: implement timeout
          if(jobstate.HasAttribute(EMIES_SATTR_CLIENT_STAGEIN_POSSIBLE_S)) break;
          if(jobstate.state == EMIES_STATE_TERMINAL_S) {
            logger.msg(INFO, "Job failed on service side");
            job_ok = false;
            break;
          }
          // If service jumped over stageable state client probably does not
          // have to send anything.
          if((jobstate.state != EMIES_STATE_ACCEPTED_S) &&
             (jobstate.state != EMIES_STATE_PREPROCESSING_S)) break;
          sleep(5);
          if(!ac->stat(jobid, jobstate)) {
            logger.msg(INFO, "Failed to obtain state of job");
            job_ok = false;
            break;
          }
        }
        if (!job_ok) {
          return false;
        }
      }
        
      if(have_uploads) {
        if(!jobstate.HasAttribute(EMIES_SATTR_CLIENT_STAGEIN_POSSIBLE_S)) {
          logger.msg(INFO, "Failed to wait for job to allow stage in");
          return false;
        }
        if(jobid.stagein.empty()) {
          // Try to obtain it from job info
          Job tjob;
          if((!ac->info(jobid, tjob)) ||
             (jobid.stagein.empty())) {
            job_ok = false;
          } else {
            job_ok = false;
            for(std::list<URL>::iterator stagein = jobid.stagein.begin();
                         stagein != jobid.stagein.end();++stagein) {
              if(*stagein) {
                job_ok = true;
                break;
              }
            }
          }
          if(!job_ok) {
            logger.msg(INFO, "Failed to obtain valid stagein URL for input files");
            return false;
          }
        }

        job_ok = false;
        for(std::list<URL>::iterator stagein = jobid.stagein.begin();
                       stagein != jobid.stagein.end();++stagein) {
          if(!*stagein) continue;
          // Enhance file upload performance by tuning URL
          if((stagein->Protocol() == "https") || (stagein->Protocol() == "http")) {
            stagein->AddOption("threads=2",false);
            stagein->AddOption("encryption=optional",false);
          }
          if (!PutFiles(preparedjobdesc, *stagein)) {
            logger.msg(INFO, "Failed uploading local input files to %s",stagein->str());
          } else {
            job_ok = true;
          }
        }
        if (!job_ok) {
          return false;
        }
      }
  
      clients.release(ac.Release());

      // It is not clear how service is implemented. So notifying should not harm.
      // Notification must be sent to manager URL.
      AutoPointer<EMIESClient> mac(clients.acquire(jobid.manager));
      if (!mac->notify(jobid)) {
        logger.msg(INFO, "Failed to notify service");
        // TODO: Maybe job should be killed in this case?
        return false;
      }
      clients.release(mac.Release());
  
    return true;
  }



} // namespace Arc
