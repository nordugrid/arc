// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib.h>

#include <arc/Logger.h>
#include <arc/URL.h>
#include <arc/message/MCC.h>

#include "CREAMClient.h"
#include "JobControllerCREAM.h"

namespace Arc {

  Logger JobControllerCREAM::logger(JobController::logger, "CREAM");

  JobControllerCREAM::JobControllerCREAM(Config *cfg)
    : JobController(cfg, "CREAM") {}

  JobControllerCREAM::~JobControllerCREAM() {}

  Plugin* JobControllerCREAM::Instance(PluginArgument *arg) {
    ACCPluginArgument *accarg =
      arg ? dynamic_cast<ACCPluginArgument*>(arg) : NULL;
    if (!accarg)
      return NULL;
    return new JobControllerCREAM((Config*)(*accarg));
  }

  void JobControllerCREAM::GetJobInformation() {
    MCCConfig cfg;
    ApplySecurity(cfg);
    for (std::list<Job>::iterator iter = jobstore.begin();
         iter != jobstore.end(); iter++) {
      PathIterator pi(iter->JobID.Path(), true);
      URL url(iter->JobID);
      url.ChangePath(*pi);
      CREAMClient gLiteClient(url, cfg);
      if (!gLiteClient.stat(pi.Rest(), (*iter)))
        logger.msg(ERROR, "Could not retrieve job information");
    }
  }

  bool JobControllerCREAM::GetJob(const Job& job,
                                  const std::string& downloaddir) {

    logger.msg(DEBUG, "Downloading job: %s", job.JobID.str());

    std::string path = job.JobID.Path();
    std::string::size_type pos = path.rfind('/');
    std::string jobidnum = path.substr(pos + 1);

    std::list<std::string> files = GetDownloadFiles(job.OSB);

    URL src(job.OSB);
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

  bool JobControllerCREAM::CleanJob(const Job& job, bool force) {

    MCCConfig cfg;
    ApplySecurity(cfg);
    PathIterator pi(job.JobID.Path(), true);
    URL url(job.JobID);
    url.ChangePath(*pi);
    CREAMClient gLiteClient(url, cfg);
    if (!gLiteClient.purge(pi.Rest())) {
      logger.msg(ERROR, "Failed to clean job");
      return false;
    }
    PathIterator pi2(job.AuxURL.Path(), true);
    URL url2(job.AuxURL);
    url2.ChangePath(*pi2);
    CREAMClient gLiteClient2(url2, cfg);
    if (!gLiteClient2.destroyDelegation(pi2.Rest())) {
      logger.msg(ERROR, "Destroying delegation failed");
      return false;
    }
    return true;
  }

  bool JobControllerCREAM::CancelJob(const Job& job) {

    MCCConfig cfg;
    ApplySecurity(cfg);
    PathIterator pi(job.JobID.Path(), true);
    URL url(job.JobID);
    url.ChangePath(*pi);
    CREAMClient gLiteClient(url, cfg);
    if (!gLiteClient.cancel(pi.Rest())) {
      logger.msg(ERROR, "Failed to cancel job");
      return false;
    }
    return true;
  }

  bool JobControllerCREAM::RenewJob(const Job& job){
    logger.msg(ERROR, "Renewal of CREAM jobs is not supported");
    return false;
  }

  bool JobControllerCREAM::ResumeJob(const Job& job){
    logger.msg(ERROR, "Resumation of CREAM jobs is not supported");
    return false;
  }

  URL JobControllerCREAM::GetFileUrlForJob(const Job& job,
                                           const std::string& whichfile) {}
  bool JobControllerCREAM::GetJobDescription(const Job& job, std::string& desc_str) {}

} // namespace Arc
