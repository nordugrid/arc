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

#include "AREXClient.h"
#include "JobControllerBES.h"

namespace Arc {

  Logger JobControllerBES::logger(Logger::getRootLogger(), "JobController.BES");

  bool JobControllerBES::isEndpointNotSupported(const std::string& endpoint) const {
    const std::string::size_type pos = endpoint.find("://");
    return pos != std::string::npos && lower(endpoint.substr(0, pos)) != "http" && lower(endpoint.substr(0, pos)) != "https";
  }

  void JobControllerBES::UpdateJobs(std::list<Job*>& jobs, std::list<URL>& IDsProcessed, std::list<URL>& IDsNotProcessed, bool isGrouped) const {
    MCCConfig cfg;
    usercfg.ApplyToConfig(cfg);

    for (std::list<Job*>::iterator it = jobs.begin(); it != jobs.end(); ++it) {
      AREXClient ac((*it)->Cluster, cfg, usercfg.Timeout(),false);
      if (!ac.stat((*it)->IDFromEndpoint, **it)) {
        logger.msg(INFO, "Failed retrieving job status information");
        IDsNotProcessed.push_back((*it)->JobID);
        continue;
      }
      IDsProcessed.push_back((*it)->JobID);
    }
  }

  bool JobControllerBES::CleanJobs(const std::list<Job*>& jobs, std::list<URL>&, std::list<URL>& IDsNotProcessed, bool) const {
    for (std::list<Job*>::const_iterator it = jobs.begin(); it != jobs.end(); ++it) {
      logger.msg(INFO, "Cleaning of BES jobs is not supported");
      IDsNotProcessed.push_back((*it)->JobID);
    }
    return false;
  }

  bool JobControllerBES::CancelJobs(const std::list<Job*>& jobs, std::list<URL>& IDsProcessed, std::list<URL>& IDsNotProcessed, bool isGrouped) const {
    MCCConfig cfg;
    usercfg.ApplyToConfig(cfg);
    bool ok = true;
    for (std::list<Job*>::const_iterator it = jobs.begin(); it != jobs.end(); ++it) {
      Job& job = **it;
      AREXClient ac(job.Cluster, cfg, usercfg.Timeout(), false);
      if (!ac.kill(job.IDFromEndpoint)) {
        ok = false;
        IDsNotProcessed.push_back(job.JobID);
        continue;
      }
      
      IDsProcessed.push_back(job.JobID);
    }
    return ok;
  }

  bool JobControllerBES::RenewJobs(const std::list<Job*>& jobs, std::list<URL>&, std::list<URL>& IDsNotProcessed, bool) const {
    for (std::list<Job*>::const_iterator it = jobs.begin(); it != jobs.end(); ++it) {
      logger.msg(INFO, "Renewal of BES jobs is not supported");
      IDsNotProcessed.push_back((*it)->JobID);
    }
    return false;
  }

  bool JobControllerBES::ResumeJobs(const std::list<Job*>& jobs, std::list<URL>&, std::list<URL>& IDsNotProcessed, bool) const {
    for (std::list<Job*>::const_iterator it = jobs.begin(); it != jobs.end(); ++it) {
      logger.msg(INFO, "Resuming BES jobs is not supported");
      IDsNotProcessed.push_back((*it)->JobID);
    }
    return false;
  }

  bool JobControllerBES::GetJobDescription(const Job& job, std::string& desc_str) const {
    MCCConfig cfg;
    usercfg.ApplyToConfig(cfg);
    AREXClient ac(job.Cluster, cfg, usercfg.Timeout(), false);
    if (ac.getdesc(job.IDFromEndpoint, desc_str)) {
      std::list<JobDescription> descs;
      if (JobDescription::Parse(desc_str, descs) && !descs.empty()) {
        return true;
      }
    }

    logger.msg(ERROR, "Failed retrieving job description for job: %s", job.JobID.fullstr());
    return false;
  }

  URL JobControllerBES::CreateURL(std::string service, ServiceType /* st */) const {
    std::string::size_type pos1 = service.find("://");
    if (pos1 == std::string::npos)
      service = "https://" + service;
    // Default port other than 443?
    // Default path?
    return service;
  }

} // namespace Arc
