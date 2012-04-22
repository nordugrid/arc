// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/client/Submitter.h>

namespace Arc {

  SubmitterPluginLoader Submitter::loader;

  void Submitter::removeConsumer(EntityConsumer<Job>& jc) {
    std::list<EntityConsumer<Job>*>::iterator it = std::find(consumers.begin(), consumers.end(), &jc);
    if (it != consumers.end()) {
      consumers.erase(it);
    }
  }
  
  bool Submitter::Submit(const ExecutionTarget& et, const std::list<JobDescription>& descs, std::list<Job>& jobs) {
    ClearNotSubmittedDescriptions();

    SubmitterPlugin *sp = loader.loadByInterfaceName(et.ComputingEndpoint->InterfaceName, uc);
    
    if (sp == NULL) {
      for (std::list<JobDescription>::const_iterator it = descs.begin(); it != descs.end(); ++it) {
        notsubmitted.push_back(&*it);
      }
      return false;
    }
    
    bool success = true;
    for (std::list<JobDescription>::const_iterator it = descs.begin(); it != descs.end(); ++it) {
      std::list<Job> tmpJobs;
      if (sp->Submit(std::list<JobDescription>(1, *it), et, tmpJobs, notsubmitted) && !tmpJobs.empty()) {
        jobs.push_back(tmpJobs.back());
        for (std::list< EntityConsumer<Job>*>::iterator itc = consumers.begin(); itc != consumers.end(); ++itc) {
          (*itc)->addEntity(tmpJobs.back());
        }
      } else {
        success = false;
        notsubmitted.push_back(&*it);
      }
    }
    return success;
  }
  
  bool Submitter::BrokeredSubmit(const std::list<Endpoint>& endpoints, const std::list<JobDescription>& descs, std::list<Job>& jobs) {
    return false;
  }
  
} // namespace Arc
