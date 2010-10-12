// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <unistd.h>
#include <glibmm.h>

#include <arc/Logger.h>
#include <arc/UserConfig.h>
#include <arc/client/ExecutionTarget.h>
#include <arc/client/JobDescription.h>

#include "SubmitterARC0.h"
#include "FTPControl.h"

namespace Arc {

  Logger SubmitterARC0::logger(Submitter::logger, "ARC0");

  SubmitterARC0::SubmitterARC0(const UserConfig& usercfg)
    : Submitter(usercfg, "ARC0") {}

  SubmitterARC0::~SubmitterARC0() {}

  Plugin* SubmitterARC0::Instance(PluginArgument *arg) {
    SubmitterPluginArgument *subarg =
      dynamic_cast<SubmitterPluginArgument*>(arg);
    if (!subarg)
      return NULL;
    return new SubmitterARC0(*subarg);
  }

  URL SubmitterARC0::Submit(const JobDescription& jobdesc,
                            const ExecutionTarget& et) const {

    FTPControl ctrl;

    if (!ctrl.Connect(et.url,
                      usercfg.ProxyPath(), usercfg.CertificatePath(),
                      usercfg.KeyPath(), usercfg.Timeout())) {
      logger.msg(INFO, "Submit: Failed to connect");
      return URL();
    }

    if (!ctrl.SendCommand("CWD " + et.url.Path(), usercfg.Timeout())) {
      logger.msg(INFO, "Submit: Failed sending CWD command");
      ctrl.Disconnect(usercfg.Timeout());
      return URL();
    }

    std::string response;

    if (!ctrl.SendCommand("CWD new", response, usercfg.Timeout())) {
      logger.msg(INFO, "Submit: Failed sending CWD new command");
      ctrl.Disconnect(usercfg.Timeout());
      return URL();
    }

    std::string::size_type pos2 = response.rfind('"');
    std::string::size_type pos1 = response.rfind('/', pos2 - 1);
    std::string jobnumber = response.substr(pos1 + 1, pos2 - pos1 - 1);

    JobDescription job(jobdesc);

    if (!ModifyJobDescription(job, et)) {
      logger.msg(INFO, "Submit: Failed to modify job description "
                        "to be sent to target.");
      ctrl.Disconnect(usercfg.Timeout());
      return URL();
    }

    std::string jobdescstring = job.UnParse("XRSL");

    if (!ctrl.SendData(jobdescstring, "job", usercfg.Timeout())) {
      logger.msg(INFO, "Submit: Failed sending job description");
      ctrl.Disconnect(usercfg.Timeout());
      return URL();
    }

    if (!ctrl.Disconnect(usercfg.Timeout())) {
      logger.msg(INFO, "Submit: Failed to disconnect after submission");
      return URL();
    }

    URL jobid(et.url);
    jobid.ChangePath(jobid.Path() + '/' + jobnumber);

    if (!PutFiles(job, jobid)) {
      logger.msg(INFO, "Submit: Failed uploading local input files");
      return URL();
    }

    // Prepare contact url for information about this job
    URL infoEndpoint(et.Cluster);
    infoEndpoint.ChangeLDAPFilter("(nordugrid-job-globalid=" +
                                  jobid.str() + ")");
    infoEndpoint.ChangeLDAPScope(URL::subtree);

    AddJob(job, jobid, et.Cluster, infoEndpoint);

    return jobid;
  }

  URL SubmitterARC0::Migrate(const URL& jobid, const JobDescription& jobdesc,
                             const ExecutionTarget& et, bool forcemigration) const {
    logger.msg(INFO, "Trying to migrate to %s: Migration to a ARC GM-powered resource is not supported.", et.url.str());
    return URL();
  }

