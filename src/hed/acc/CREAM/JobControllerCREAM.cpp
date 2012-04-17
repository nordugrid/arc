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

  bool JobControllerCREAM::RetrieveJob(const Job& job,
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
      std::string path = job.JobID.Path();
      std::string::size_type pos = path.rfind('/');
      downloaddir += path.substr(pos + 1);
    }
    creamJobInfo info;
    info = XMLNode(job.IDFromEndpoint);

    std::list<std::string> files;
    if (!Job::ListFilesRecursive(usercfg, info.OSB, files)) {
      logger.msg(ERROR, "Unable to retrieve list of job files to download for job %s", job.JobID.fullstr());
      return false;
    }

    URL src(info.OSB);
    URL dst(downloaddir);

    std::string srcpath = src.Path();
    std::string dstpath = dst.Path();

    if (!force && Glib::file_test(dstpath, Glib::FILE_TEST_EXISTS)) {
      logger.msg(WARNING, "%s directory exist! Skipping job.", dstpath);
      return false;
    }

    if (srcpath.empty() || (srcpath[srcpath.size() - 1] != '/'))
      srcpath += '/';
    if (dstpath.empty() || (dstpath[dstpath.size() - 1] != G_DIR_SEPARATOR))
      dstpath += G_DIR_SEPARATOR_S;

    bool ok = true;

    for (std::list<std::string>::iterator it = files.begin();
         it != files.end(); it++) {
      src.ChangePath(srcpath + *it);
      dst.ChangePath(dstpath + *it);
      if (!Job::CopyJobFile(usercfg, src, dst)) {
        logger.msg(INFO, "Failed dowloading %s to %s", src.str(), dst.str());
        ok = false;
      }
    }

    return ok;
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

  URL JobControllerCREAM::GetFileUrlForJob(const Job& /* job */,
                                           const std::string& /* whichfile */) const {
    return URL();
  }

  bool JobControllerCREAM::GetJobDescription(const Job& /* job */, std::string& /* desc_str */) const {
    return false;
  }

} // namespace Arc
