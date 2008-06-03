#include <arc/GUID.h>
#include <arc/message/MCC.h>

#include "CREAMClient.h"
#include "SubmitterCREAM.h"

namespace Arc {

  SubmitterCREAM::SubmitterCREAM(Config *cfg)
    : Submitter(cfg) {}

  SubmitterCREAM::~SubmitterCREAM() { std::cout << "Delete submitter" << std::endl; }

  ACC *SubmitterCREAM::Instance(Config *cfg, ChainContext *) {
    return new SubmitterCREAM(cfg);
  }

  std::pair<URL, URL> SubmitterCREAM::Submit(const std::string& jobdesc) {
    MCCConfig cfg;
    std::string delegationid = UUID();
    URL url(SubmissionEndpoint);
    url.ChangePath("ce-cream/services/gridsite-delegation");
    Cream::CREAMClient gLiteClient1(url, cfg);
    gLiteClient1.createDelegation(delegationid);
    url.ChangePath("ce-cream/services/CREAM2");
    Cream::CREAMClient gLiteClient2(url, cfg);
    gLiteClient2.setDelegationId(delegationid);
    gLiteClient2.cache_path = "/tmp";
    gLiteClient2.job_root = Glib::get_current_dir();
    std::string desc(jobdesc);
    Cream::creamJobInfo jobInfo = gLiteClient2.submit(desc);
    std::cout << "jobId: " << jobInfo.jobId << std::endl;
    std::cout << "creamURL: " << jobInfo.creamURL << std::endl;
    std::cout << "ISB:" << jobInfo.ISB_URI << std::endl;
    std::cout << "OSB:" << jobInfo.OSB_URI << std::endl;

    return std::make_pair(jobInfo.creamURL + '/' + jobInfo.jobId,
			  jobInfo.OSB_URI);
  }

} // namespace Arc
