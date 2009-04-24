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
    : Submitter(cfg, "CREAM") {}

  SubmitterCREAM::~SubmitterCREAM() {}

  Plugin* SubmitterCREAM::Instance(PluginArgument *arg) {
    ACCPluginArgument *accarg =
      arg ? dynamic_cast<ACCPluginArgument*>(arg) : NULL;
    if (!accarg)
      return NULL;
    return new SubmitterCREAM((Config*)(*accarg));
  }

  bool SubmitterCREAM::Submit(const JobDescription& jobdesc, XMLNode& info) const {
    MCCConfig cfg;
    ApplySecurity(cfg);
    std::string delegationid = UUID();
    URL delegationurl(submissionEndpoint);
    delegationurl.ChangePath(delegationurl.Path() + "/gridsite-delegation");
    CREAMClient gLiteClientDelegation(delegationurl, cfg);
    if (!gLiteClientDelegation.createDelegation(delegationid, proxyPath)) {
      logger.msg(ERROR, "Creating delegation failed");
      return false;
    }
    URL submissionurl(submissionEndpoint);
    submissionurl.ChangePath(submissionurl.Path() + "/CREAM2");
    CREAMClient gLiteClientSubmission(submissionurl, cfg);
    gLiteClientSubmission.setDelegationId(delegationid);
    std::string jobdescstring = jobdesc.UnParse("JDL");
    creamJobInfo jobInfo;
    if (!gLiteClientSubmission.registerJob(jobdescstring, jobInfo)) {
      logger.msg(ERROR, "Job registration failed");
      return false;
    }
    if (!PutFiles(jobdesc, jobInfo.ISB_URI)) {
      logger.msg(ERROR, "Failed uploading local input files");
      return false;
    }
    if (!gLiteClientSubmission.startJob(jobInfo.jobId)) {
      logger.msg(ERROR, "Failed starting job");
      return false;
    }

    info.NewChild("JobID") = submissionurl.str() + '/' + jobInfo.jobId;
    info.NewChild("ISB") = jobInfo.ISB_URI;
    info.NewChild("OSB") = jobInfo.OSB_URI;
    info.NewChild("AuxURL") = delegationurl.str() + '/' + delegationid;

    return true;
  }

  bool SubmitterCREAM::Migrate(const URL& jobid, const JobDescription& jobdesc, bool forcemigration, XMLNode& info) const {
    logger.msg(ERROR, "Migration to a CREAM cluster is not supported.");
    return false;
  }
} // namespace Arc
