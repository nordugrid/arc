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

  bool JobControllerEMIES::isEndpointNotSupported(const std::string& endpoint) const {
    const std::string::size_type pos = endpoint.find("://");
    return pos != std::string::npos && lower(endpoint.substr(0, pos)) != "http" && lower(endpoint.substr(0, pos)) != "https";
  }
  
  static EMIESJob JobToEMIES(const Job& ajob) {
    EMIESJob job;
    job.id = ajob.JobID.Option("emiesjobid");
    job.manager = ajob.JobID;
    job.manager.RemoveOption("emiesjobid");
    return job;
  }

  void JobControllerEMIES::UpdateJobs(std::list<Job*>& jobs) const {
    MCCConfig cfg;
    usercfg.ApplyToConfig(cfg);

    for (std::list<Job*>::iterator iter = jobs.begin();
         iter != jobs.end(); iter++) {
      EMIESJob job = JobToEMIES(**iter);
      EMIESClient ac(job.manager, cfg, usercfg.Timeout());
      if (!ac.info(job, **iter)) {
        logger.msg(WARNING, "Job information not found in the information system: %s", (*iter)->JobID.fullstr());
      }
      // Going for more detailed state
      XMLNode jst;
      if (!ac.stat(job, jst)) {
      } else {
        JobStateEMIES jst_ = jst;
        if(jst_) (*iter)->State = jst_;
      }
    }
  }

  bool JobControllerEMIES::RetrieveJob(const Job& job,
                                       std::string& downloaddir,
                                       bool usejobname,
                                       bool force) const {
    logger.msg(VERBOSE, "Downloading job: %s", job.JobID.fullstr());

    if (!downloaddir.empty()) {
      downloaddir += G_DIR_SEPARATOR_S;
    }
    if (usejobname && !job.Name.empty()) {
      downloaddir += job.Name;
    } else {
      downloaddir += job.JobID.Option("emiesjobid");
    }

    URL src(GetFileUrlForJob(job,""));
    URL dst(downloaddir);
    std::list<std::string> files;
    if (!ListFilesRecursive(src, files)) {
      logger.msg(ERROR, "Unable to retrieve list of job files to download for job %s", job.JobID.fullstr());
      return false;
    }

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
      if (!CopyJobFile(src, dst)) {
        logger.msg(INFO, "Failed dowloading %s to %s", src.str(), dst.str());
        ok = false;
      }
    }

    return ok;
  }

  bool JobControllerEMIES::CleanJob(const Job& job) const {
    MCCConfig cfg;
    usercfg.ApplyToConfig(cfg);

    EMIESJob ejob = JobToEMIES(job);
    EMIESClient ac(ejob.manager, cfg, usercfg.Timeout());
    return ac.clean(ejob);
  }

  bool JobControllerEMIES::CancelJob(const Job& job) const {
    logger.msg(INFO, "Cancel of EMI ES jobs is not supported");
    return false;
  }

  bool JobControllerEMIES::RenewJob(const Job& /* job */) const {
    logger.msg(INFO, "Renewal of EMI ES jobs is not supported");
    return false;
  }

  bool JobControllerEMIES::ResumeJob(const Job& job) const {
    logger.msg(INFO, "Resume of EMI ES jobs is not supported");
    return false;
  }

  URL JobControllerEMIES::GetFileUrlForJob(const Job& job,
                                          const std::string& whichfile) const {
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

  bool JobControllerEMIES::GetJobDescription(const Job& /* job */, std::string& /* desc_str */) const {
    logger.msg(INFO, "Retrieving job description of EMI ES jobs is not supported");
    return false;
  }

  URL JobControllerEMIES::CreateURL(std::string service, ServiceType /* st */) const {
    std::string::size_type pos1 = service.find("://");
    if (pos1 == std::string::npos)
      service = "https://" + service;
    return service;
  }

} // namespace Arc
