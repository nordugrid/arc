// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/FileLock.h>
#include <arc/Logger.h>
#include <arc/client/JobDescription.h>
#include <arc/client/Sandbox.h>

#include "SubmitterARC0.h"
#include "FTPControl.h"

namespace Arc {

  Logger SubmitterARC0::logger(Submitter::logger, "ARC0");

  SubmitterARC0::SubmitterARC0(Config *cfg)
    : Submitter(cfg, "ARC0") {
    queue = (std::string)(*cfg)["Queue"];
  }

  SubmitterARC0::~SubmitterARC0() {}

  Plugin* SubmitterARC0::Instance(PluginArgument *arg) {
    ACCPluginArgument *accarg = dynamic_cast<ACCPluginArgument*>(arg);
    if (!accarg)
      return NULL;
    return new SubmitterARC0((Config*)(*accarg));
  }

  URL SubmitterARC0::Submit(const JobDescription& jobdesc,
                            const std::string& joblistfile) const {

    FTPControl ctrl;

    if (!ctrl.Connect(submissionEndpoint,
                      proxyPath, certificatePath, keyPath, 500)) {
      logger.msg(ERROR, "Submit: Failed to connect");
      return URL();
    }

    if (!ctrl.SendCommand("CWD " + submissionEndpoint.Path(), 500)) {
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
    if (job.Resources.CandidateTarget.empty()) {
      ResourceTargetType candidateTarget;
      candidateTarget.EndPointURL = URL();
      candidateTarget.QueueName = queue;
      job.Resources.CandidateTarget.push_back(candidateTarget);
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

    URL jobid(submissionEndpoint);
    jobid.ChangePath(jobid.Path() + '/' + jobnumber);

    if (!PutFiles(jobdesc, jobid)) {
      logger.msg(ERROR, "Submit: Failed uploading local input files");
      return URL();
    }

    // Prepare contact url for information about this job
    URL infoEndpoint(cluster);
    infoEndpoint.ChangeLDAPFilter("(nordugrid-job-globalid=" +
                                  jobid.str() + ")");
    infoEndpoint.ChangeLDAPScope(URL::subtree);

    Arc::NS ns;
    Arc::XMLNode info(ns, "Job");
    info.NewChild("JobID") = jobid.str();
    if (!jobdesc.Identification.JobName.empty())
      info.NewChild("Name") = jobdesc.Identification.JobName;
    info.NewChild("Flavour") = flavour;
    info.NewChild("Cluster") = cluster.str();
    info.NewChild("InfoEndpoint") = infoEndpoint.str();
    info.NewChild("LocalSubmissionTime") = (std::string)Arc::Time();
    Sandbox::Add(jobdesc, info);

    FileLock lock(joblistfile);
    Config jobs;
    jobs.ReadFromFile(joblistfile);
    jobs.NewChild(info);
    jobs.SaveToFile(joblistfile);

    return jobid;
  }

  URL SubmitterARC0::Migrate(const URL& jobid, const JobDescription& jobdesc,
                             bool forcemigration,
                             const std::string& joblistfile) const {
    logger.msg(ERROR, "Migration to a ARC0 cluster is not supported.");
    return URL();
  }

} // namespace Arc
