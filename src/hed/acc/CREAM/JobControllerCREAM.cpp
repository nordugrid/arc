#include <arc/URL.h>
#include <arc/message/MCC.h>

#include "CREAMClient.h"
#include "JobControllerCREAM.h"

namespace Arc {

  JobControllerCREAM::JobControllerCREAM(Arc::Config *cfg)
    : JobController(cfg, "CREAM") {}

  JobControllerCREAM::~JobControllerCREAM() {}

  ACC *JobControllerCREAM::Instance(Config *cfg, ChainContext *) {
    return new JobControllerCREAM(cfg);
  }

  void JobControllerCREAM::GetJobInformation() {
    for (std::list<Job>::iterator iter = JobStore.begin(); iter != JobStore.end(); iter++) {
      MCCConfig cfg;
      PathIterator pi(iter->JobID.Path(), true);
      URL url(iter->JobID);
      url.ChangePath(*pi);
      Cream::CREAMClient gLiteClient(url, cfg);
      gLiteClient.stat(pi.Rest());
    }
  }

  void JobControllerCREAM::DownloadJobOutput(bool, std::string) {}

  void JobControllerCREAM::PerformAction(std::string) {}

} // namespace Arc
