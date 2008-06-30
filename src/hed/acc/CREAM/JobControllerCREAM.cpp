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
      iter->State = gLiteClient.stat(pi.Rest());
    }
  }

  void JobControllerCREAM::DownloadJobOutput(bool, std::string) {}

  void JobControllerCREAM::Clean(bool force) {
    for (std::list<Job>::iterator iter = JobStore.begin(); iter != JobStore.end(); iter++) {
      MCCConfig cfg;
      PathIterator pi(iter->JobID.Path(), true);
      URL url(iter->JobID);
      url.ChangePath(*pi);
      Cream::CREAMClient gLiteClient(url, cfg);
      gLiteClient.purge(pi.Rest());
    }
  }

  void JobControllerCREAM::Kill(bool keep) {
    for (std::list<Job>::iterator iter = JobStore.begin(); iter != JobStore.end(); iter++) {
      MCCConfig cfg;
      PathIterator pi(iter->JobID.Path(), true);
      URL url(iter->JobID);
      url.ChangePath(*pi);
      Cream::CREAMClient gLiteClient(url, cfg);
      gLiteClient.cancel(pi.Rest());
      // Need to figure out how to handle this. The following fails because
      // "the job has a status not compatible with the JOB_PURGE command"
      // if (!keep)
      //   gLiteClient.purge(pi.Rest());
    }
  }

} // namespace Arc
