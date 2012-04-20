// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib.h>

#include <arc/StringConv.h>
#include <arc/UserConfig.h>
#include <arc/XMLNode.h>
#include <arc/client/JobDescription.h>
#include <arc/data/DataMover.h>
#include <arc/data/DataHandle.h>
#include <arc/data/URLMap.h>
#include <arc/message/MCC.h>

#include "EMIESClient.h"
#include "JobStateEMIES.h"
#include "JobControllerEMIES.h"

namespace Arc {

  Logger JobControllerEMIES::logger(Logger::getRootLogger(), "JobController.EMIES");

  bool JobControllerEMIES::isEndpointNotSupported(const std::string& endpoint) const {
    const std::string::size_type pos = endpoint.find("://");
    return pos != std::string::npos && lower(endpoint.substr(0, pos)) != "http" && lower(endpoint.substr(0, pos)) != "https";
  }
  
  void JobControllerEMIES::UpdateJobs(std::list<Job*>& jobs, std::list<URL>& IDsProcessed, std::list<URL>& IDsNotProcessed, bool isGrouped) const {
    MCCConfig cfg;
    usercfg.ApplyToConfig(cfg);

    for (std::list<Job*>::iterator it = jobs.begin();
         it != jobs.end(); ++it) {
      EMIESJob job;
      job = (*it)->IDFromEndpoint;
      EMIESClient ac(job.manager, cfg, usercfg.Timeout());
      if (!ac.info(job, **it)) {
        logger.msg(WARNING, "Job information not found in the information system: %s", (*it)->JobID.fullstr());
        IDsNotProcessed.push_back((*it)->JobID);
        continue;
      }
      // Going for more detailed state
      XMLNode jst;
      if (!ac.stat(job, jst)) {
      } else {
        JobStateEMIES jst_ = jst;
        if(jst_) (*it)->State = jst_;
      }
      IDsProcessed.push_back((*it)->JobID);
    }
  }

  bool JobControllerEMIES::CleanJobs(const std::list<Job*>& jobs, std::list<URL>& IDsProcessed, std::list<URL>& IDsNotProcessed, bool isGrouped) const {
    MCCConfig cfg;
    usercfg.ApplyToConfig(cfg);

    bool ok = true;
    for (std::list<Job*>::const_iterator it = jobs.begin(); it != jobs.end(); ++it) {
      Job& job = **it;
      EMIESJob ejob;
      ejob = job.IDFromEndpoint;
      EMIESClient ac(ejob.manager, cfg, usercfg.Timeout());
      if (!ac.clean(ejob)) {
        ok = false;
        IDsNotProcessed.push_back(job.JobID);
        continue;
      }
      
      IDsProcessed.push_back(job.JobID);
    }
    
    return ok;
  }

  bool JobControllerEMIES::CancelJobs(const std::list<Job*>& jobs, std::list<URL>& IDsProcessed, std::list<URL>& IDsNotProcessed, bool isGrouped) const {
    for (std::list<Job*>::const_iterator it = jobs.begin(); it != jobs.end(); ++it) {
      logger.msg(INFO, "Cancel of EMI ES jobs is not supported");
      IDsNotProcessed.push_back((*it)->JobID);
    }
    return false;
  }

  bool JobControllerEMIES::RenewJobs(const std::list<Job*>& jobs, std::list<URL>& IDsProcessed, std::list<URL>& IDsNotProcessed, bool isGrouped) const {
    for (std::list<Job*>::const_iterator it = jobs.begin(); it != jobs.end(); ++it) {
      logger.msg(INFO, "Renewal of EMI ES jobs is not supported");
      IDsNotProcessed.push_back((*it)->JobID);
    }
    return false;
  }

  bool JobControllerEMIES::ResumeJobs(const std::list<Job*>& jobs, std::list<URL>& IDsProcessed, std::list<URL>& IDsNotProcessed, bool isGrouped) const {
    for (std::list<Job*>::const_iterator it = jobs.begin(); it != jobs.end(); ++it) {
      logger.msg(INFO, "Resume of EMI ES jobs is not supported");
      IDsNotProcessed.push_back((*it)->JobID);
    }
    return false;
  }

  bool JobControllerEMIES::GetURLToJobResource(const Job& job, Job::ResourceType resource, URL& url) const {
    if (resource == Job::JOBDESCRIPTION) {
      return false;
    }
    
    // Obtain information about staging urls
    EMIESJob ejob;
    ejob = job.IDFromEndpoint;

    if ((resource != Job::STAGEINDIR  || !ejob.stagein)  &&
        (resource != Job::STAGEOUTDIR || !ejob.stageout) &&
        (resource != Job::SESSIONDIR  || !ejob.session)) {
      MCCConfig cfg;
      usercfg.ApplyToConfig(cfg);
      Job tjob;
      EMIESClient ac(ejob.manager, cfg, usercfg.Timeout());
      if (!ac.info(ejob, tjob)) {
        logger.msg(INFO, "Failed retrieving information for job: %s", job.JobID.fullstr());
        return false;
      }
      // Choose url by state
      // TODO: maybe this method should somehow know what is purpose of URL
      // TODO: state attributes would be more suitable
      if((tjob.State == JobState::ACCEPTED) ||
         (tjob.State == JobState::PREPARING)) {
        url = ejob.stagein;
      } else if((tjob.State == JobState::DELETED) ||
                (tjob.State == JobState::FAILED) ||
                (tjob.State == JobState::KILLED) ||
                (tjob.State == JobState::FINISHED) ||
                (tjob.State == JobState::FINISHING)) {
        url = ejob.stageout;
      } else {
        url = ejob.session;
      }
      // If no url found by state still try to get something
      if(!url) {
        if(ejob.session)  url = ejob.session;
        if(ejob.stagein)  url = ejob.stagein;
        if(ejob.stageout) url = ejob.stageout;
      }
    }
    
    switch (resource) {
    case Job::STDIN:
      url.ChangePath(url.Path() + '/' + job.StdIn);
      break;
    case Job::STDOUT:
      url.ChangePath(url.Path() + '/' + job.StdOut);
      break;
    case Job::STDERR:
      url.ChangePath(url.Path() + '/' + job.StdErr);
      break;
    case Job::JOBLOG:
      url.ChangePath(url.Path() + "/" + job.LogDir + "/errors");
      break;
    case Job::STAGEINDIR:
      url = ejob.stagein;
      break;
    case Job::STAGEOUTDIR:
      url = ejob.stageout;
      break;
    case Job::SESSIONDIR:
      url = ejob.session;
      break;
    default:
      break;
    }
    return true;
  }

  bool JobControllerEMIES::GetJobDescription(const Job& /* job */, std::string& /* desc_str */) const {
    logger.msg(INFO, "Retrieving job description of EMI ES jobs is not supported");
    return false;
  }

} // namespace Arc
