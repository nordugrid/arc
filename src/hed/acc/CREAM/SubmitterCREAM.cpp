// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/GUID.h>
#include <arc/message/MCC.h>
#include <arc/client/JobDescription.h>

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
    if (!PutFiles(job, jobInfo.ISB_URI)) {
      logger.msg(ERROR, "Failed uploading local input files");
      return URL();
    }
    if (!gLiteClientSubmission.startJob(jobInfo.jobId)) {
      logger.msg(ERROR, "Failed starting job");
      return URL();
    }

    AddJob(job, submissionurl.str() + '/' + jobInfo.jobId,
           delegationurl.str() + '/' + delegationid, joblistfile);

    return submissionurl.str() + '/' + jobInfo.jobId;
  }

  URL SubmitterCREAM::Migrate(const URL& jobid, const JobDescription& jobdesc,
                              bool forcemigration,
                              const std::string& joblistfile) const {
    logger.msg(ERROR, "Migration to a CREAM cluster is not supported.");
    return URL();
  }

} // namespace Arc
