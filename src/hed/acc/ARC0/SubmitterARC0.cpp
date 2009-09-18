// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <unistd.h>

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

    Config cfg;
    usercfg.ApplyToConfig(cfg);

    if (!ctrl.Connect(et.url,
                      cfg["ProxyPath"], cfg["CertificatePath"],
                      cfg["KeyPath"], 500)) {
      logger.msg(ERROR, "Submit: Failed to connect");
      return URL();
    }

    if (!ctrl.SendCommand("CWD " + et.url.Path(), 500)) {
      logger.msg(ERROR, "Submit: Failed sending CWD command");
      ctrl.Disconnect(500);
      return URL();
    }

    std::string response;

    if (!ctrl.SendCommand("CWD new", response, 500)) {
      logger.msg(ERROR, "Submit: Failed sending CWD new command");
      ctrl.Disconnect(500);
      return URL();
    }

    std::string::size_type pos2 = response.rfind('"');
    std::string::size_type pos1 = response.rfind('/', pos2 - 1);
    std::string jobnumber = response.substr(pos1 + 1, pos2 - pos1 - 1);

    JobDescription job(jobdesc);

    if (!ModifyJobDescription(job, et)) {
      logger.msg(ERROR, "Submit: Failed to modify job description "
                        "to be sent to target.");
      ctrl.Disconnect(500);
      return URL();
    }

    std::string jobdescstring = job.UnParse("XRSL");

    if (!ctrl.SendData(jobdescstring, "job", 500)) {
      logger.msg(ERROR, "Submit: Failed sending job description");
      ctrl.Disconnect(500);
      return URL();
    }

    if (!ctrl.Disconnect(500)) {
      logger.msg(ERROR, "Submit: Failed to disconnect after submission");
      return URL();
    }

    URL jobid(et.url);
    jobid.ChangePath(jobid.Path() + '/' + jobnumber);

    if (!PutFiles(job, jobid)) {
      logger.msg(ERROR, "Submit: Failed uploading local input files");
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
    logger.msg(ERROR, "Migration to a ARC0 cluster is not supported.");
    return URL();
  }
  
  bool SubmitterARC0::ModifyJobDescription(JobDescription& jobdesc, const ExecutionTarget& et) const {
    jobdesc.XRSL_elements["clientxrsl"] = jobdesc.UnParse("XRSL");

    // Check for identical file names.
    // Check if executable and input is contained in the file list.
    bool inputIsAdded(false), executableIsAdded(false);
    for (std::list<FileType>::const_iterator it1 = jobdesc.DataStaging.File.begin();
         it1 != jobdesc.DataStaging.File.end(); it1++) {
      for (std::list<FileType>::const_iterator it2 = it1;
           it2 != jobdesc.DataStaging.File.end(); it2++) {
        if (it1 == it2) continue;

        if (it1->Name == it2->Name && (!it1->Source.empty() && !it2->Source.empty() ||
                                       !it1->Target.empty() && !it2->Target.empty())) {
          logger.msg(DEBUG, "Two files have identical file name '%s'.", it1->Name);
          return false;
        }
      }

      if (it1->Source.empty()) continue;
      
      executableIsAdded |= (it1->Name == jobdesc.Application.Executable.Name);
      inputIsAdded      |= (it1->Name == jobdesc.Application.Input);
    }

    if (!jobdesc.Application.Executable.Name.empty() && !executableIsAdded) {
      FileType executable;
      executable.Name = jobdesc.Application.Executable.Name;
      DataSourceType source;
      source.URI = executable.Name;
      source.Threads = -1;
      executable.Source.push_back(source);
      executable.KeepData = false;
      executable.IsExecutable = true;
      executable.DownloadToCache = false;
      jobdesc.DataStaging.File.push_back(executable);
    }

    if (!jobdesc.Application.Input.empty() && !inputIsAdded) {
      FileType input;
      input.Name = jobdesc.Application.Input;
      DataSourceType source;
      source.URI = input.Name;
      source.Threads = -1;
      input.Source.push_back(source);
      input.KeepData = false;
      input.IsExecutable = false;
      input.DownloadToCache = false;
      jobdesc.DataStaging.File.push_back(input);
    }

    if (!jobdesc.Resources.RunTimeEnvironment.empty() &&
        !jobdesc.Resources.RunTimeEnvironment.selectSoftware(et.ApplicationEnvironments)) {
      // This error should never happen since RTE is checked in the Broker.
      logger.msg(DEBUG, "Unable to select run time environment");
      return false;
    }

    if (!jobdesc.Resources.CEType.empty() &&
        !jobdesc.Resources.CEType.selectSoftware(et.Implementation)) {
      // This error should never happen since Middleware is checked in the Broker.
      logger.msg(DEBUG, "Unable to select middleware");
      return false;
    }

    if (!jobdesc.Resources.OperatingSystem.empty() &&
        !jobdesc.Resources.OperatingSystem.selectSoftware(et.Implementation)) {
      // This error should never happen since OS is checked in the Broker.
      logger.msg(DEBUG, "Unable to select operating system.");
      return false;
    }

    if (jobdesc.Resources.CandidateTarget.empty()) {
      ResourceTargetType candidateTarget;
      candidateTarget.EndPointURL = URL();
      candidateTarget.QueueName = et.MappingQueue;
      jobdesc.Resources.CandidateTarget.push_back(candidateTarget);
    }
    else if (jobdesc.Resources.CandidateTarget.front().QueueName.empty())
      jobdesc.Resources.CandidateTarget.front().QueueName = et.MappingQueue;

    jobdesc.XRSL_elements["action"] = "request";
    jobdesc.XRSL_elements["savestate"] = "yes";
    jobdesc.XRSL_elements["clientsoftware"] = "arclibclient-" VERSION;
#ifdef HAVE_GETHOSTNAME
    char hostname[1024];
    gethostname(hostname, 1024);
    jobdesc.XRSL_elements["hostname"] = hostname;
#endif

    return true;
  }

} // namespace Arc
