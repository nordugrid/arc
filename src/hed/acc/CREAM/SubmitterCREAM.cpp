// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/FileLock.h>
#include <arc/GUID.h>
#include <arc/message/MCC.h>
#include <arc/client/JobDescription.h>
#include <arc/client/Sandbox.h>

#include "CREAMClient.h"
#include "SubmitterCREAM.h"

namespace Arc {

  SubmitterCREAM::SubmitterCREAM(Config *cfg)
    : Submitter(cfg, "CREAM") {
    queue = (std::string)(*cfg)["Queue"];
    if ((*cfg)["LRMSType"])
      lrmsType = (std::string)(*cfg)["LRMSType"];
  }

  SubmitterCREAM::~SubmitterCREAM() {}

  Plugin* SubmitterCREAM::Instance(PluginArgument *arg) {
    ACCPluginArgument *accarg =
      arg ? dynamic_cast<ACCPluginArgument*>(arg) : NULL;
    if (!accarg)
      return NULL;
    return new SubmitterCREAM((Config*)(*accarg));
  }

  URL SubmitterCREAM::Submit(const JobDescription& jobdesc,
                             const std::string& joblistfile) const {
    MCCConfig cfg;
    ApplySecurity(cfg);
    std::string delegationid = UUID();
    URL delegationurl(submissionEndpoint);
    delegationurl.ChangePath(delegationurl.Path() + "/gridsite-delegation");
    CREAMClient gLiteClientDelegation(delegationurl, cfg);
    if (!gLiteClientDelegation.createDelegation(delegationid, proxyPath)) {
      logger.msg(ERROR, "Creating delegation failed");
      return URL();
    }
    URL submissionurl(submissionEndpoint);
    submissionurl.ChangePath(submissionurl.Path() + "/CREAM2");
    CREAMClient gLiteClientSubmission(submissionurl, cfg);
    gLiteClientSubmission.setDelegationId(delegationid);

    JobDescription job(jobdesc);
    if (job.JDL_elements.find("BatchSystem") == job.JDL_elements.end() &&
        !lrmsType.empty())
      job.JDL_elements["BatchSystem"] = lrmsType;

    if (job.Resources.CandidateTarget.empty()) {
      ResourceTargetType candidateTarget;
      candidateTarget.EndPointURL = URL();
      candidateTarget.QueueName = queue;
      job.Resources.CandidateTarget.push_back(candidateTarget);
    }

    std::string jobdescstring = job.UnParse("JDL");
    creamJobInfo jobInfo;
    if (!gLiteClientSubmission.registerJob(jobdescstring, jobInfo)) {
      logger.msg(ERROR, "Job registration failed");
      return URL();
    }
    if (!PutFiles(jobdesc, jobInfo.ISB_URI)) {
      logger.msg(ERROR, "Failed uploading local input files");
      return URL();
    }
    if (!gLiteClientSubmission.startJob(jobInfo.jobId)) {
      logger.msg(ERROR, "Failed starting job");
      return URL();
    }

    Arc::NS ns;
    Arc::XMLNode info(ns, "Job");
    info.NewChild("JobID") = submissionurl.str() + '/' + jobInfo.jobId;
    if (!jobdesc.Identification.JobName.empty())
      info.NewChild("Name") = jobdesc.Identification.JobName;
    info.NewChild("Flavour") = flavour;
    info.NewChild("Cluster") = cluster.str();
    info.NewChild("LocalSubmissionTime") = (std::string)Arc::Time();
    info.NewChild("ISB") = jobInfo.ISB_URI;
    info.NewChild("OSB") = jobInfo.OSB_URI;
    info.NewChild("AuxURL") = delegationurl.str() + '/' + delegationid;
    Sandbox::Add(jobdesc, info);

    FileLock lock(joblistfile);
    Config jobs;
    jobs.ReadFromFile(joblistfile);
    jobs.NewChild(info);
    jobs.SaveToFile(joblistfile);

    return submissionurl.str() + '/' + jobInfo.jobId;
  }

  URL SubmitterCREAM::Migrate(const URL& jobid, const JobDescription& jobdesc,
                              bool forcemigration,
                              const std::string& joblistfile) const {
    logger.msg(ERROR, "Migration to a CREAM cluster is not supported.");
    return URL();
  }

} // namespace Arc
