// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/client/Submitter.h>

namespace Arc {

  void Submitter::removeConsumer(EntityConsumer<Job>& jc) {
    std::list<EntityConsumer<Job>*>::iterator it = std::find(consumers.begin(), consumers.end(), &jc);
    if (it != consumers.end()) {
      consumers.erase(it);
    }
  }
  
  bool Submitter::Submit(const ExecutionTarget& et, const std::list<JobDescription>& descs, std::list<Job>& jobs) {
    bool success = true;
    for (std::list<JobDescription>::const_iterator it = descs.begin(); it != descs.end(); it++) {
      Arc::Job job;
      if (et.Submit(uc, *it, job)) {
        jobs.push_back(job);
        for (std::list< EntityConsumer<Job>*>::iterator itc = consumers.begin(); itc != consumers.end(); itc++) {
          (*itc)->addEntity(job);
        }
      } else {
        success = false;
      }
    }
    return success;
  }
  
  bool Submitter::BrokeredSubmit(const std::list<Endpoint>& endpoints, const std::list<JobDescription>& descs, std::list<Job>& jobs) {
    return false;
  }
  
} // namespace Arc
