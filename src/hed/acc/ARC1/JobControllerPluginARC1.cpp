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
#include <arc/Utils.h>

#include "AREXClient.h"
#include "JobControllerPluginARC1.h"

namespace Arc {

  Logger JobControllerPluginARC1::logger(Logger::getRootLogger(), "JobControllerPlugin.ARC1");

  bool JobControllerPluginARC1::isEndpointNotSupported(const std::string& endpoint) const {
    const std::string::size_type pos = endpoint.find("://");
    return pos != std::string::npos && lower(endpoint.substr(0, pos)) != "http" && lower(endpoint.substr(0, pos)) != "https";
  }

  void JobControllerPluginARC1::UpdateJobs(std::list<Job*>& jobs, std::list<URL>& IDsProcessed, std::list<URL>& IDsNotProcessed, bool isGrouped) const {
    for (std::list<Job*>::iterator it = jobs.begin(); it != jobs.end(); ++it) {
      AutoPointer<AREXClient> ac(((AREXClients&)clients).acquire((*it)->Cluster, true));
      std::string idstr;
      AREXClient::createActivityIdentifier((*it)->JobID, idstr);
      if (!ac->stat(idstr, **it)) {
        logger.msg(WARNING, "Job information not found in the information system: %s", (*it)->JobID.fullstr());
        IDsNotProcessed.push_back((*it)->JobID);
        ((AREXClients&)clients).release(ac.Release());
        continue;
      }
      IDsProcessed.push_back((*it)->JobID);
      ((AREXClients&)clients).release(ac.Release());
    }
  }

  bool JobControllerPluginARC1::CleanJobs(const std::list<Job*>& jobs, std::list<URL>& IDsProcessed, std::list<URL>& IDsNotProcessed, bool isGrouped) const {
    bool ok = true;
    for (std::list<Job*>::const_iterator it = jobs.begin(); it != jobs.end(); ++it) {
      Job& job = **it;
      AutoPointer<AREXClient> ac(((AREXClients&)clients).acquire(job.Cluster, true));
      std::string idstr;
      AREXClient::createActivityIdentifier(job.JobID, idstr);
      if (!ac->clean(idstr)) {
        ok = false;
        IDsNotProcessed.push_back(job.JobID);
        ((AREXClients&)clients).release(ac.Release());
        continue;
      }
      IDsProcessed.push_back(job.JobID);
      ((AREXClients&)clients).release(ac.Release());
    }
    
    return ok;
  }

  bool JobControllerPluginARC1::CancelJobs(const std::list<Job*>& jobs, std::list<URL>& IDsProcessed, std::list<URL>& IDsNotProcessed, bool isGrouped) const {
    bool ok = true;
    for (std::list<Job*>::const_iterator it = jobs.begin(); it != jobs.end(); ++it) {
      Job& job = **it;
      AutoPointer<AREXClient> ac(((AREXClients&)clients).acquire(job.Cluster, true));
      std::string idstr;
      AREXClient::createActivityIdentifier(job.JobID, idstr);
      if (!ac->kill(idstr)) {
        ok = false;
        IDsNotProcessed.push_back(job.JobID);
        ((AREXClients&)clients).release(ac.Release());
        continue;
      }
      IDsProcessed.push_back(job.JobID);
      ((AREXClients&)clients).release(ac.Release());
    }
    
    return ok;
  }

  bool JobControllerPluginARC1::RenewJobs(const std::list<Job*>& jobs, std::list<URL>& IDsProcessed, std::list<URL>& IDsNotProcessed, bool isGrouped) const {
    for (std::list<Job*>::const_iterator it = jobs.begin(); it != jobs.end(); ++it) {
      logger.msg(INFO, "Renewal of ARC1 jobs is not supported");
      IDsNotProcessed.push_back((*it)->JobID);
    }
    return false;
  }

  bool JobControllerPluginARC1::ResumeJobs(const std::list<Job*>& jobs, std::list<URL>& IDsProcessed, std::list<URL>& IDsNotProcessed, bool isGrouped) const {
    bool ok = true;
    for (std::list<Job*>::const_iterator it = jobs.begin(); it != jobs.end(); ++it) {
      Job& job = **it;
      if (!job.RestartState) {
        logger.msg(INFO, "Job %s does not report a resumable state", job.JobID.fullstr());
        ok = false;
        IDsNotProcessed.push_back(job.JobID);
        continue;
      }
  
      logger.msg(VERBOSE, "Resuming job: %s at state: %s (%s)", job.JobID.fullstr(), job.RestartState.GetGeneralState(), job.RestartState());
  
      AutoPointer<AREXClient> ac(((AREXClients&)clients).acquire(job.Cluster, true));
      std::string idstr;
      AREXClient::createActivityIdentifier(job.JobID, idstr);
      if (!ac->resume(idstr)) {
        ok = false;
        IDsNotProcessed.push_back(job.JobID);
        ((AREXClients&)clients).release(ac.Release());
        continue;
      }

      IDsProcessed.push_back(job.JobID);
      ((AREXClients&)clients).release(ac.Release());
      logger.msg(VERBOSE, "Job resuming successful");
    }
    
    return ok;
  }

  bool JobControllerPluginARC1::GetURLToJobResource(const Job& job, Job::ResourceType resource, URL& url) const {
    url = job.JobID;
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
    case Job::STAGEINDIR:
    case Job::STAGEOUTDIR:
    case Job::SESSIONDIR:
      break;
    case Job::JOBLOG:
    case Job::JOBDESCRIPTION:
      std::string path = url.Path();
      path.insert(path.rfind('/'), "/info");
      url.ChangePath(path + (Job::JOBLOG ? "/errors" : "/description"));
      break;
    }

    return true;
  }

  bool JobControllerPluginARC1::GetJobDescription(const Job& job, std::string& desc_str) const {
    MCCConfig cfg;
    usercfg.ApplyToConfig(cfg);
    AutoPointer<AREXClient> ac(((AREXClients&)clients).acquire(job.Cluster, true));
    std::string idstr;
    AREXClient::createActivityIdentifier(job.JobID, idstr);
    if (ac->getdesc(idstr, desc_str)) {
      std::list<JobDescription> descs;
      if (JobDescription::Parse(desc_str, descs) && !descs.empty()) {
        ((AREXClients&)clients).release(ac.Release());
        return true;
      }
    }
    ((AREXClients&)clients).release(ac.Release());

    logger.msg(ERROR, "Failed retrieving job description for job: %s", job.JobID.fullstr());
    return false;
  }
} // namespace Arc
