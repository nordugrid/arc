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
#include "JobControllerARC1.h"

namespace Arc {

  Logger JobControllerARC1::logger(JobController::logger, "ARC1");

  JobControllerARC1::JobControllerARC1(const UserConfig& usercfg)
    : JobController(usercfg, "ARC1") {}

  JobControllerARC1::~JobControllerARC1() {}

  Plugin* JobControllerARC1::Instance(PluginArgument *arg) {
    JobControllerPluginArgument *jcarg =
      dynamic_cast<JobControllerPluginArgument*>(arg);
    if (!jcarg)
      return NULL;
    return new JobControllerARC1(*jcarg);
  }

  void JobControllerARC1::GetJobInformation() {
    MCCConfig cfg;
    usercfg.ApplyToConfig(cfg);

    for (std::list<Job>::iterator iter = jobstore.begin();
         iter != jobstore.end(); iter++) {
      AREXClient ac(iter->Cluster, cfg, usercfg.Timeout());
      std::string idstr;
      AREXClient::createActivityIdentifier(iter->JobID, idstr);
      if (!ac.stat(idstr, *iter))
        logger.msg(ERROR, "Failed retrieving job status information");
    }
  }

  bool JobControllerARC1::GetJob(const Job& job,
                                 const std::string& downloaddir) {

    logger.msg(VERBOSE, "Downloading job: %s", job.JobID.str());

    std::string path = job.JobID.Path();
    std::string::size_type pos = path.rfind('/');
    std::string jobidnum = path.substr(pos + 1);

    std::list<std::string> files = GetDownloadFiles(job.JobID);

    URL src(job.JobID);
    URL dst(downloaddir.empty() ? jobidnum : downloaddir + G_DIR_SEPARATOR_S + jobidnum);

    std::string srcpath = src.Path();
    std::string dstpath = dst.Path();

    if (srcpath.empty() || (srcpath[srcpath.size() - 1] != '/'))
      srcpath += '/';
    if (dstpath.empty() || (dstpath[dstpath.size() - 1] != G_DIR_SEPARATOR))
      dstpath += G_DIR_SEPARATOR_S;

    bool ok = true;

    for (std::list<std::string>::iterator it = files.begin();
         it != files.end(); it++) {
      src.ChangePath(srcpath + *it);
      dst.ChangePath(dstpath + *it);
      if (!ARCCopyFile(src, dst)) {
        logger.msg(ERROR, "Failed dowloading %s to %s", src.str(), dst.str());
        ok = false;
      }
    }

    return ok;
  }

  bool JobControllerARC1::CleanJob(const Job& job, bool force) {
    MCCConfig cfg;
    usercfg.ApplyToConfig(cfg);
    AREXClient ac(job.Cluster, cfg, usercfg.Timeout());
    std::string idstr;
    AREXClient::createActivityIdentifier(job.JobID, idstr);
    return ac.clean(idstr);
  }

  bool JobControllerARC1::CancelJob(const Job& job) {
    MCCConfig cfg;
    usercfg.ApplyToConfig(cfg);
    AREXClient ac(job.Cluster, cfg, usercfg.Timeout());
    std::string idstr;
    AREXClient::createActivityIdentifier(job.JobID, idstr);
    return ac.kill(idstr);
  }

  bool JobControllerARC1::RenewJob(const Job& job) {
    logger.msg(ERROR, "Renewal of ARC1 jobs is not supported");
    return false;
  }

  bool JobControllerARC1::ResumeJob(const Job& job) {

    if (job.RestartState.empty())
      logger.msg(ERROR, "Job %s does not report a resumable state", job.JobID.str());
      //return false;

    logger.msg(VERBOSE, "Resuming job: %s at state: %s", job.JobID.str(), job.RestartState);

    MCCConfig cfg;
    usercfg.ApplyToConfig(cfg);
    AREXClient ac(job.Cluster, cfg, usercfg.Timeout());
    std::string idstr;
    AREXClient::createActivityIdentifier(job.JobID, idstr);
    bool ok = ac.resume(idstr);
    if (ok)
      logger.msg(VERBOSE, "Job resuming successful");
    return ok;
  }

  URL JobControllerARC1::GetFileUrlForJob(const Job& job,
                                          const std::string& whichfile) {
    URL url(job.JobID);

    if (whichfile == "stdout")
      url.ChangePath(url.Path() + '/' + job.StdOut);
    else if (whichfile == "stderr")
      url.ChangePath(url.Path() + '/' + job.StdErr);
    else if (whichfile == "gmlog")
      url.ChangePath(url.Path() + "/" + job.LogDir + "/errors");

    return url;
  }

  bool JobControllerARC1::GetJobDescription(const Job& job, std::string& desc_str) {
    MCCConfig cfg;
    usercfg.ApplyToConfig(cfg);
    AREXClient ac(job.Cluster, cfg, usercfg.Timeout());
    std::string idstr;
    AREXClient::createActivityIdentifier(job.JobID, idstr);
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
} // namespace Arc
