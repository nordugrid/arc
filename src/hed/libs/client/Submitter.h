// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_SUBMITTER_H__
#define __ARC_SUBMITTER_H__

#include <arc/UserConfig.h>

#include <arc/client/Endpoint.h>
#include <arc/client/JobDescription.h>
#include <arc/client/Job.h>
#include <arc/client/SubmitterPlugin.h>
#include <arc/client/ExecutionTarget.h>
#include <arc/client/EntityRetriever.h>


namespace Arc {  

  class Submitter {
  public:
    Submitter(const UserConfig& uc) : uc(uc) {}
    ~Submitter() {}
  
    // === Using the consumer concept as in the EntityRetriever ===
    void addConsumer(EntityConsumer<Job>& jc) { consumers.push_back(&jc); }
    void removeConsumer(EntityConsumer<Job>& jc);
    // ===

    // === No brokering ===
    // ==== Submission to single configuration (adaption of job description) ====
    // ===== Single job =====
    bool Submit(const ExecutionTarget& et,   const JobDescription& desc) { Job job; return Submit(et, desc, job); }
    bool Submit(const ExecutionTarget& et,   const JobDescription& desc, Job& job) {
      std::list<Job> jobs;
      bool result = Submit(et, std::list<JobDescription>(1, desc), jobs);
      if (jobs.size() > 0) job = jobs.front();
      return result;
    }
    // =====
    // ===== Multiple jobs =====
    bool Submit(const ExecutionTarget& et,   const std::list<JobDescription>& descs) { std::list<Job> jobs; return Submit(et, descs, jobs); }
    bool Submit(const ExecutionTarget& et,   const std::list<JobDescription>& descs, std::list<Job>& jobs);
    // =====
    // ====
    // ===

    // === Brokering with service discovery (multiple endpoints) ===
    // ==== Using provided JobDescription objects for brokering ====
    bool BrokeredSubmit(const std::list<std::string>& endpoints, const std::list<JobDescription>& descs) { std::list<Job> jobs; return BrokeredSubmit(endpoints, descs, jobs); }
    bool BrokeredSubmit(const std::list<Endpoint>& endpoints, const std::list<JobDescription>& descs) { std::list<Job> jobs; return BrokeredSubmit(endpoints, descs, jobs); }
    bool BrokeredSubmit(const std::list<std::string>& endpoints, const std::list<JobDescription>& descs, std::list<Job>& jobs) {
      std::list<Endpoint> endpointObjects;
      for (std::list<std::string>::const_iterator it = endpoints.begin(); it != endpoints.end(); it++) {
        endpointObjects.push_back(Endpoint(*it, Endpoint::JOBSUBMIT));
      }
      return BrokeredSubmit(endpointObjects, descs, jobs);
    }
    bool BrokeredSubmit(const std::list<Endpoint>& endpoints, const std::list<JobDescription>& descs, std::list<Job>& jobs);
    // ====
    // ===
  
    // === Methods for handling errors ===
    const std::list<const JobDescription*>& GetDescriptionsNotSubmitted() const { return notsubmitted; }
    void ClearNotSubmittedDescriptions() { notsubmitted.clear(); }
    // ===
  
  private:
    const UserConfig& uc;
  
    std::list<const JobDescription*> notsubmitted;
    std::list<EntityConsumer<Job>*> consumers;
  
    static SubmitterPluginLoader loader;
  };

}

#endif // __ARC_SUBMITTER_H__
