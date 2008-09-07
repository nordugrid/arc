#include <arc/GUID.h>
#include <arc/message/MCC.h>
#include <arc/client/JobDescription.h>

#include "CREAMClient.h"
#include "SubmitterCREAM.h"

namespace Arc {

  SubmitterCREAM::SubmitterCREAM(Config *cfg)
    : Submitter(cfg) {}

  SubmitterCREAM::~SubmitterCREAM() {}

  ACC *SubmitterCREAM::Instance(Config *cfg, ChainContext*) {
    return new SubmitterCREAM(cfg);
  }

  bool SubmitterCREAM::Submit(JobDescription& jobdesc, XMLNode &info) {
    MCCConfig cfg;
    std::string delegationid = UUID();
    URL url(submissionEndpoint);
    url.ChangePath("ce-cream/services/gridsite-delegation");
    CREAMClient gLiteClient1(url, cfg);
    if (!gLiteClient1.createDelegation(delegationid)) {
      logger.msg(ERROR, "Create delegation failed");
      return false;
    }
    url.ChangePath("ce-cream/services/CREAM2");
    CREAMClient gLiteClient2(url, cfg);
    gLiteClient2.setDelegationId(delegationid);
    std::string jobdescstring;
    jobdesc.getProduct(jobdescstring, "JDL");
    creamJobInfo jobInfo;
    if (!gLiteClient2.registerJob(jobdescstring, jobInfo)) {
      logger.msg(ERROR, "Submission failed");
      return false;
    }
    if (!PutFiles(jobdesc, jobInfo.ISB_URI)) {
      logger.msg(ERROR, "Submission failed");
      return false;
    }
    if (!gLiteClient2.startJob(jobInfo.jobId)) {
      logger.msg(ERROR, "Submission failed");
      return false;
    }

    info.NewChild("JobID") = jobInfo.creamURL + '/' + jobInfo.jobId;
    info.NewChild("InfoEndpoint") = jobInfo.OSB_URI; // should change
    info.NewChild("ISB") = jobInfo.ISB_URI;
    info.NewChild("OSB") = jobInfo.OSB_URI;
    info.NewChild("DelegationID") = delegationid;

    return true;
  }

} // namespace Arc
