// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <map>
#include <utility>

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

  void JobControllerPluginEMIES::SetUserConfig(const UserConfig& uc) {
    JobControllerPlugin::SetUserConfig(uc);
    clients.SetUserConfig(uc);
  }

  bool JobControllerPluginEMIES::isEndpointNotSupported(const std::string& endpoint) const {
    const std::string::size_type pos = endpoint.find("://");
    return pos != std::string::npos && lower(endpoint.substr(0, pos)) != "http" && lower(endpoint.substr(0, pos)) != "https";
  }

  void JobControllerPluginEMIES::UpdateJobs(std::list<Job*>& jobs, std::list<std::string>& IDsProcessed, std::list<std::string>& IDsNotProcessed, bool isGrouped) const {
    if (jobs.empty()) return;

    std::map<std::string, std::list<Job*> > groupedJobs;
    if (!isGrouped) {
      // Group jobs per host.
      for (std::list<Job*>::const_iterator it = jobs.begin();
           it != jobs.end(); ++it) {
        std::map<std::string, std::list<Job*> >::iterator entry = groupedJobs.find((**it).JobManagementURL.str());
        if (entry == groupedJobs.end()) {
          groupedJobs.insert(make_pair((**it).JobManagementURL.str(), std::list<Job*>(1, *it)));
        }
        else {
          entry->second.push_back(*it);
        }
      }
    }
    else {
      groupedJobs.insert(make_pair(jobs.front()->JobManagementURL.str(), jobs));
    }

    for (std::map<std::string, std::list<Job*> >::iterator it = groupedJobs.begin();
         it != groupedJobs.end(); ++it) {
      std::list<EMIESResponse*> responses;
      AutoPointer<EMIESClient> ac(clients.acquire(it->first));
      ac->info(it->second, responses);

      for (std::list<Job*>::iterator itJ = it->second.begin();
           itJ != it->second.end(); ++itJ) {
        std::list<EMIESResponse*>::iterator itR = responses.begin();
        for (; itR != responses.end(); ++itR) {
          EMIESJobInfo *j = dynamic_cast<EMIESJobInfo*>(*itR);
          if (j) {
            if (EMIESJob::getIDFromJob(**itJ) == j->getActivityID()) {
              j->toJob(**itJ);
              if ((*itJ)->LogDir.empty()) {
                // In general EMIES does not provide access to LogDir.
                // But assuming it is ARC EMIES and what we have REST interface 
                // enabled there there is a chance. Here we only put an indicator.
                (*itJ)->LogDir = ".";
              }
              delete *itR;
              break;
            }
          }
          // TODO: Handle ERROR.
          // TODO: Log warning: //logger.msg(WARNING, "Job information not found in the information system: %s", (*it)->JobID);
        }
        if (itR != responses.end()) {
          IDsProcessed.push_back((**itJ).JobID);
          responses.erase(itR);
        }
        else {
          IDsNotProcessed.push_back((**itJ).JobID);
        }
      }

      for (std::list<EMIESResponse*>::iterator itR = responses.begin();
           itR != responses.end(); ++itR) {
        delete *itR;
      }

      clients.release(ac.Release());
    }
  }

  bool JobControllerPluginEMIES::CleanJobs(const std::list<Job*>& jobs, std::list<std::string>& IDsProcessed, std::list<std::string>& IDsNotProcessed, bool isGrouped) const {
    MCCConfig cfg;
    usercfg->ApplyToConfig(cfg);

    bool ok = true;
    for (std::list<Job*>::const_iterator it = jobs.begin(); it != jobs.end(); ++it) {
      Job& job = **it;
      EMIESJob ejob;
      ejob = job;
      AutoPointer<EMIESClient> ac(clients.acquire(ejob.manager));
      if (!ac->clean(ejob)) {
        ok = false;
        IDsNotProcessed.push_back(job.JobID);
        clients.release(ac.Release());
        continue;
      }

      IDsProcessed.push_back(job.JobID);
      clients.release(ac.Release());
    }

    return ok;
  }

  bool JobControllerPluginEMIES::CancelJobs(const std::list<Job*>& jobs, std::list<std::string>& IDsProcessed, std::list<std::string>& IDsNotProcessed, bool isGrouped) const {
    MCCConfig cfg;
    usercfg->ApplyToConfig(cfg);

    bool ok = true;
    for (std::list<Job*>::const_iterator it = jobs.begin(); it != jobs.end(); ++it) {
      Job& job = **it;
      EMIESJob ejob;
      ejob = job;
      AutoPointer<EMIESClient> ac(clients.acquire(ejob.manager));
      if(!ac->kill(ejob)) {
        ok = false;
        IDsNotProcessed.push_back((*it)->JobID);
        clients.release(ac.Release());
        continue;
      }
      // Force assign terminal state so job is cleaned afterwards
      (*it)->State = JobStateEMIES((std::string)"emies:terminal");
      IDsProcessed.push_back((*it)->JobID);
      clients.release(ac.Release());
    }
    return ok;
  }

  bool JobControllerPluginEMIES::RenewJobs(const std::list<Job*>& jobs, std::list<std::string>& IDsProcessed, std::list<std::string>& IDsNotProcessed, bool isGrouped) const {
    MCCConfig cfg;
    usercfg->ApplyToConfig(cfg);

    for (std::list<Job*>::const_iterator it = jobs.begin(); it != jobs.end(); ++it) {
      // 1. Fetch/find delegation ids for each job
      if((*it)->DelegationID.empty()) {
        logger.msg(INFO, "Job %s has no delegation associated. Can't renew such job.", (*it)->JobID);
        IDsNotProcessed.push_back((*it)->JobID);
        continue;
      }
      // 2. Leave only unique IDs - not needed yet because current code uses
      //    different delegations for each job.
      // 3. Renew credentials for every ID
      Job& job = **it;
      EMIESJob ejob;
      ejob = job;
      AutoPointer<EMIESClient> ac(clients.acquire(ejob.manager));
      std::list<std::string>::const_iterator did = (*it)->DelegationID.begin();
      for(;did != (*it)->DelegationID.end();++did) {
        if(ac->delegation(*did).empty()) {
          logger.msg(INFO, "Job %s failed to renew delegation %s - %s.", (*it)->JobID, *did, ac->failure());
          break;
        }
      }
      if(did != (*it)->DelegationID.end()) {
        IDsNotProcessed.push_back((*it)->JobID);
        continue;
      }
      IDsProcessed.push_back((*it)->JobID);
      clients.release(ac.Release());
    }
    return false;
  }

  bool JobControllerPluginEMIES::ResumeJobs(const std::list<Job*>& jobs, std::list<std::string>& IDsProcessed, std::list<std::string>& IDsNotProcessed, bool isGrouped) const {
	    MCCConfig cfg;
	    usercfg->ApplyToConfig(cfg);

	    bool ok = true;
	    for (std::list<Job*>::const_iterator it = jobs.begin(); it != jobs.end(); ++it) {
	      Job& job = **it;
	      if (!job.RestartState) {
	        logger.msg(INFO, "Job %s does not report a resumable state", job.JobID);
	        ok = false;
	        IDsNotProcessed.push_back(job.JobID);
	        continue;
	      }
	      logger.msg(VERBOSE, "Resuming job: %s at state: %s (%s)", job.JobID, job.RestartState.GetGeneralState(), job.RestartState());
	      EMIESJob ejob;
	      ejob = job;
	      AutoPointer<EMIESClient> ac(clients.acquire(ejob.manager));
	      if(!ac->restart(ejob)) {
	        ok = false;
	        IDsNotProcessed.push_back((*it)->JobID);
	        clients.release(ac.Release());
	        continue;
	      }

	      IDsProcessed.push_back((*it)->JobID);
	      clients.release(ac.Release());
	      logger.msg(VERBOSE, "Job resuming successful");
	    }
	    return ok;
  }

  bool JobControllerPluginEMIES::GetURLToJobResource(const Job& job, Job::ResourceType resource, URL& url) const {
    // Obtain information about staging urls
    EMIESJob ejob;
    ejob = job;
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
      usercfg->ApplyToConfig(cfg);
      Job tjob;
      AutoPointer<EMIESClient> ac(clients.acquire(ejob.manager));
      if (!ac->info(ejob, tjob)) {
        clients.release(ac.Release());
        logger.msg(INFO, "Failed retrieving information for job: %s", job.JobID);
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
      // TODO: library need to be extended to allow for multiple URLs
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
      clients.release(ac.Release());
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
    case Job::JOBDESCRIPTION:
      if (job.LogDir == ".") {
        // Trying to use old REST interface
        std::string path = url.Path();
        path.insert(path.rfind('/'), "/*logs");
        url.ChangePath(path + ((resource == Job::JOBLOG) ? "/errors" : "/description"));
      } else if (!job.LogDir.empty()) {
        url.ChangePath(url.Path() + '/' + job.LogDir + ((resource == Job::JOBLOG) ? "/errors" : "/description"));
      } else {
        return false;
      };
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
    case Job::LOGDIR:
      if (job.LogDir == ".") {
        // Trying to use old REST interface
        std::string path = url.Path();
        path.insert(path.rfind('/'), "/*logs");
        url.ChangePath(path);
      } else if (!job.LogDir.empty()) {
        url.ChangePath(url.Path() + '/' + job.LogDir);
      } else {
        return false;
      };
      {
      URL file(url);
      file.ChangePath(url.Path() + "/failed"); url.AddLocation(URLLocation(file, "failed"));
      file.ChangePath(url.Path() + "/local"); url.AddLocation(URLLocation(file, "local"));
      file.ChangePath(url.Path() + "/errors"); url.AddLocation(URLLocation(file, "errors"));
      file.ChangePath(url.Path() + "/description"); url.AddLocation(URLLocation(file, "description"));
      file.ChangePath(url.Path() + "/diag"); url.AddLocation(URLLocation(file, "diag"));
      file.ChangePath(url.Path() + "/comment"); url.AddLocation(URLLocation(file, "comment"));
      file.ChangePath(url.Path() + "/status"); url.AddLocation(URLLocation(file, "status"));
      file.ChangePath(url.Path() + "/acl"); url.AddLocation(URLLocation(file, "acl"));
      file.ChangePath(url.Path() + "/xml"); url.AddLocation(URLLocation(file, "xml"));
      file.ChangePath(url.Path() + "/input"); url.AddLocation(URLLocation(file, "input"));
      file.ChangePath(url.Path() + "/output"); url.AddLocation(URLLocation(file, "output"));
      file.ChangePath(url.Path() + "/input_status"); url.AddLocation(URLLocation(file, "input_status"));
      file.ChangePath(url.Path() + "/output_status"); url.AddLocation(URLLocation(file, "output_status"));
      file.ChangePath(url.Path() + "/statistics"); url.AddLocation(URLLocation(file, "statistics"));
      }
      break;
    default:
      break;
    }
    if(url && ((url.Protocol() == "https") || (url.Protocol() == "http"))) {
      url.AddOption("threads=2",false);
      url.AddOption("encryption=optional",false);
      // url.AddOption("httpputpartial=yes",false); - TODO: use for A-REX
    }

    return true;
  }

  bool JobControllerPluginEMIES::GetJobDescription(const Job& /* job */, std::string& /* desc_str */) const {
    logger.msg(INFO, "Retrieving job description of EMI ES jobs is not supported");
    return false;
  }

} // namespace Arc
