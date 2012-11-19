// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_SUBMITTER_H__
#define __ARC_SUBMITTER_H__

#include <arc/UserConfig.h>

#include <arc/compute/Endpoint.h>
#include <arc/compute/EndpointQueryingStatus.h>
#include <arc/compute/JobDescription.h>
#include <arc/compute/Job.h>
#include <arc/compute/SubmitterPlugin.h>
#include <arc/compute/ExecutionTarget.h>
#include <arc/compute/EntityRetriever.h>


namespace Arc {  

  class EndpointSubmissionStatus {
  public:
    /** The possible states: */
    enum EndpointSubmissionStatusType {
      UNKNOWN,
      NOPLUGIN,
      SUCCESSFUL
    };
  
    /** String representation of the states in the enum #EndpointSubmissionStatusType */
    static std::string str(EndpointSubmissionStatusType status);
  
    /** A new EndpointSubmissionStatus is created with #UNKNOWN status and with an empty description by default */
    EndpointSubmissionStatus(EndpointSubmissionStatusType status = UNKNOWN, const std::string& description = "") : status(status), description(description) {};
  
    /** This EndpointSubmissionStatus object equals to an enum #EndpointSubmissionStatusType if it contains the same state */
    bool operator==(EndpointSubmissionStatusType s) const { return status == s; };
    /** This EndpointSubmissionStatus object equals to another EndpointQueryingStatus object, if their state equals.
      The description doesn't matter.
    */
    bool operator==(const EndpointSubmissionStatus& s) const { return status == s.status; };
    /** Inequality. \see operator==(EndpointSubmissionStatus) */
    bool operator!=(EndpointSubmissionStatusType s) const { return status != s; };
    /** Inequality. \see operator==(const EndpointSubmissionStatus&) */
    bool operator!=(const EndpointSubmissionStatus& s) const { return status != s.status; };
    /** \return true if the status is not successful */
    bool operator!() const { return status != SUCCESSFUL; };
    /** \return true if the status is successful */
    operator bool() const  { return status == SUCCESSFUL; };
  
    /** Setting the EndpointSubmissionStatus object's state
      \param[in] s the new enum #EndpointSubmissionStatusType status 
    */
    EndpointSubmissionStatus& operator=(EndpointSubmissionStatusType s) { status = s; return *this; };
    /** Copying the EndpointSubmissionStatus object into this one.
      \param[in] s the EndpointSubmissionStatus object whose status and description will be copied into this object 
    */
    EndpointSubmissionStatus& operator=(const EndpointSubmissionStatus& s) { status = s.status; description = s.description; return *this; };
  
    /** Return the enum #EndpointSubmissionStatusType contained within this EndpointSubmissionStatus object */
    EndpointSubmissionStatusType getStatus() const { return status; };
    /** Return the description string contained within this EndpointSubmissionStatus object */
    const std::string& getDescription() const { return description; };
    /** String representation of the EndpointSubmissionStatus object,
      which is currently simply the string representation of the enum #EndpointSubmissionStatusType
    */
    std::string str() const { return str(status); };
  
  private:
    EndpointSubmissionStatusType status;
    std::string description;
  };

  class SubmissionStatus;

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
    SubmissionStatus Submit(const ExecutionTarget& et, const JobDescription& desc) { return Submit(et, std::list<JobDescription>(1, desc)); }
    SubmissionStatus Submit(const ExecutionTarget& et, const JobDescription& desc, Job& job);
    // =====
    // ===== Multiple jobs =====
    SubmissionStatus Submit(const ExecutionTarget& et, const std::list<JobDescription>& descs);
    SubmissionStatus Submit(const ExecutionTarget& et, const std::list<JobDescription>& descs, std::list<Job>& jobs);
    // =====
    // ====
    // ===

    // === Brokering with service discovery (multiple endpoints) ===
    // ==== Using provided JobDescription objects for brokering ====
    SubmissionStatus BrokeredSubmit(const std::list<std::string>& endpoints, const std::list<JobDescription>& descs, const std::list<std::string>& requestedSubmissionInterfaces = std::list<std::string>());
    SubmissionStatus BrokeredSubmit(const std::list<std::string>& endpoints, const std::list<JobDescription>& descs, std::list<Job>&, const std::list<std::string>& requestedSubmissionInterfaces = std::list<std::string>());
    SubmissionStatus BrokeredSubmit(const std::list<Endpoint>& endpoints, const std::list<JobDescription>& descs, const std::list<std::string>& requestedSubmissionInterfaces = std::list<std::string>());
    SubmissionStatus BrokeredSubmit(const std::list<Endpoint>& endpoints, const std::list<JobDescription>& descs, std::list<Job>& jobs, const std::list<std::string>& requestedSubmissionInterfaces = std::list<std::string>());
    // ====
    // ===
  
    // === Methods for handling errors ===
    const std::list<const JobDescription*>& GetDescriptionsNotSubmitted() const { return notsubmitted; }
    void ClearNotSubmittedDescriptions() { notsubmitted.clear(); }

    const EndpointStatusMap& GetEndpointQueryingStatuses() const { return queryingStatusMap; }
    void ClearEndpointQueryingStatuses() { queryingStatusMap.clear(); }
    
    const std::map<Endpoint, EndpointSubmissionStatus>& GetEndpointSubmissionStatuses() const { return submissionStatusMap; }
    void ClearEndpointSubmissionStatuses() { submissionStatusMap.clear(); }
    
    void ClearAllStatuses() { queryingStatusMap.clear(); submissionStatusMap.clear(); }
    void ClearAll() { notsubmitted.clear(); queryingStatusMap.clear(); submissionStatusMap.clear(); }
    // ===
  
  private:
    class ConsumerWrapper : public EntityConsumer<Job> {
    public:
      ConsumerWrapper(Submitter& s) : s(s) {}
      void addEntity(const Job& j) {
        for (std::list<EntityConsumer<Job>*>::iterator it = s.consumers.begin(); it != s.consumers.end(); ++it) {
          (*it)->addEntity(j);
        }
      }
    private:
      Submitter& s;
    };

    const UserConfig& uc;

    EndpointStatusMap queryingStatusMap;
    std::map<Endpoint, EndpointSubmissionStatus> submissionStatusMap;
    
    std::list<const JobDescription*> notsubmitted;
    
    std::list<EntityConsumer<Job>*> consumers;
  
    static SubmitterPluginLoader loader;

    static Logger logger;
  };
}

#endif // __ARC_SUBMITTER_H__
