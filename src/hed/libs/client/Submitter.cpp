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
  
  class JobConsumerSingle : public EntityConsumer<Job> {
  public:
    JobConsumerSingle(Job& j) : _j(j) {}
    void addEntity(const Job& j) { _j = j; }
  private:
    Job& _j;
  };

  class JobConsumerList : public EntityConsumer<Job> {
  public:
    JobConsumerList(std::list<Job>& joblist) : joblist(joblist) {}
    void addEntity(const Job& j) { joblist.push_back(j); }
  private:
    std::list<Job>& joblist;
  };

  bool Submitter::Submit(const ExecutionTarget& et, const JobDescription& desc, Job& job) {
    JobConsumerSingle jcs(job);
    addConsumer(jcs);
    bool ok = Submit(et, std::list<JobDescription>(1, desc));
    removeConsumer(jcs);
    return ok;
  }

  bool Submitter::Submit(const ExecutionTarget& et, const std::list<JobDescription>& descs, std::list<Job>& jobs) {
    JobConsumerList jcl(jobs);
    addConsumer(jcl);
    bool ok = Submit(et, descs);
    removeConsumer(jcl);
    return ok;
  }

  bool Submitter::Submit(const ExecutionTarget& et, const std::list<JobDescription>& descs) {
    ClearNotSubmittedDescriptions();

    ConsumerWrapper cw(*this);

    SubmitterPlugin *sp = loader.loadByInterfaceName(et.ComputingEndpoint->InterfaceName, uc);
    
    if (sp == NULL) {
      for (std::list<JobDescription>::const_iterator it = descs.begin(); it != descs.end(); ++it) {
        notsubmitted.push_back(&*it);
      }
      return false;
    }
    
    return sp->Submit(descs, et, cw, notsubmitted);
  }
  
  bool Submitter::BrokeredSubmit(const std::list<Endpoint>& endpoints, const std::list<JobDescription>& descs, std::list<Job>& jobs) {
    return false;
  }
  
} // namespace Arc
