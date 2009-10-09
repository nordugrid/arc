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

  Logger JobControllerBES::logger(JobController::logger, "BES");

  static char hex_to_char(const char* v) {
    char r = 0;
    if((*v >= '0') && (*v <= '9')) { r |= *v  - '0'; }
    else if((*v >= 'a') && (*v <= 'f')) { r |= *v  - 'a' + 10; }
    else if((*v >= 'A') && (*v <= 'F')) { r |= *v  - 'A' + 10; }
    r <<= 4; ++v;
    if((*v >= '0') && (*v <= '9')) { r |= *v  - '0'; }
    else if((*v >= 'a') && (*v <= 'f')) { r |= *v  - 'a' + 10; }
    else if((*v >= 'A') && (*v <= 'F')) { r |= *v  - 'A' + 10; }
    return r;
  }

  static std::string extract_job_id(const URL& u) {
    std::string jobid = u.Path();
    if(jobid.empty()) return jobid;
    std::string::size_type p = jobid.find('#');
    if(p == std::string::npos) { p=0; } else { ++p; }
    std::string s;
    for(;p < jobid.length();p+=2) {
      char r = hex_to_char(jobid.c_str() + p);
      if(r == 0) { s.resize(0); break; }
      s += r;
    }
    return s;
  }

  JobControllerBES::JobControllerBES(const UserConfig& usercfg)
    : JobController(usercfg, "BES") {}

  JobControllerBES::~JobControllerBES() {}

  Plugin* JobControllerBES::Instance(PluginArgument *arg) {
    JobControllerPluginArgument *jcarg =
      dynamic_cast<JobControllerPluginArgument*>(arg);
    if (!jcarg)
      return NULL;
    return new JobControllerBES(*jcarg);
  }

  void JobControllerBES::GetJobInformation() {
    MCCConfig cfg;
    usercfg.ApplyToConfig(cfg);

    for (std::list<Job>::iterator iter = jobstore.begin();
         iter != jobstore.end(); iter++) {
      AREXClient ac(iter->Cluster, cfg, usercfg.Timeout(),false);
      std::string idstr = extract_job_id(iter->JobID);
      if (!ac.stat(idstr, *iter))
        logger.msg(ERROR, "Failed retrieving job status information");
    }
  }

  bool JobControllerBES::GetJob(const Job& job,
                                 const std::string& downloaddir) {
    logger.msg(ERROR, "Get for BES jobs is not supported");
    return false;
  }

  bool JobControllerBES::CleanJob(const Job& job, bool force) {
    logger.msg(ERROR, "Cleaning of BES jobs is not supported");
    return false;
  }

  bool JobControllerBES::CancelJob(const Job& job) {
    MCCConfig cfg;
    usercfg.ApplyToConfig(cfg);
    AREXClient ac(job.Cluster, cfg, usercfg.Timeout(), false);
    std::string idstr = extract_job_id(job.JobID);
    return ac.kill(idstr);
  }

  bool JobControllerBES::RenewJob(const Job& job) {
    logger.msg(ERROR, "Renewal of BES jobs is not supported");
    return false;
  }

  bool JobControllerBES::ResumeJob(const Job& job) {
    logger.msg(ERROR, "Resume of BES jobs is not supported");
    return false;
  }

  bool JobControllerBES::GetJobDescription(const Job& job, std::string& desc_str) {
    MCCConfig cfg;
    usercfg.ApplyToConfig(cfg);
    AREXClient ac(job.Cluster, cfg, usercfg.Timeout(), false);
    std::string idstr = extract_job_id(job.JobID);
    if (ac.getdesc(idstr, desc_str)) {
      JobDescription desc;
      desc.Parse(desc_str);
      if (desc)
        logger.msg(INFO, "Valid job description");
      return true;
    }
    else {
      logger.msg(ERROR, "No job description");
      return false;
    }
  }

  URL JobControllerBES::GetFileUrlForJob(const Job& job, const std::string& whichfile) {
    return URL();
  }

} // namespace Arc
