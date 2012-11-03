// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib.h>

#include <arc/StringConv.h>
#include <arc/UserConfig.h>
#include <arc/XMLNode.h>
#include <arc/compute/JobDescription.h>
#include <arc/data/DataMover.h>
#include <arc/data/DataHandle.h>
#include <arc/data/URLMap.h>
#include <arc/message/MCC.h>
#include <arc/Utils.h>

#include "EMIESClient.h"
#include "JobStateEMIES.h"
#include "JobControllerPluginEMIES.h"

namespace Arc {

  Logger JobControllerPluginEMIES::logger(Logger::getRootLogger(), "JobControllerPlugin.EMIES");

  bool JobControllerPluginEMIES::isEndpointNotSupported(const std::string& endpoint) const {
    const std::string::size_type pos = endpoint.find("://");
    return pos != std::string::npos && lower(endpoint.substr(0, pos)) != "http" && lower(endpoint.substr(0, pos)) != "https";
  }
  
  void JobControllerPluginEMIES::UpdateJobs(std::list<Job*>& jobs, std::list<URL>& IDsProcessed, std::list<URL>& IDsNotProcessed, bool isGrouped) const {
    for (std::list<Job*>::iterator it = jobs.begin();
         it != jobs.end(); ++it) {
      bool job_ok = false;
      EMIESJob job;
      job = (*it)->IDFromEndpoint;
      AutoPointer<EMIESClient> ac(((EMIESClients&)clients).acquire(job.manager));
      if (!ac->info(job, **it)) {
        logger.msg(WARNING, "Job information not found in the information system: %s", (*it)->JobID.fullstr());
        //IDsNotProcessed.push_back((*it)->JobID);
        //((EMIESClients&)clients).release(ac.Release());
        //continue;
      } else {
        job_ok = true;
      }
      // Going for more detailed state
      XMLNode jst;
      if (!ac->stat(job, jst)) {
      } else {
        JobStateEMIES jst_ = jst;
        if(jst_) (*it)->State = jst_;
        job_ok = true;
      }
      if(job_ok) {
        IDsProcessed.push_back((*it)->JobID);
      } else {
        IDsNotProcessed.push_back((*it)->JobID);
      }
      ((EMIESClients&)clients).release(ac.Release());
    }
  }

  bool JobControllerPluginEMIES::CleanJobs(const std::list<Job*>& jobs, std::list<URL>& IDsProcessed, std::list<URL>& IDsNotProcessed, bool isGrouped) const {
    MCCConfig cfg;
    usercfg.ApplyToConfig(cfg);

    bool ok = true;
    for (std::list<Job*>::const_iterator it = jobs.begin(); it != jobs.end(); ++it) {
      Job& job = **it;
      EMIESJob ejob;
      ejob = job.IDFromEndpoint;
      AutoPointer<EMIESClient> ac(((EMIESClients&)clients).acquire(ejob.manager));
      if (!ac->clean(ejob)) {
        ok = false;
        IDsNotProcessed.push_back(job.JobID);
        ((EMIESClients&)clients).release(ac.Release());
        continue;
      }
      
      IDsProcessed.push_back(job.JobID);
      ((EMIESClients&)clients).release(ac.Release());
    }
    
    return ok;
  }

  bool JobControllerPluginEMIES::CancelJobs(const std::list<Job*>& jobs, std::list<URL>& IDsProcessed, std::list<URL>& IDsNotProcessed, bool isGrouped) const {
    MCCConfig cfg;
    usercfg.ApplyToConfig(cfg);

    bool ok = true;
    for (std::list<Job*>::const_iterator it = jobs.begin(); it != jobs.end(); ++it) {
      Job& job = **it;
      EMIESJob ejob;
      ejob = job.IDFromEndpoint;
      AutoPointer<EMIESClient> ac(((EMIESClients&)clients).acquire(ejob.manager));
      if(!ac->kill(ejob)) {
        ok = false;
        IDsNotProcessed.push_back((*it)->JobID);
        ((EMIESClients&)clients).release(ac.Release());
        continue;
      }
      
      (*it)->State = JobStateEMIES((std::string)"emies:TERMINAL");
      IDsProcessed.push_back((*it)->JobID);
      ((EMIESClients&)clients).release(ac.Release());
    }
    return ok;
  }

