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
#include "JobStateEMIES.h"
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
      if (!ac.info(job, *iter)) {
        logger.msg(WARNING, "Job information not found: %s", iter->JobID.fullstr());
      }
      // Going for more detailed state
      XMLNode jst;
      if (!ac.stat(job, jst)) {
      } else {
        JobStateEMIES jst_ = jst;
        if(jst_) iter->State = jst_;
      }
    }
  }

  bool JobControllerEMIES::GetJob(const Job& job,
                                 const std::string& downloaddir,
                                 bool usejobname,
                                 bool force) {
    logger.msg(VERBOSE, "Downloading job: %s", job.JobID.fullstr());

    std::string jobidnum;
    if (usejobname && !job.Name.empty()) {
      jobidnum = job.Name;
    } else {
      jobidnum = job.JobID.Option("emiesjobid");
    }

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
    MCCConfig cfg;
    usercfg.ApplyToConfig(cfg);

    // Obtain information about staging urls
    EMIESJob ejob = JobToEMIES(job);
    std::string stagein;
    std::string stageout;
    std::string session;
    Job tjob;
    EMIESClient ac(ejob.manager, cfg, usercfg.Timeout());
    if (!ac.info(ejob, tjob, stagein, stageout, session)) {
      logger.msg(INFO, "Failed retrieving information for job: %s", job.JobID.fullstr());
      return URL();
    }
    URL url;
    // Choose url by state
    // TODO: maybe this method should somehow know what is purpose of URL
    // TODO: state attributes woul dbe more suitable
    if((tjob.State == JobState::ACCEPTED) ||
       (tjob.State == JobState::PREPARING)) {
      url = stagein;
    } else if((tjob.State == JobState::DELETED) ||
              (tjob.State == JobState::FAILED) ||
              (tjob.State == JobState::KILLED) ||
              (tjob.State == JobState::FINISHED) ||
              (tjob.State == JobState::FINISHING)) {
      url = stageout;
    } else {
      url = session;
    }
    // If no url found by state still try to get something
    if(!url) if(!session.empty()) url = session;
    if(!url) if(!stagein.empty()) url = stagein;
    if(!url) if(!stageout.empty()) url = stageout;
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
