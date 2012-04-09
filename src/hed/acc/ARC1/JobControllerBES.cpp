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

  void JobControllerBES::UpdateJobs(std::list<Job*>& jobs) const {
    MCCConfig cfg;
    usercfg.ApplyToConfig(cfg);

    for (std::list<Job*>::iterator iter = jobs.begin();
         iter != jobs.end(); iter++) {
      AREXClient ac((*iter)->Cluster, cfg, usercfg.Timeout(),false);
      if (!ac.stat((*iter)->IDFromEndpoint, **iter)) {
        logger.msg(INFO, "Failed retrieving job status information");
      }
    }
  }

  bool JobControllerBES::RetrieveJob(const Job& /* job */,
                                        std::string& /* downloaddir */,
                                        bool /* usejobname */,
                                        bool /*force*/) const {
    logger.msg(INFO, "Getting BES jobs is not supported");
    return false;
  }

  bool JobControllerBES::CleanJob(const Job& /* job */) const {
    logger.msg(INFO, "Cleaning of BES jobs is not supported");
    return false;
  }

  bool JobControllerBES::CancelJob(const Job& job) const {
    MCCConfig cfg;
    usercfg.ApplyToConfig(cfg);
    AREXClient ac(job.Cluster, cfg, usercfg.Timeout(), false);
    return ac.kill(job.IDFromEndpoint);
  }

  bool JobControllerBES::RenewJob(const Job& /* job */) const {
    logger.msg(INFO, "Renewal of BES jobs is not supported");
    return false;
  }

  bool JobControllerBES::ResumeJob(const Job& /* job */) const {
    logger.msg(INFO, "Resuming BES jobs is not supported");
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

  URL JobControllerBES::GetFileUrlForJob(const Job& /* job */, const std::string& /* whichfile */) const {
    return URL();
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