  bool JobControllerPluginEMIES::RenewJobs(const std::list<Job*>& jobs, std::list<URL>& IDsProcessed, std::list<URL>& IDsNotProcessed, bool isGrouped) const {
    for (std::list<Job*>::const_iterator it = jobs.begin(); it != jobs.end(); ++it) {
      logger.msg(INFO, "Renewal of EMI ES jobs is not supported");
      IDsNotProcessed.push_back((*it)->JobID);
    }
    return false;
  }

  bool JobControllerPluginEMIES::ResumeJobs(const std::list<Job*>& jobs, std::list<URL>& IDsProcessed, std::list<URL>& IDsNotProcessed, bool isGrouped) const {
    for (std::list<Job*>::const_iterator it = jobs.begin(); it != jobs.end(); ++it) {
      logger.msg(INFO, "Resume of EMI ES jobs is not supported");
      IDsNotProcessed.push_back((*it)->JobID);
    }
    return false;
  }

  bool JobControllerPluginEMIES::GetURLToJobResource(const Job& job, Job::ResourceType resource, URL& url) const {
    if (resource == Job::JOBDESCRIPTION) {
      return false;
    }
    
    // Obtain information about staging urls
    EMIESJob ejob;
    ejob = job.IDFromEndpoint;
    URL stagein;
    URL stageout;
    URL session;
    // TODO: currently using first valid URL. Need support for multiple.
    for(std::list<URL>::iterator s = ejob.stagein.begin();s!=ejob.stagein.end();++s) {
      if(*s) { stagein = *s; break; }
    }
    for(std::list<URL>::iterator s = ejob.stageout.begin();s!=ejob.stageout.end();++s) {
      if(*s) { stageout = *s; break; }
    }
    for(std::list<URL>::iterator s = ejob.session.begin();s!=ejob.session.end();++s) {
      if(*s) { session = *s; break; }
    }

    if ((resource != Job::STAGEINDIR  || !stagein)  &&
        (resource != Job::STAGEOUTDIR || !stageout) &&
        (resource != Job::SESSIONDIR  || !session)) {
      // If there is no needed URL provided try to fetch it from server
      MCCConfig cfg;
      usercfg.ApplyToConfig(cfg);
      Job tjob;
      AutoPointer<EMIESClient> ac(((EMIESClients&)clients).acquire(ejob.manager));
      if (!ac->info(ejob, tjob)) {
        ((EMIESClients&)clients).release(ac.Release());
        logger.msg(INFO, "Failed retrieving information for job: %s", job.JobID.fullstr());
        return false;
      }
      for(std::list<URL>::iterator s = ejob.stagein.begin();s!=ejob.stagein.end();++s) {
        if(*s) { stagein = *s; break; }
      }
      for(std::list<URL>::iterator s = ejob.stageout.begin();s!=ejob.stageout.end();++s) {
        if(*s) { stageout = *s; break; }
      }
      for(std::list<URL>::iterator s = ejob.session.begin();s!=ejob.session.end();++s) {
        if(*s) { session = *s; break; }
      }
      // Choose url by state
      // TODO: maybe this method should somehow know what is purpose of URL
      // TODO: state attributes would be more suitable
      // TODO: library need to be etended to allow for multiple URLs
      if((tjob.State == JobState::ACCEPTED) ||
         (tjob.State == JobState::PREPARING)) {
        url = stagein;
      } else if((tjob.State == JobState::DELETED) ||
                (tjob.State == JobState::FAILED) ||
                (tjob.State == JobState::KILLED) ||
                (tjob.State == JobState::FINISHED) ||
                (tjob.State == JobState::FINISHING)) {
        url = stageout;
      } else {
        url = session;
      }
      // If no url found by state still try to get something
      if(!url) {
        if(session)  url = session;
        if(stagein)  url = stagein;
        if(stageout) url = stageout;
      }
      ((EMIESClients&)clients).release(ac.Release());
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
      if(stagein) url = stagein;
      break;
    case Job::STAGEOUTDIR:
      if(stageout) url = stageout;
      break;
    case Job::SESSIONDIR:
      if(session) url = session;
      break;
    default:
      break;
    }
    if(url && ((url.Protocol() == "https") || (url.Protocol() == "http"))) {
      url.AddOption("threads=2",false);
      url.AddOption("encryption=optional",false);
    }

    return true;
  }

  bool JobControllerPluginEMIES::GetJobDescription(const Job& /* job */, std::string& /* desc_str */) const {
    logger.msg(INFO, "Retrieving job description of EMI ES jobs is not supported");
    return false;
  }

} // namespace Arc
