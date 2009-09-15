// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/GUID.h>
#include <arc/StringConv.h>
#include <arc/UserConfig.h>
#include <arc/client/ExecutionTarget.h>
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

  URL SubmitterCREAM::Submit(const JobDescription& jobdesc,
                             const ExecutionTarget& et) const {
    MCCConfig cfg;
    usercfg.ApplyToConfig(cfg);
    std::string delegationid = UUID();
    URL delegationurl(et.url);
    delegationurl.ChangePath(delegationurl.Path() + "/gridsite-delegation");
    CREAMClient gLiteClientDelegation(delegationurl, cfg, stringtoi(usercfg.ConfTree()["TimeOut"]));
    Config xcfg;
    usercfg.ApplyToConfig(xcfg);
    if (!gLiteClientDelegation.createDelegation(delegationid, xcfg["ProxyPath"])) {
      logger.msg(ERROR, "Creating delegation failed");
      return URL();
    }
    URL submissionurl(et.url);
    submissionurl.ChangePath(submissionurl.Path() + "/CREAM2");
    CREAMClient gLiteClientSubmission(submissionurl, cfg, stringtoi(usercfg.ConfTree()["TimeOut"]));
    gLiteClientSubmission.setDelegationId(delegationid);

    JobDescription job(jobdesc);
    ModifyJobDescription(job, et);

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

    AddJob(job, submissionurl.str() + '/' + jobInfo.jobId, et.Cluster,
           delegationurl.str() + '/' + delegationid);

    return submissionurl.str() + '/' + jobInfo.jobId;
  }

  URL SubmitterCREAM::Migrate(const URL& jobid, const JobDescription& jobdesc,
                              const ExecutionTarget& et,
                              bool forcemigration) const {
    logger.msg(ERROR, "Migration to a CREAM cluster is not supported.");
    return URL();
  }
  
  bool SubmitterCREAM::ModifyJobDescription(JobDescription& jobdesc, const ExecutionTarget& et) const {
    if (jobdesc.JDL_elements.find("BatchSystem") == jobdesc.JDL_elements.end() &&
        !et.ManagerProductName.empty())
      jobdesc.JDL_elements["BatchSystem"] = et.ManagerProductName;

    if (jobdesc.Resources.CandidateTarget.empty()) {
      ResourceTargetType candidateTarget;
      candidateTarget.EndPointURL = URL();
      candidateTarget.QueueName = et.MappingQueue;
      jobdesc.Resources.CandidateTarget.push_back(candidateTarget);
    }
    
    return true;
  }
} // namespace Arc
