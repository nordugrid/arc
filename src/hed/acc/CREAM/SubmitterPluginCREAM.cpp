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
#include "SubmitterPluginCREAM.h"

namespace Arc {

  bool SubmitterPluginCREAM::isEndpointNotSupported(const std::string& endpoint) const {
    const std::string::size_type pos = endpoint.find("://");
    return pos != std::string::npos && lower(endpoint.substr(0, pos)) != "http" && lower(endpoint.substr(0, pos)) != "https";
  }

  bool SubmitterPluginCREAM::Submit(const JobDescription& jobdesc, const ExecutionTarget& et, Job& job) {
    MCCConfig cfg;
    usercfg.ApplyToConfig(cfg);
    std::string delegationid = UUID();
    URL url(et.ComputingEndpoint->URLString);
    URL delegationurl(url);
    delegationurl.ChangePath(delegationurl.Path() + "/gridsite-delegation");
    CREAMClient gLiteClientDelegation(delegationurl, cfg, usercfg.Timeout());
    if (!gLiteClientDelegation.createDelegation(delegationid, usercfg.ProxyPath())) {
      logger.msg(INFO, "Failed creating singed delegation certificate");
      return false;
    }
    URL submissionurl(url);
    submissionurl.ChangePath(submissionurl.Path() + "/CREAM2");
    CREAMClient gLiteClientSubmission(submissionurl, cfg, usercfg.Timeout());
    gLiteClientSubmission.setDelegationId(delegationid);

    JobDescription preparedjobdesc(jobdesc);
    if (!preparedjobdesc.Prepare(et)) {
      logger.msg(INFO, "Failed to prepare job description to target resources");
      return false;
    }

    if (preparedjobdesc.OtherAttributes.find("egee:jdl;BatchSystem") == preparedjobdesc.OtherAttributes.end()) {
      if (!et.ComputingManager->ProductName.empty()) {
        preparedjobdesc.OtherAttributes["egee:jdl;BatchSystem"] = et.ComputingManager->ProductName;
      }
      else if (!et.ComputingShare->MappingQueue.empty()) {
        preparedjobdesc.OtherAttributes["egee:jdl;BatchSystem"] = et.ComputingShare->MappingQueue;
      }
    }

    std::string jobdescstring;
    if (!preparedjobdesc.UnParse(jobdescstring, "egee:jdl")) {
      logger.msg(INFO, "Unable to submit job. Job description is not valid in the %s format", "egee:jdl");
      return false;
    }

    creamJobInfo jobInfo;
    if (!gLiteClientSubmission.registerJob(jobdescstring, jobInfo)) {
      logger.msg(INFO, "Failed registering job");
      return false;
    }

    if (!PutFiles(preparedjobdesc, jobInfo.ISB)) {
      logger.msg(INFO, "Failed uploading local input files");
      return false;
    }

    if (!gLiteClientSubmission.startJob(jobInfo.id)) {
      logger.msg(INFO, "Failed starting job");
      return false;
    }

    AddJobDetails(preparedjobdesc, URL(submissionurl.str() + '/' + jobInfo.id), et.ComputingService->Cluster, job);

    XMLNode xIDFromEndpoint(jobInfo.ToXML());
    xIDFromEndpoint.NewChild("delegationID") = delegationurl.str() + '/' + delegationid;

    xIDFromEndpoint.GetXML(job.IDFromEndpoint);

    return true;
  }

  bool SubmitterPluginCREAM::Migrate(const URL& /* jobid */, const JobDescription& /* jobdesc */,
                               const ExecutionTarget& et, bool /* forcemigration */,
                               Job& /* job */) {
    logger.msg(INFO, "Trying to migrate to %s: Migration to a CREAM resource is not supported.", et.ComputingEndpoint->URLString);
    return false;
  }
} // namespace Arc