  bool SubmitterARC0::ModifyJobDescription(JobDescription& jobdesc, const ExecutionTarget& et) const {
    if (jobdesc.XRSL_elements["clientxrsl"].empty())
      jobdesc.XRSL_elements["clientxrsl"] = jobdesc.UnParse("XRSL");

    // Check for identical file names.
    // Check if executable and input is contained in the file list.
    bool executableIsAdded(false), inputIsAdded(false), outputIsAdded(false), errorIsAdded(false), logDirIsAdded(false);
    for (std::list<FileType>::const_iterator it1 = jobdesc.DataStaging.File.begin();
         it1 != jobdesc.DataStaging.File.end(); it1++) {
      for (std::list<FileType>::const_iterator it2 = it1;
           it2 != jobdesc.DataStaging.File.end(); it2++) {
        if (it1 == it2) continue;

        if (it1->Name == it2->Name && ((!it1->Source.empty() && !it2->Source.empty()) ||
                                       (!it1->Target.empty() && !it2->Target.empty()))) {
          logger.msg(VERBOSE, "Two files have identical file name '%s'.", it1->Name);
          return false;
        }
      }

      executableIsAdded  |= (it1->Name == jobdesc.Application.Executable.Name);
      inputIsAdded       |= (it1->Name == jobdesc.Application.Input);
      outputIsAdded      |= (it1->Name == jobdesc.Application.Output);
      errorIsAdded       |= (it1->Name == jobdesc.Application.Error);
      logDirIsAdded      |= (it1->Name == jobdesc.Application.LogDir);
    }

    if (!executableIsAdded &&
        !Glib::path_is_absolute(jobdesc.Application.Executable.Name)) {
      FileType file;
      file.Name = jobdesc.Application.Executable.Name;
      DataSourceType s;
      s.URI = file.Name;
      file.Source.push_back(s);
      file.KeepData = false;
      file.IsExecutable = true;
      file.DownloadToCache = false;
      jobdesc.DataStaging.File.push_back(file);
    }

    if (!jobdesc.Application.Input.empty() && !inputIsAdded) {
      FileType file;
      file.Name = jobdesc.Application.Input;
      DataSourceType s;
      s.URI = file.Name;
      file.Source.push_back(s);
      file.KeepData = false;
      file.IsExecutable = false;
      file.DownloadToCache = false;
      jobdesc.DataStaging.File.push_back(file);
    }

    if (!jobdesc.Application.Output.empty() && !outputIsAdded) {
      FileType file;
      file.Name = jobdesc.Application.Output;
      file.KeepData = true;
      file.IsExecutable = false;
      file.DownloadToCache = false;
      jobdesc.DataStaging.File.push_back(file);
    }

    if (!jobdesc.Application.Error.empty() && !errorIsAdded) {
      FileType file;
      file.Name = jobdesc.Application.Error;
      file.KeepData = true;
      file.IsExecutable = false;
      file.DownloadToCache = false;
      jobdesc.DataStaging.File.push_back(file);
    }

    if (!jobdesc.Application.LogDir.empty() && !logDirIsAdded) {
      FileType file;
      file.Name = jobdesc.Application.LogDir;
      file.KeepData = true;
      file.IsExecutable = false;
      file.DownloadToCache = false;
      jobdesc.DataStaging.File.push_back(file);
    }

    if (!jobdesc.Resources.RunTimeEnvironment.empty() &&
        !jobdesc.Resources.RunTimeEnvironment.selectSoftware(et.ApplicationEnvironments)) {
      // This error should never happen since RTE is checked in the Broker.
      logger.msg(VERBOSE, "Unable to select runtime environment");
      return false;
    }

    if (!jobdesc.Resources.CEType.empty() &&
        !jobdesc.Resources.CEType.selectSoftware(et.Implementation)) {
      // This error should never happen since Middleware is checked in the Broker.
      logger.msg(VERBOSE, "Unable to select middleware");
      return false;
    }

    if (!jobdesc.Resources.OperatingSystem.empty() &&
        !jobdesc.Resources.OperatingSystem.selectSoftware(et.Implementation)) {
      // This error should never happen since OS is checked in the Broker.
      logger.msg(VERBOSE, "Unable to select operating system.");
      return false;
    }

    if (jobdesc.Resources.CandidateTarget.empty()) {
      ResourceTargetType candidateTarget;
      candidateTarget.EndPointURL = URL();
      candidateTarget.QueueName = et.ComputingShareName;
      jobdesc.Resources.CandidateTarget.push_back(candidateTarget);
    }
    else if (jobdesc.Resources.CandidateTarget.front().QueueName.empty())
      jobdesc.Resources.CandidateTarget.front().QueueName = et.ComputingShareName;

    jobdesc.XRSL_elements["action"] = "request";
    jobdesc.XRSL_elements["savestate"] = "yes";
    jobdesc.XRSL_elements["clientsoftware"] = "arclibclient-" VERSION;
#ifdef HAVE_GETHOSTNAME
    char hostname[1024];
    gethostname(hostname, 1024);
    jobdesc.XRSL_elements["hostname"] = hostname;
#endif

    jobdesc.AddHint("TARGETDIALECT","GRIDMANAGER");

    return true;
  }

} // namespace Arc
