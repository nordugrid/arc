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
#include <arc/compute/ExecutionTarget.h>
#include <arc/compute/Job.h>
#include <arc/compute/JobDescription.h>
#include <arc/compute/SubmissionStatus.h>
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
  
  static URL CreateURL(std::string str) {
    std::string::size_type pos1 = str.find("://");
    if (pos1 == std::string::npos) {
      str = "https://" + str;
      pos1 = 5;
    } else {
      if(lower(str.substr(0,pos1)) != "https" && lower(str.substr(0,pos1)) != "http" ) return URL();
    }
    std::string::size_type pos2 = str.find(":", pos1 + 3);
    std::string::size_type pos3 = str.find("/", pos1 + 3);
    if (pos3 == std::string::npos && pos2 == std::string::npos)  str += ":443";
    else if (pos2 == std::string::npos || pos2 > pos3) str.insert(pos3, ":443");

    return str;
  }
  
  SubmissionStatus SubmitterPluginEMIES::Submit(const std::list<JobDescription>& jobdescs, const std::string& endpoint, EntityConsumer<Job>& jc, std::list<const JobDescription*>& notSubmitted) {
    // TODO: this is multi step process. So having retries would be nice.

    URL url = CreateURL(endpoint);

    SubmissionStatus retval;
    for (std::list<JobDescription>::const_iterator it = jobdescs.begin(); it != jobdescs.end(); ++it) {

      JobDescription preparedjobdesc(*it);
  
      if (!preparedjobdesc.Prepare()) {
        logger.msg(INFO, "Failed preparing job description");
        notSubmitted.push_back(&*it);
        retval |= SubmissionStatus::DESCRIPTION_NOT_SUBMITTED;
        continue;
      }

      EMIESJob jobid;
      if(!SubmitterPluginEMIES::submit(preparedjobdesc,url,URL(),URL(),jobid)) {
        notSubmitted.push_back(&*it);
        retval |= SubmissionStatus::DESCRIPTION_NOT_SUBMITTED;
        retval |= SubmissionStatus::ERROR_FROM_ENDPOINT;
        continue;
      }
 
      Job j = jobid.ToJob();
      AddJobDetails(preparedjobdesc, j);
      jc.addEntity(j);
    }

    return retval;
  }

  SubmissionStatus SubmitterPluginEMIES::Submit(const std::list<JobDescription>& jobdescs, const ExecutionTarget& et, EntityConsumer<Job>& jc, std::list<const JobDescription*>& notSubmitted) {
    // TODO: this is multi step process. So having retries would be nice.

    // Submission to EMI ES involves delegation. Delegation may happen through 
    // separate service. Currently existing framework does not provide possibility
    // to collect this information. So service is re-queried again here.
    URL iurl;
    iurl = et.ComputingService->InformationOriginEndpoint.URLString;
    
    URL durl;
    
    for (std::list< CountedPointer<ComputingEndpointAttributes> >::const_iterator it = et.OtherEndpoints.begin(); it != et.OtherEndpoints.end(); it++) {
      if ((*it)->InterfaceName == "org.ogf.glue.emies.delegation") {
        durl = URL((*it)->URLString);
      }
    }

    URL url(et.ComputingEndpoint->URLString);

    SubmissionStatus retval;
    for (std::list<JobDescription>::const_iterator it = jobdescs.begin(); it != jobdescs.end(); ++it) {

      JobDescription preparedjobdesc(*it);
  
      if (!preparedjobdesc.Prepare(et)) {
        logger.msg(INFO, "Failed preparing job description to target resources");
        notSubmitted.push_back(&*it);
        retval |= SubmissionStatus::DESCRIPTION_NOT_SUBMITTED;
        continue;
      }
  
      EMIESJob jobid;
      if(!SubmitterPluginEMIES::submit(preparedjobdesc,url,iurl,durl,jobid)) {
        notSubmitted.push_back(&*it);
        retval |= SubmissionStatus::DESCRIPTION_NOT_SUBMITTED;
        retval |= SubmissionStatus::ERROR_FROM_ENDPOINT;
        continue;
      }
      
      Job j = jobid.ToJob();
      
      AddJobDetails(preparedjobdesc, j);
      jc.addEntity(j);
    }

    return retval;
  }


  bool SubmitterPluginEMIES::submit(const JobDescription& preparedjobdesc, const URL& url, const URL& iurl, URL durl, EMIESJob& jobid) {

    bool job_ok = true;

    Arc::XMLNode product;
    {
      std::string jstr;
      JobDescriptionResult ures = preparedjobdesc.UnParse(jstr, "emies:adl");
      if (!ures) {
        logger.msg(INFO, "Unable to submit job. Job description is not valid in the %s format: %s", "emies:adl", ures.str());
        return false;
      }
      XMLNode(jstr).Move(product);
      if(!product) {
        logger.msg(INFO, "Unable to submit job. Job description is not valid XML");
        return false;
      }
    }

    bool have_uploads = false;
    bool need_delegation = false;
    for(std::list<InputFileType>::const_iterator itIF =
          preparedjobdesc.DataStaging.InputFiles.begin();
          itIF != preparedjobdesc.DataStaging.InputFiles.end(); ++itIF) {
      if(need_delegation && have_uploads)  break;
      if(!itIF->Sources.empty()) {
        if(itIF->Sources.front().Protocol() == "file") {
          have_uploads = true;
        } else {
          need_delegation = true;
        }
      }
    }
    for(std::list<OutputFileType>::const_iterator itOF =
          preparedjobdesc.DataStaging.OutputFiles.begin();
          itOF != preparedjobdesc.DataStaging.OutputFiles.end(); ++itOF) {
      if(need_delegation)  break;
      if(!itOF->Targets.empty()) {
        need_delegation = true;
      }
    }

    if(have_uploads) {
      // At least CREAM expects to have ClientDataPush for any input file
      std::string prefix = product.Prefix();
      Arc::XMLNode stage = product["DataStaging"];
      Arc::XMLNode flag = stage["ClientDataPush"];
      // Following 2 are for satisfying inner paranoic feeling
      if(!stage) stage = product.NewChild(prefix+":DataStaging");
      if(!flag) flag = stage.NewChild(prefix+":ClientDataPush",0,true);
      flag = "true";
    }

    if(iurl && !durl && need_delegation) {
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
    if(need_delegation) {
      if(!durl) {
        logger.msg(INFO, "Failed to delegate credentials to server - no delegation interface found");
        return false;
      }
      AutoPointer<EMIESClient> ac(clients.acquire(durl));
      delegation_id = ac->delegation();
      if(delegation_id.empty()) {
        logger.msg(INFO, "Failed to delegate credentials to server - %s",ac->failure());
        return false;
      }
      clients.release(ac.Release());
    }

    AutoPointer<EMIESClient> ac(clients.acquire(url));
    EMIESJobState jobstate;
    if (!ac->submit(product, jobid, jobstate, delegation_id)) {
      logger.msg(INFO, "Failed to submit job description: %s", ac->failure());
      return false;
    }
  
    if (!jobid) {
      logger.msg(INFO, "No valid job identifier returned by EMI ES");
      return false;
    }
  
    if(!jobid.manager) jobid.manager = url;
  
    // Check if we have anything to upload. Otherwise there is no need to wait.
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
        stagein->AddOption("checksum=no",false);
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
      // TODO: exact logic still requires clarification of specs.
      // TODO: Maybe job should be killed in this case?
      // So far assume if there are no files to upload 
      // activity can survive without notification.
      if(have_uploads) return false;
    } else {
      clients.release(mac.Release());
    }
  
    return true;
  }



} // namespace Arc
