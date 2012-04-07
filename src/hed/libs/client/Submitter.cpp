// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/client/Submitter.h>

namespace Arc {

  void Submitter::removeConsumer(JobConsumer& jc) {
    std::list<JobConsumer*>::iterator it = std::find(consumers.begin(), consumers.end(), &jc);
    if (it != consumers.end()) {
      consumers.erase(it);
    }
  }
  
  bool Submitter::Submit(const Endpoint& endpoint, const std::list<std::string>& descs, std::list<Job>& jobs) {
    return false;
  }
  
  bool Submit(const ExecutionTarget& et, const std::list<JobDescription>& descs, std::list<Job>& jobs) {
    return false;
  }
  
  bool BrokeredSubmit(const std::list<Endpoint>& endpoints, const std::list<JobDescription>& descs, std::list<Job>& jobs) {
    return false;
  }
  
} // namespace Arc
