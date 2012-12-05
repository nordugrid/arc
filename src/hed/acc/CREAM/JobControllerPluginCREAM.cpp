// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib.h>

#include <arc/Logger.h>
#include <arc/StringConv.h>
#include <arc/URL.h>
#include <arc/UserConfig.h>
#include <arc/message/MCC.h>

#include "CREAMClient.h"
#include "JobControllerPluginCREAM.h"
#include "JobStateCREAM.h"


namespace Arc {

  Logger JobControllerPluginCREAM::logger(Logger::getRootLogger(), "JobControllerPlugin.CREAM");

  bool JobControllerPluginCREAM::isEndpointNotSupported(const std::string& endpoint) const {
    const std::string::size_type pos = endpoint.find("://");
    return pos != std::string::npos && lower(endpoint.substr(0, pos)) != "http" && lower(endpoint.substr(0, pos)) != "https";
  }

  void JobControllerPluginCREAM::UpdateJobs(std::list<Job*>& jobs, std::list<std::string>& IDsProcessed, std::list<std::string>& IDsNotProcessed, bool isGrouped) const {
    MCCConfig cfg;
    usercfg.ApplyToConfig(cfg);
    for (std::list<Job*>::iterator it = jobs.begin(); it != jobs.end(); ++it) {
      URL url((*it)->JobID);
      PathIterator pi(url.Path(), true);
      url.ChangePath(*pi);
      CREAMClient gLiteClient(url, cfg, usercfg.Timeout());
      if (!gLiteClient.stat(pi.Rest(), (**it))) {
        logger.msg(WARNING, "Job information not found in the information system: %s", (*it)->JobID);
        IDsNotProcessed.push_back((*it)->JobID);
        continue;
      }
      IDsProcessed.push_back((*it)->JobID);
    }
  }

  bool JobControllerPluginCREAM::CleanJobs(const std::list<Job*>& jobs, std::list<std::string>& IDsProcessed, std::list<std::string>& IDsNotProcessed, bool isGrouped) const {
    MCCConfig cfg;
    usercfg.ApplyToConfig(cfg);
    bool ok = true;
    for (std::list<Job*>::const_iterator it = jobs.begin(); it != jobs.end(); ++it) {
      Job& job = **it;
      URL url(job.JobID);
      PathIterator pi(url.Path(), true);
      url.ChangePath(*pi);
      CREAMClient gLiteClient(url, cfg, usercfg.Timeout());
      if (!gLiteClient.purge(pi.Rest())) {
        logger.msg(INFO, "Failed cleaning job: %s", job.JobID);
        ok = false;
        IDsNotProcessed.push_back(job.JobID);
        continue;
      }
      IDsProcessed.push_back(job.JobID);
    }
    return ok;
  }

  bool JobControllerPluginCREAM::CancelJobs(const std::list<Job*>& jobs, std::list<std::string>& IDsProcessed, std::list<std::string>& IDsNotProcessed, bool isGrouped) const {
    MCCConfig cfg;
    usercfg.ApplyToConfig(cfg);
    bool ok = true;
    for (std::list<Job*>::const_iterator it = jobs.begin(); it != jobs.end(); ++it) {
      Job& job = **it;
      URL url(job.JobID);
      PathIterator pi(url.Path(), true);
      url.ChangePath(*pi);
      CREAMClient gLiteClient(url, cfg, usercfg.Timeout());
      if (!gLiteClient.cancel(pi.Rest())) {
        logger.msg(INFO, "Failed canceling job: %s", job.JobID);
        ok = false;
        IDsNotProcessed.push_back(job.JobID);
        continue;
      }
      job.State = JobStateCREAM("CANCELLED");
      IDsProcessed.push_back(job.JobID);
    }
    
    return ok;
  }

  bool JobControllerPluginCREAM::RenewJobs(const std::list<Job*>& jobs, std::list<std::string>&, std::list<std::string>& IDsNotProcessed, bool) const {
    for (std::list<Job*>::const_iterator it = jobs.begin(); it != jobs.end(); ++it) {
      logger.msg(INFO, "Renewal of CREAM jobs is not supported");
      IDsNotProcessed.push_back((*it)->JobID);
    }
    return false;
  }

  bool JobControllerPluginCREAM::ResumeJobs(const std::list<Job*>& jobs, std::list<std::string>&, std::list<std::string>& IDsNotProcessed, bool) const {
    for (std::list<Job*>::const_iterator it = jobs.begin(); it != jobs.end(); ++it) {
      logger.msg(INFO, "Resumation of CREAM jobs is not supported");
      IDsNotProcessed.push_back((*it)->JobID);
    }
    return false;
  }

  bool JobControllerPluginCREAM::GetURLToJobResource(const Job& job, Job::ResourceType resource, URL& url) const {
    creamJobInfo info;
    info = XMLNode(job.IDFromEndpoint);
    
    switch (resource) {
    case Job::STDIN:
    case Job::STDOUT:
    case Job::STDERR:
      return false;
      break;
    case Job::STAGEINDIR:
      if (info.ISB.empty()) {
        return false;
      }
      url = info.ISB;
      break;
    case Job::STAGEOUTDIR:
      if (info.OSB.empty()) {
        return false;
      }
      url = info.OSB;
      break;
    case Job::SESSIONDIR:
    case Job::JOBLOG:
    case Job::JOBDESCRIPTION:
      return false;
      break;
    }
    
    return true;
  }

  bool JobControllerPluginCREAM::GetJobDescription(const Job& /* job */, std::string& /* desc_str */) const {
    return false;
  }

} // namespace Arc
