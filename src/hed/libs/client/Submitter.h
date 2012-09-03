// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_SUBMITTER_H__
#define __ARC_SUBMITTER_H__

#include <arc/UserConfig.h>

#include <arc/client/Endpoint.h>
#include <arc/client/EndpointQueryingStatus.h>
#include <arc/client/JobDescription.h>
#include <arc/client/Job.h>
#include <arc/client/SubmitterPlugin.h>
#include <arc/client/ExecutionTarget.h>
#include <arc/client/EntityRetriever.h>


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


  class SubmissionStatus {
  public:
    enum SubmissionStatusType {
      NONE = 0,
      NO_SERVICES = 1 << 0,
      ENDPOINT_NOT_QUERIED = 1 << 1,
      BROKER_PLUGIN_NOT_LOADED = 1 << 2,
      DESCRIPTION_NOT_SUBMITTED = 1 << 3,
      SUBMITTER_PLUGIN_NOT_LOADED = 1 << 4, 
      ERROR_FROM_ENDPOINT = 1 << 5
    };

    SubmissionStatus() : status(NONE) {}
    SubmissionStatus(const SubmissionStatus& s) : status(s.status) {}
    SubmissionStatus(SubmissionStatusType s) : status(s) {}
    SubmissionStatus(unsigned int s) : status(s & maxValue) {}
  
    SubmissionStatus& operator|=(SubmissionStatusType s) { status |= s; return *this; }
    SubmissionStatus& operator|=(const SubmissionStatus& s) { status |= s.status; return *this; }
    SubmissionStatus& operator|=(unsigned int s) { status |= (s & maxValue); return *this; }

    SubmissionStatus operator|(SubmissionStatusType s) { return (status | s); }
    SubmissionStatus operator|(const SubmissionStatus& s) { return (status | s.status); }
    SubmissionStatus operator|(unsigned int s) { return (status | (s & maxValue)); }
    
    SubmissionStatus& operator&=(SubmissionStatusType s) { status &= s; return *this; }
    SubmissionStatus& operator&=(const SubmissionStatus& s) { status &= s.status; return *this; }
    SubmissionStatus& operator&=(unsigned int s) { status &= s; return *this; }

    SubmissionStatus operator&(SubmissionStatusType s) { return (status & s); }
    SubmissionStatus operator&(const SubmissionStatus& s) { return (status & s.status); }
    SubmissionStatus operator&(unsigned int s) { return (status & s); }

    SubmissionStatus& operator=(SubmissionStatusType s) { status = s; return *this; }
    SubmissionStatus& operator=(unsigned int s) { status = (s & maxValue); return *this; }

    operator bool() const { return (status & NONE) == NONE; }
    
    bool operator==(const SubmissionStatus& s) const { return status == s.status; }
    bool operator==(SubmissionStatusType s) const { return status == (unsigned)s; }
    bool operator==(unsigned int s) const { return status == s; }

    bool operator!=(const SubmissionStatus& s) const { return !operator==(s); }
    bool operator!=(SubmissionStatusType s) const { return !operator==(s); }
    bool operator!=(unsigned int s) const { return !operator==(s); }
    
    bool isSet(SubmissionStatusType s) const { return (s & status) == (unsigned)s; }
    
  private:
    static const unsigned int maxValue = (1 << 6) - 1;
    unsigned int status;
  };


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
    bool Submit(const ExecutionTarget& et,   const JobDescription& desc) { return Submit(et, std::list<JobDescription>(1, desc)); }
    bool Submit(const ExecutionTarget& et,   const JobDescription& desc, Job& job);
    // =====
    // ===== Multiple jobs =====
    bool Submit(const ExecutionTarget& et,   const std::list<JobDescription>& descs);
    bool Submit(const ExecutionTarget& et,   const std::list<JobDescription>& descs, std::list<Job>& jobs);
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

    const std::map<Endpoint, EndpointQueryingStatus>& GetEndpointQueryingStatuses() const { return queryingStatusMap; }
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

    std::map<Endpoint, EndpointQueryingStatus> queryingStatusMap;
    std::map<Endpoint, EndpointSubmissionStatus> submissionStatusMap;
    
    std::list<const JobDescription*> notsubmitted;
    
    std::list<EntityConsumer<Job>*> consumers;
  
    static SubmitterPluginLoader loader;

    static Logger logger;
  };
}

#endif // __ARC_SUBMITTER_H__
