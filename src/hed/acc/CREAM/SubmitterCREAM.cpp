// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/GUID.h>
#include <arc/StringConv.h>
#include <arc/UserConfig.h>
#include <arc/client/ExecutionTarget.h>
#include <arc/client/Job.h>
#include <arc/client/JobDescription.h>
#include <arc/message/MCC.h>

#include "CREAMClient.h"
#include "SubmitterCREAM.h"

namespace Arc {

  SubmitterCREAM::SubmitterCREAM(const UserConfig& usercfg)
    : Submitter(usercfg, "CREAM") {}

  SubmitterCREAM::~SubmitterCREAM() {}

  Plugin* SubmitterCREAM::Instance(PluginArgument *arg) {
    SubmitterPluginArgument *subarg =
      dynamic_cast<SubmitterPluginArgument*>(arg);
    if (!subarg)
      return NULL;
    return new SubmitterCREAM(*subarg);
  }

  bool SubmitterCREAM::Submit(const JobDescription& jobdesc, const ExecutionTarget& et, Job& job) {
    MCCConfig cfg;
    usercfg.ApplyToConfig(cfg);
    std::string delegationid = UUID();
    URL delegationurl(et.url);
    delegationurl.ChangePath(delegationurl.Path() + "/gridsite-delegation");
    CREAMClient gLiteClientDelegation(delegationurl, cfg, usercfg.Timeout());
    if (!gLiteClientDelegation.createDelegation(delegationid, usercfg.ProxyPath())) {
      logger.msg(INFO, "Failed creating singed delegation certificate");
      return false;
    }
    URL submissionurl(et.url);
    submissionurl.ChangePath(submissionurl.Path() + "/CREAM2");
    CREAMClient gLiteClientSubmission(submissionurl, cfg, usercfg.Timeout());
    gLiteClientSubmission.setDelegationId(delegationid);

    JobDescription modjobdesc(jobdesc);
    if (!ModifyJobDescription(modjobdesc, et)) {
      logger.msg(INFO, "Failed adapting job description to target resources");
      return false;
    }

    std::string jobdescstring;
    if (!modjobdesc.UnParse(jobdescstring, "egee:jdl")) {
      logger.msg(INFO, "Unable to submit job. Job description is not valid in the %s format", "egee:jdl");
      return false;
    }

    creamJobInfo jobInfo;
    if (!gLiteClientSubmission.registerJob(jobdescstring, jobInfo)) {
      logger.msg(INFO, "Failed registering job");
      return false;
    }

    if (!PutFiles(modjobdesc, jobInfo.ISB_URI)) {
      logger.msg(INFO, "Failed uploading local input files");
      return false;
    }

    if (!gLiteClientSubmission.startJob(jobInfo.jobId)) {
      logger.msg(INFO, "Failed starting job");
      return false;
    }

    AddJobDetails(modjobdesc, submissionurl.str() + '/' + jobInfo.jobId, et.Cluster,
                  delegationurl.str() + '/' + delegationid, job);

    job.ISB = URL(jobInfo.ISB_URI);
    job.OSB = URL(jobInfo.OSB_URI);

    return true;
  }

  bool SubmitterCREAM::Migrate(const URL& /* jobid */, const JobDescription& /* jobdesc */,
                               const ExecutionTarget& et, bool /* forcemigration */,
                               Job& /* job */) {
    logger.msg(INFO, "Trying to migrate to %s: Migration to a CREAM resource is not supported.", et.url.str());
    return false;
  }

  bool SubmitterCREAM::ModifyJobDescription(JobDescription& jobdesc, const ExecutionTarget& et) const {
    if (jobdesc.OtherAttributes.find("egee:jdl;BatchSystem") == jobdesc.OtherAttributes.end() &&
        !et.ManagerProductName.empty()) {
      jobdesc.OtherAttributes["egee:jdl;BatchSystem"] = et.ManagerProductName;
    }

    jobdesc.Resources.QueueName = et.ComputingShareName;;

    return true;
  }
} // namespace Arc
