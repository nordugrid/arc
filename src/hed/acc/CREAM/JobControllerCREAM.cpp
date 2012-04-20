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
#include "JobControllerCREAM.h"

namespace Arc {

  Logger JobControllerCREAM::logger(Logger::getRootLogger(), "JobController.CREAM");

  bool JobControllerCREAM::isEndpointNotSupported(const std::string& endpoint) const {
    const std::string::size_type pos = endpoint.find("://");
    return pos != std::string::npos && lower(endpoint.substr(0, pos)) != "http" && lower(endpoint.substr(0, pos)) != "https";
  }

  void JobControllerCREAM::UpdateJobs(std::list<Job*>& jobs) const {
    MCCConfig cfg;
    usercfg.ApplyToConfig(cfg);
    for (std::list<Job*>::iterator iter = jobs.begin();
         iter != jobs.end(); ++iter) {
      URL url((*iter)->JobID);
      PathIterator pi(url.Path(), true);
      url.ChangePath(*pi);
      CREAMClient gLiteClient(url, cfg, usercfg.Timeout());
      if (!gLiteClient.stat(pi.Rest(), (**iter))) {
        logger.msg(WARNING, "Job information not found in the information system: %s", (*iter)->JobID.fullstr());
      }
    }
  }

  bool JobControllerCREAM::CleanJob(const Job& job) const {

    MCCConfig cfg;
    usercfg.ApplyToConfig(cfg);
    URL url(job.JobID);
    PathIterator pi(url.Path(), true);
    url.ChangePath(*pi);
    CREAMClient gLiteClient(url, cfg, usercfg.Timeout());
    if (!gLiteClient.purge(pi.Rest())) {
      logger.msg(INFO, "Failed cleaning job: %s", job.JobID.fullstr());
      return false;
    }
    
    creamJobInfo info;
    info = XMLNode(job.IDFromEndpoint);
    URL url2(info.delegationID);
    PathIterator pi2(url2.Path(), true);
    url2.ChangePath(*pi2);
    CREAMClient gLiteClient2(url2, cfg, usercfg.Timeout());
    if (!gLiteClient2.destroyDelegation(pi2.Rest())) {
      logger.msg(INFO, "Failed destroying delegation credentials for job: %s", job.JobID.fullstr());
      return false;
    }
    return true;
  }

  bool JobControllerCREAM::CancelJob(const Job& job) const {

    MCCConfig cfg;
    usercfg.ApplyToConfig(cfg);
    URL url(job.JobID);
    PathIterator pi(url.Path(), true);
    url.ChangePath(*pi);
    CREAMClient gLiteClient(url, cfg, usercfg.Timeout());
    if (!gLiteClient.cancel(pi.Rest())) {
      logger.msg(INFO, "Failed canceling job: %s", job.JobID.fullstr());
      return false;
    }
    return true;
  }

  bool JobControllerCREAM::RenewJob(const Job& /* job */) const {
    logger.msg(INFO, "Renewal of CREAM jobs is not supported");
    return false;
  }

  bool JobControllerCREAM::ResumeJob(const Job& /* job */) const {
    logger.msg(INFO, "Resumation of CREAM jobs is not supported");
    return false;
  }

  bool JobControllerCREAM::GetURLToJobResource(const Job& job, Job::ResourceType resource, URL& url) const {
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

  bool JobControllerCREAM::GetJobDescription(const Job& /* job */, std::string& /* desc_str */) const {
    return false;
  }

} // namespace Arc
