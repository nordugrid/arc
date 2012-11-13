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

#include "AREXClient.h"
#include "JobControllerPluginBES.h"
#include "JobStateBES.h"

namespace Arc {

  Logger JobControllerPluginBES::logger(Logger::getRootLogger(), "JobControllerPlugin.BES");

  bool JobControllerPluginBES::isEndpointNotSupported(const std::string& endpoint) const {
    const std::string::size_type pos = endpoint.find("://");
    return pos != std::string::npos && lower(endpoint.substr(0, pos)) != "http" && lower(endpoint.substr(0, pos)) != "https";
  }

  void JobControllerPluginBES::UpdateJobs(std::list<Job*>& jobs, std::list<URL>& IDsProcessed, std::list<URL>& IDsNotProcessed, bool isGrouped) const {
    MCCConfig cfg;
    usercfg.ApplyToConfig(cfg);

    for (std::list<Job*>::iterator it = jobs.begin(); it != jobs.end(); ++it) {
      AREXClient ac((*it)->JobStatusURL, cfg, usercfg.Timeout(),false);
      if (!ac.stat((*it)->IDFromEndpoint, **it)) {
        logger.msg(INFO, "Failed retrieving job status information");
        IDsNotProcessed.push_back((*it)->JobID);
        continue;
      }
      IDsProcessed.push_back((*it)->JobID);
    }
  }

  bool JobControllerPluginBES::CleanJobs(const std::list<Job*>& jobs, std::list<URL>&, std::list<URL>& IDsNotProcessed, bool) const {
    for (std::list<Job*>::const_iterator it = jobs.begin(); it != jobs.end(); ++it) {
      logger.msg(INFO, "Cleaning of BES jobs is not supported");
      IDsNotProcessed.push_back((*it)->JobID);
    }
    return false;
  }

  bool JobControllerPluginBES::CancelJobs(const std::list<Job*>& jobs, std::list<URL>& IDsProcessed, std::list<URL>& IDsNotProcessed, bool isGrouped) const {
    MCCConfig cfg;
    usercfg.ApplyToConfig(cfg);
    bool ok = true;
    for (std::list<Job*>::const_iterator it = jobs.begin(); it != jobs.end(); ++it) {
      Job& job = **it;
      AREXClient ac(job.JobManagementURL, cfg, usercfg.Timeout(), false);
      if (!ac.kill(job.IDFromEndpoint)) {
        ok = false;
        IDsNotProcessed.push_back(job.JobID);
        continue;
      }
      job.State = JobStateBES("cancelled");
      IDsProcessed.push_back(job.JobID);
    }
    return ok;
  }

  bool JobControllerPluginBES::RenewJobs(const std::list<Job*>& jobs, std::list<URL>&, std::list<URL>& IDsNotProcessed, bool) const {
    for (std::list<Job*>::const_iterator it = jobs.begin(); it != jobs.end(); ++it) {
      logger.msg(INFO, "Renewal of BES jobs is not supported");
      IDsNotProcessed.push_back((*it)->JobID);
    }
    return false;
  }

  bool JobControllerPluginBES::ResumeJobs(const std::list<Job*>& jobs, std::list<URL>&, std::list<URL>& IDsNotProcessed, bool) const {
    for (std::list<Job*>::const_iterator it = jobs.begin(); it != jobs.end(); ++it) {
      logger.msg(INFO, "Resuming BES jobs is not supported");
      IDsNotProcessed.push_back((*it)->JobID);
    }
    return false;
  }

  bool JobControllerPluginBES::GetJobDescription(const Job& job, std::string& desc_str) const {
    MCCConfig cfg;
    usercfg.ApplyToConfig(cfg);
    AREXClient ac(job.JobManagementURL, cfg, usercfg.Timeout(), false);
    if (ac.getdesc(job.IDFromEndpoint, desc_str)) {
      std::list<JobDescription> descs;
      if (JobDescription::Parse(desc_str, descs) && !descs.empty()) {
        return true;
      }
    }

    logger.msg(ERROR, "Failed retrieving job description for job: %s", job.JobID.fullstr());
    return false;
  }

  URL JobControllerPluginBES::CreateURL(std::string service, ServiceType /* st */) const {
    std::string::size_type pos1 = service.find("://");
    if (pos1 == std::string::npos)
      service = "https://" + service;
    // Default port other than 443?
    // Default path?
    return service;
  }

} // namespace Arc
