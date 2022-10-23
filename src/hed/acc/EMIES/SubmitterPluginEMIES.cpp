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

  SubmitterPluginEMIES::SubmitterPluginEMIES(const UserConfig& usercfg, PluginArgument* parg) : SubmitterPlugin(usercfg, parg),clients(usercfg) {
    supportedInterfaces.push_back("org.ogf.glue.emies.activitycreation");
  }

  SubmitterPluginEMIES::~SubmitterPluginEMIES() { }

  void SubmitterPluginEMIES::SetUserConfig(const UserConfig& uc) {
    SubmitterPlugin::SetUserConfig(uc);
    clients.SetUserConfig(uc);    
  }

  bool SubmitterPluginEMIES::isEndpointNotSupported(const std::string& endpoint) const {
    const std::string::size_type pos = endpoint.find("://");
    return pos != std::string::npos && lower(endpoint.substr(0, pos)) != "http" && lower(endpoint.substr(0, pos)) != "https";
  }
  
  bool SubmitterPluginEMIES::getDelegationID(const URL& durl, std::string& delegation_id) {
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
    
    return true;
  }


  SubmissionStatus SubmitterPluginEMIES::Submit(const std::list<JobDescription>& jobdescs, const std::string& endpoint, EntityConsumer<Job>& jc, std::list<const JobDescription*>& notSubmitted) {
    // TODO: this is multi step process. So having retries would be nice.
    // TODO: If delegation interface is not on same endpoint as submission interface this method is faulty.

    URL url((endpoint.find("://") == std::string::npos ? "https://" : "") + endpoint, false, 443, "/arex");

    SubmissionStatus retval;
    std::string delegation_id;
    std::list<bool> have_uploads;
    XMLNodeList products;
    for (std::list<JobDescription>::const_iterator itJ = jobdescs.begin(); itJ != jobdescs.end(); ++itJ) {
      JobDescription preparedjobdesc(*itJ);
      if (!preparedjobdesc.Prepare()) {
        logger.msg(INFO, "Failed preparing job description");
        notSubmitted.push_back(&*itJ);
        retval |= SubmissionStatus::DESCRIPTION_NOT_SUBMITTED;
        continue;
      }

      {
        std::string jstr;
        JobDescriptionResult ures = preparedjobdesc.UnParse(jstr, "emies:adl");
        if (!ures) {
          logger.msg(INFO, "Unable to submit job. Job description is not valid in the %s format: %s", "emies:adl", ures.str());
          notSubmitted.push_back(&*itJ);
          retval |= SubmissionStatus::DESCRIPTION_NOT_SUBMITTED;
          continue;
        }
        products.push_back(XMLNode());
        XMLNode(jstr).Move(products.back());
        if(!products.back()) {
          logger.msg(INFO, "Unable to submit job. Job description is not valid XML");
          notSubmitted.push_back(&*itJ);
          retval |= SubmissionStatus::DESCRIPTION_NOT_SUBMITTED;
          products.pop_back();
          continue;
        }
      }
      

      have_uploads.push_back(false);
      for(std::list<InputFileType>::const_iterator itIF = itJ->DataStaging.InputFiles.begin();
          itIF != itJ->DataStaging.InputFiles.end() && (!have_uploads.back()); ++itIF) {
        have_uploads.back() = have_uploads.back() || (!itIF->Sources.empty() && (itIF->Sources.front().Protocol() == "file"));
      }

      if (preparedjobdesc.X509Delegation && delegation_id.empty()) {
        // Assume that delegation interface is on same machine as submission interface.
        if (!getDelegationID(url, delegation_id)) {
          notSubmitted.push_back(&*itJ);
          retval |= SubmissionStatus::DESCRIPTION_NOT_SUBMITTED;
          products.pop_back();
          have_uploads.pop_back();
          continue;
        }
      }

      if(have_uploads.back()) {
        // At least CREAM expects to have ClientDataPush for any input file
        std::string prefix = products.back().Prefix();
        Arc::XMLNode stage = products.back()["DataStaging"];
        Arc::XMLNode flag = stage["ClientDataPush"];
        // Following 2 are for satisfying inner paranoic feeling
        if(!stage) stage = products.back().NewChild(prefix+":DataStaging");
        if(!flag) flag = stage.NewChild(prefix+":ClientDataPush",0,true);
        flag = "true";
      }
  
    }
    
    AutoPointer<EMIESClient> ac(clients.acquire(url));
    std::list<EMIESResponse*> responses;
    ac->submit(products, responses, delegation_id);
    
    std::list<bool>::iterator itHU = have_uploads.begin();
    std::list<EMIESResponse*>::iterator itR = responses.begin();
    std::list<JobDescription>::const_iterator itJ = jobdescs.begin();
    
    std::list<EMIESJob*> jobsToNotify;
    std::list<const JobDescription*> jobDescriptionsOfJobsToNotify;
    for (; itR != responses.end() && itHU != have_uploads.end() && itJ != jobdescs.end(); ++itJ, ++itR) {
      EMIESJob *j = dynamic_cast<EMIESJob*>(*itR);
      if (j) {
        if (!(*j)) {
          logger.msg(INFO, "No valid job identifier returned by EMI ES");
          notSubmitted.push_back(&*itJ);
          retval |= SubmissionStatus::DESCRIPTION_NOT_SUBMITTED;
          delete *itR; *itR = NULL;
          continue;
        }

        if(!j->manager) {
          j->manager = url;
        }

        if(j->delegation_id.empty()) {
          j->delegation_id = delegation_id;
        }
  
        JobDescription preparedjobdesc(*itJ);
        preparedjobdesc.Prepare();

        bool job_ok = true;
        // Check if we have anything to upload. Otherwise there is no need to wait.
        if(*itHU) {
          // Wait for job to go into proper state
          for(;;) {
            // TODO: implement timeout
            if(j->state.HasAttribute(EMIES_SATTR_CLIENT_STAGEIN_POSSIBLE_S)) break;
            if(j->state.state == EMIES_STATE_TERMINAL_S) {
              logger.msg(INFO, "Job failed on service side");
              job_ok = false;
              break;
            }
            // If service jumped over stageable state client probably does not
            // have to send anything.
            if((j->state.state != EMIES_STATE_ACCEPTED_S) &&
               (j->state.state != EMIES_STATE_PREPROCESSING_S)) break;
            sleep(5);
            if(!ac->stat(*j, j->state)) {
              logger.msg(INFO, "Failed to obtain state of job");
              job_ok = false;
              break;
            }
          }
          if (!job_ok) {
            notSubmitted.push_back(&*itJ);
            retval |= SubmissionStatus::DESCRIPTION_NOT_SUBMITTED;
            delete *itR; *itR = NULL;
            continue;
          }
        }
            
        if(*itHU) {
          if(!j->state.HasAttribute(EMIES_SATTR_CLIENT_STAGEIN_POSSIBLE_S)) {
            logger.msg(INFO, "Failed to wait for job to allow stage in");
            notSubmitted.push_back(&*itJ);
            retval |= SubmissionStatus::DESCRIPTION_NOT_SUBMITTED;
            delete *itR; *itR = NULL;
            continue;
          }
          if(j->stagein.empty()) {
            // Try to obtain it from job info
            Job tjob;
            if((!ac->info(*j, tjob)) ||
               (j->stagein.empty())) {
              job_ok = false;
            } else {
              job_ok = false;
              for(std::list<URL>::iterator stagein = j->stagein.begin();
                  stagein != j->stagein.end();++stagein) {
                if(*stagein) {
                  job_ok = true;
                  break;
                }
              }
            }
            if(!job_ok) {
              logger.msg(INFO, "Failed to obtain valid stagein URL for input files");
              notSubmitted.push_back(&*itJ);
              retval |= SubmissionStatus::DESCRIPTION_NOT_SUBMITTED;
              delete *itR; *itR = NULL;
              continue;
            }
          }
    
          job_ok = false;
          for(std::list<URL>::iterator stagein = j->stagein.begin();
              stagein != j->stagein.end();++stagein) {
            if(!*stagein) continue;
            // Enhance file upload performance by tuning URL
            if((stagein->Protocol() == "https") || (stagein->Protocol() == "http")) {
              stagein->AddOption("threads=2",false);
              stagein->AddOption("encryption=optional",false);
              // stagein->AddOption("httpputpartial=yes",false); - TODO: use for A-REX
            }
            stagein->AddOption("checksum=no",false);
            if (!PutFiles(preparedjobdesc, *stagein)) {
              logger.msg(INFO, "Failed uploading local input files to %s",stagein->str());
            } else {
              job_ok = true;
            }
          }
          if (!job_ok) {
            notSubmitted.push_back(&*itJ);
            retval |= SubmissionStatus::DESCRIPTION_NOT_SUBMITTED;
            delete *itR; *itR = NULL;
            continue;
          }
        }
        clients.release(ac.Release());

        jobsToNotify.push_back(j);
        jobDescriptionsOfJobsToNotify.push_back(&*itJ);
        ++itHU;
        continue;
      }
      EMIESFault *f = dynamic_cast<EMIESFault*>(*itR);
      if (f) {
        logger.msg(INFO, "Failed to submit job description: EMIESFault(%s , %s)", f->message, f->description);
        notSubmitted.push_back(&*itJ);
        retval |= SubmissionStatus::DESCRIPTION_NOT_SUBMITTED;
        delete *itR; *itR = NULL;
        itHU = have_uploads.erase(itHU);
        continue;
      }
      UnexpectedError *ue = dynamic_cast<UnexpectedError*>(*itR);
      if (ue) {
        logger.msg(INFO, "Failed to submit job description: UnexpectedError(%s)", ue->message);
        notSubmitted.push_back(&*itJ);
        retval |= SubmissionStatus::DESCRIPTION_NOT_SUBMITTED;
        delete *itR; *itR = NULL;
        itHU = have_uploads.erase(itHU);
        continue;
      }
    }

    for (; itJ != jobdescs.end(); ++itJ) {
      notSubmitted.push_back(&*itJ);
      retval |= SubmissionStatus::DESCRIPTION_NOT_SUBMITTED;
    }

    // Safety
    for (; itR != responses.end(); ++itR) {
      delete *itR; *itR = NULL;
    }
    responses.clear();

    if (jobsToNotify.empty()) return retval;
    
    // It is not clear how service is implemented. So notifying should not harm.
    // Notification must be sent to manager URL.
    // Assumption: Jobs is managed by the same manager.
    AutoPointer<EMIESClient> mac(clients.acquire(jobsToNotify.front()->manager));
    mac->notify(jobsToNotify, responses);
    clients.release(mac.Release());
    
    itR = responses.begin(); 
    itHU = have_uploads.begin();
    std::list<EMIESJob*>::iterator itJob = jobsToNotify.begin();
    std::list<const JobDescription*>::const_iterator itJPtr = jobDescriptionsOfJobsToNotify.begin();
    for (; itR != responses.end() && itJPtr != jobDescriptionsOfJobsToNotify.end() && itJob != jobsToNotify.end() && itHU != have_uploads.end();
         ++itR, ++itJPtr, ++itJob, ++itHU) {
      EMIESAcknowledgement *ack = dynamic_cast<EMIESAcknowledgement*>(*itR);
      if (!ack) {
        logger.msg(VERBOSE, "Failed to notify service");
        // TODO: exact logic still requires clarification of specs.
        // TODO: Maybe job should be killed in this case?
        // So far assume if there are no files to upload 
        // activity can survive without notification.
        if(*itHU) {
          notSubmitted.push_back(*itJPtr);
          retval |= SubmissionStatus::DESCRIPTION_NOT_SUBMITTED;
          delete *itR; *itR = NULL;
          continue;
        }
      }
      

      JobDescription preparedjobdesc(**itJPtr);
      preparedjobdesc.Prepare();
      
      Job job;
      (**itJob).toJob(job);
      AddJobDetails(preparedjobdesc, job);
      jc.addEntity(job);
      delete *itR; *itR = NULL;
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
      
      Job j;
      jobid.toJob(j);
      
      AddJobDetails(preparedjobdesc, j);
      jc.addEntity(j);
    }

    return retval;
  }

  static bool urlisinsecure(Arc::URL const & url) {
    std::string protocol = url.Protocol();
    return protocol.empty() || (protocol == "http") || (protocol == "ftp") || (protocol == "ldap");
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
    for(std::list<InputFileType>::const_iterator itIF =
          preparedjobdesc.DataStaging.InputFiles.begin();
          itIF != preparedjobdesc.DataStaging.InputFiles.end(); ++itIF) {
      if(have_uploads)  break;
      if(!itIF->Sources.empty()) {
        if(itIF->Sources.front().Protocol() == "file") {
          have_uploads = true;
        }
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

    if(iurl && !durl && preparedjobdesc.X509Delegation) {
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
    if(preparedjobdesc.X509Delegation) {
      if(!getDelegationID(durl, delegation_id)) return false;
    }

    EMIESResponse *response = NULL;
    AutoPointer<EMIESClient> ac(clients.acquire(url));
    if (!ac->submit(product, &response, delegation_id)) {
      delete response;
      logger.msg(INFO, "Failed to submit job description: %s", ac->failure());
      return false;
    }
  
    EMIESJob* jobid_ptr = dynamic_cast<EMIESJob*>(response);
    if (!jobid_ptr) {
      delete response;
      logger.msg(INFO, "No valid job identifier returned by EMI ES");
      return false;
    }
  
    jobid = *jobid_ptr;
    delete response;
    
    if(!jobid.manager) jobid.manager = url;
    if(jobid.delegation_id.empty()) jobid.delegation_id = delegation_id;
  
    // Check if we have anything to upload. Otherwise there is no need to wait.
    if(have_uploads) {
      // Wait for job to go into proper state
      for(;;) {
        // TODO: implement timeout
        if(jobid.state.HasAttribute(EMIES_SATTR_CLIENT_STAGEIN_POSSIBLE_S)) break;
        if(jobid.state.state == EMIES_STATE_TERMINAL_S) {
          logger.msg(INFO, "Job failed on service side");
          job_ok = false;
          break;
        }
        // If service jumped over stageable state client probably does not
        // have to send anything.
        if((jobid.state.state != EMIES_STATE_ACCEPTED_S) &&
           (jobid.state.state != EMIES_STATE_PREPROCESSING_S)) break;
        sleep(5);
        if(!ac->stat(jobid, jobid.state)) {
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
      if(!jobid.state.HasAttribute(EMIES_SATTR_CLIENT_STAGEIN_POSSIBLE_S)) {
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
          // stagein->AddOption("httpputpartial=yes",false); - TODO: use for A-REX
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
