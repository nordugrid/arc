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

#include "EMIESClient.h"
#include "JobControllerEMIES.h"

namespace Arc {

  Logger JobControllerEMIES::logger(Logger::getRootLogger(), "JobController.EMIES");

  JobControllerEMIES::JobControllerEMIES(const UserConfig& usercfg)
    : JobController(usercfg, "EMIES") {}

  JobControllerEMIES::~JobControllerEMIES() {}

  Plugin* JobControllerEMIES::Instance(PluginArgument *arg) {
    JobControllerPluginArgument *jcarg =
      dynamic_cast<JobControllerPluginArgument*>(arg);
    if (!jcarg) return NULL;
    return new JobControllerEMIES(*jcarg);
  }

  static EMIESJob JobToEMIES(const Job& ajob) {
    EMIESJob job;
    job.id = ajob.JobID.Option("emiesjobid");
    job.manager = ajob.JobID;
    job.manager.RemoveOption("emiesjobid");
    return job;
  }

  void JobControllerEMIES::GetJobInformation() {
    MCCConfig cfg;
    usercfg.ApplyToConfig(cfg);

    for (std::list<Job>::iterator iter = jobstore.begin();
         iter != jobstore.end(); iter++) {
      EMIESJob job = JobToEMIES(*iter);
      EMIESClient ac(job.manager, cfg, usercfg.Timeout());
      if (!ac.stat(job, *iter)) {
        logger.msg(INFO, "Failed retrieving information for job: %s", iter->JobID.str());
      }
    }
  }

  bool JobControllerEMIES::GetJob(const Job& job,
                                 const std::string& downloaddir,
                                 const bool usejobname,
                                 const bool force) {
    logger.msg(VERBOSE, "Downloading job: %s", job.JobID.str());

    std::string jobidnum;
    if (usejobname && !job.Name.empty()) {
      jobidnum = job.Name;
    } else {
      jobidnum = job.JobID.Option("emiesjobid");
    }

    // TODO: This only works for ARC implementation
    URL src(GetFileUrlForJob(job,""));
    URL dst(downloaddir.empty() ? jobidnum : downloaddir + G_DIR_SEPARATOR_S + jobidnum);
    std::list<std::string> files = GetDownloadFiles(src);

    std::string srcpath = src.Path();
    std::string dstpath = dst.Path();

    if (!force && Glib::file_test(dstpath, Glib::FILE_TEST_EXISTS))
    {
      logger.msg(INFO, "%s directory exist! This job downloaded previously.", dstpath);
      return true;
    }

    if (srcpath.empty() || (srcpath[srcpath.size() - 1] != '/')) {
      srcpath += '/';
    }
    if (dstpath.empty() || (dstpath[dstpath.size() - 1] != G_DIR_SEPARATOR)) {
      dstpath += G_DIR_SEPARATOR_S;
    }

    bool ok = true;

    for (std::list<std::string>::iterator it = files.begin();
         it != files.end(); it++) {
      src.ChangePath(srcpath + *it);
      dst.ChangePath(dstpath + *it);
      if (!ARCCopyFile(src, dst)) {
        logger.msg(INFO, "Failed dowloading %s to %s", src.str(), dst.str());
        ok = false;
      }
    }

    return ok;
  }

  bool JobControllerEMIES::CleanJob(const Job& job) {
    MCCConfig cfg;
    usercfg.ApplyToConfig(cfg);

    EMIESJob ejob = JobToEMIES(job);
    EMIESClient ac(ejob.manager, cfg, usercfg.Timeout());
    return ac.clean(ejob);
  }

  bool JobControllerEMIES::CancelJob(const Job& job) {
    logger.msg(INFO, "Cancel of EMI ES jobs is not supported");
    return false;
  }

  bool JobControllerEMIES::RenewJob(const Job& /* job */) {
    logger.msg(INFO, "Renewal of EMI ES jobs is not supported");
    return false;
  }

  bool JobControllerEMIES::ResumeJob(const Job& job) {
    logger.msg(INFO, "Resume of EMI ES jobs is not supported");
    return false;
  }

  URL JobControllerEMIES::GetFileUrlForJob(const Job& job,
                                          const std::string& whichfile) {
    // TODO: Folowing only works for ARC implementation
    URL url(job.JobID);
    url.ChangePath(url.Path() + '/' + url.Option("emiesjobid"));
    url.RemoveOption("emiesjobid");

    if (whichfile == "stdout") {
      url.ChangePath(url.Path() + '/' + job.StdOut);
    } else if (whichfile == "stderr") {
      url.ChangePath(url.Path() + '/' + job.StdErr);
    } else if (whichfile == "joblog") {
      url.ChangePath(url.Path() + "/" + job.LogDir + "/errors");
    } else {
      if(!whichfile.empty()) url.ChangePath(url.Path() + "/" + whichfile);
    }

    return url;
  }

  bool JobControllerEMIES::GetJobDescription(const Job& /* job */, std::string& /* desc_str */) {
    logger.msg(INFO, "Retrieving job description of EMI ES jobs is not supported");
    return false;
  }

  URL JobControllerEMIES::CreateURL(std::string service, ServiceType /* st */) {
    std::string::size_type pos1 = service.find("://");
    if (pos1 == std::string::npos)
      service = "https://" + service;
    return service;
  }

} // namespace Arc
