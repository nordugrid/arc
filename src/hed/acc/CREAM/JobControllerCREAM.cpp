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
  
  bool JobControllerCREAM::GetThisJob(Job ThisJob, std::string downloaddir){};
  
  bool JobControllerCREAM::CleanThisJob(Job ThisJob, bool force){
    bool cleaned = true;
    
    MCCConfig cfg;
    PathIterator pi(ThisJob.JobID.Path(), true);
    URL url(ThisJob.JobID);
    url.ChangePath(*pi);
    Cream::CREAMClient gLiteClient(url, cfg);
    gLiteClient.purge(pi.Rest());
    
    return cleaned;
    
  }

  bool JobControllerCREAM::CancelThisJob(Job ThisJob){

    bool cancelled = true;
    
    MCCConfig cfg;
    PathIterator pi(ThisJob.JobID.Path(), true);
    URL url(ThisJob.JobID);
    url.ChangePath(*pi);
    Cream::CREAMClient gLiteClient(url, cfg);
    gLiteClient.cancel(pi.Rest());
    
    return cancelled;
    
  }

  URL JobControllerCREAM::GetFileUrlThisJob(Job ThisJob, std::string whichfile){};

} // namespace Arc
