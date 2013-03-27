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
  
  /**
   * \defgroup compute ARC Compute Library (libarccompute)
   * 
   * libarccompute is a library for discovering, quering, matching and ranking,
   * submitting jobs to and managing jobs on Grid resources, as well as parsing
   * and assembling job descriptions. It features a uniform high-level interface
   * to a wide range of Service Registries, Information Systems and Computing
   * Services. With this interface, registries can be queried for service
   * endpoints, information systems can be queried for detailed resource and job
   * information, and jobs can be submitted to and managed in a Grid
   * environment. The library doesn't offer specific interfaces to different
   * services, instead it tries to provide a uniform interface to different kind
   * of services.
   * 
   * An introduction on how to use the library to query services for information
   * is given in the description of the EntityRetriever class. How to use the
   * library for submitting jobs is described at the Submitter class reference
   * page. How to manage jobs with the library is described at the JobSupervisor
   * class reference page.
   * 
   * The library uses ARC's dynamic plugin mechanism to load plugins for
   * specific services and features only when required at runtime. These plugins
   * for the libarccompute library are called ARC Compute Components (ACCs).
   * Each of the classes listed below will transparently load any required ACC
   * plugins at runtime when needed. If preferred ACC plugins can also be used
   * and loaded manually, which is described in more detail \ref accplugins
   * "here".
   * 
   * Support for a custom service (info-system, registry or compute), a ranking
   * algorithm and/or a job description parsing/assembling algorithm is exactly
   * what is defined as a ACC, and it can easily be added to be accessible to
   * the libarccompute library. More details about creating such a plugin can be
   * found \ref accplugins "here".
   * 
   * With the default NorduGrid ARC plugins installed the librarccompute library
   * supports the following services and specifications:
   * 
   * <b>Computing Services:</b>
   * * EMI ES
   * * BES (+ ARC BES extension)
   * * CREAM
   * * GridFTPJob interface (requires the nordugrid-arc-plugins-globus package)
   * 
   * <b>Registry and Index Services:</b>
   * * EMIR
   * * EGIIS
   * * Top BDII
   * 
   * <b>Local Information Schemes:</b>
   * * %GLUE2 (through LDAP and EMI ES)
   * * NorduGrid schema (through LDAP)
   * * GLUE1 (through LDAP)
   * 
   * <b>Matchmaking and Ranking Algorithms:</b>
   * * Benchmark
   * * Fastest queue
   * * ACIX
   * * Random
   * * Python interface for custom broker
   * 
   * <b>%Job description languages:</b>
   * * EMI ADL
   * * xRSL
   * * JDL
   * * JSDL (+ Posix and HPC-P extensions)
   *
   * \page group__compute ARC Compute Library (libarccompute)
   */  

  /**
   * \ingroup compute
   * \headerfile Submitter.h arc/compute/Submitter.h 
   */
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
  
    /** A new EndpointSubmissionStatus is created with UNKNOWN status and with an empty description by default */
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

  /**
   * As the name indicates this class is used for submitting jobs. It has a
   * number of different submit methods which can be used directly for different
   * purposes. When initiating a object of this class a UserConfig object must
   * be passed, which should contain path to user credentials.
   * 
   * Generally there are two versions of submit methods. One which doesn't
   * accept a reference to a Job or list of Job objects, and one which does.
   * This is because the Submitter class is able to pass submitted Job objects
   * to consumer objects. Registering a consumer object is done using the
   * \ref Submitter::addConsumer "addConsumer" method passing a reference to a
   * EntityConsumer<Job> object. An
   * example of such a consumer is the JobSupervisor class. Multiple consumers
   * can be registered simultaneously. Every submit method will then pass
   * submitted Job objects to the registered consumer objects. A registered
   * consumer can be removed using the
   * \ref Submitter::removeConsumer "removeConsumer" method.
   * 
   * For submitting a Grid job one should use one of the
   * \ref Submitter::BrokeredSubmit "BrokeredSubmit" methods. They accept a list 
   * of job descriptions and a list of information system endpoints for which
   * computing services are discovered and matched to the job descriptions.
   * Jobs are then submitted to the matching services in the order ranked by the
   * \ref Broker "algorithm" specified in the UserConfig object.
   * 
   * Another way of submitting a job is by using the
   * \ref Submitter::Submit "Submit" methods. These methods accepts submission
   * endpoints or ExecutionTarget objects. Using these methods will not do
   * any client side checks whether the computing service resources pointed to
   * by the submission endpoint (or ExecutionTarget) really matches the
   * specified job description(s).
   * 
   * Common for both ways of submitting jobs is that they both return a
   * SubmissionStatus object indicating the outcome of the submission attemp(s).
   * If the returned status object indicates failures, further examination can
   * be carried out by using the
   * \ref Submitter::GetDescriptionsNotSubmitted "GetDescriptionsNotSubmitted",
   * \ref Submitter::GetEndpointQueryingStatuses "GetEndpointQueryingStatuses"
   * and/or \ref Submitter::GetEndpointSubmissionStatuses "GetEndpointSubmissionStatuses"
   * methods. Note that on each invocation of any of the submit methods the
   * state from a previous submission attemp will be cleared, thus the just
   * mentioned methods should be used just after an attempted submission fails.
   * 
   * \since Added in 2.0.0
   * \ingroup compute
   * \headerfile Submitter.h arc/compute/Submitter.h 
   */
  class Submitter {
  public:
    Submitter(const UserConfig& uc) : uc(uc) {}
    ~Submitter() {}

    // === Using the consumer concept as in the EntityRetriever ===
    void addConsumer(EntityConsumer<Job>& addConsumer_consumer /* The name 'addConsumer_consumer' is important for Swig when matching methods */) { consumers.push_back(&addConsumer_consumer); }
    void removeConsumer(EntityConsumer<Job>& removeConsumer_consumer /* The name 'removeConsumer_consumer' is important for Swig when matching methods */);
    // ===

    // === No brokering ===

    /**
     * Submit a job described by the passed JobDescription object to the
     * specified submission endpoint of a computing service. If successful, the
     * submitted job will be added to the registered consumer object. If
     * unsuccessful, more details can be obtained from the returned
     * SubmissionStatus object, or by using the
     * \ref Submitter::GetDescriptionsNotSubmitted "GetDescriptionsNotSubmitted",
     * \ref Submitter::GetEndpointQueryingStatuses "GetEndpointQueryingStatuses"
     * and \ref Submitter::GetEndpointSubmissionStatuses "GetEndpointSubmissionStatuses"
     * 
     * \param[in] endpoint the endpoint to which job should be submitted.
     * \param[in] desc the JobDescription object describing the job to be
     *  submitted.
     * \return A SubmissionStatus object is returned indicating the status of
     *  submission.
     * \see addConsumer
     * \see GetDescriptionsNotSubmitted
     * \see GetEndpointQueryingStatuses
     * \see GetEndpointSubmissionStatuses
     * \since Added in 3.0.0
     */
    SubmissionStatus Submit(const Endpoint& endpoint, const JobDescription& desc) { return Submit(endpoint, std::list<JobDescription>(1, desc)); }
    /**
     * Identical to Submit(const Endpoint&, const JobDescription&), with the
     * addition that the Job object passed as reference will also be filled with
     * job information if submission was successful.
     * 
     * \param[out] job a reference to a Job object which will be filled with
     *  job details if submission was successful.
     * \see Submit(const Endpoint&, const JobDescription&) for detailed
     *  description.
     * \since Added in 3.0.0
     */
    SubmissionStatus Submit(const Endpoint& endpoint, const JobDescription& desc, Job& job);
    /**
     * Identical to Submit(const Endpoint&, const JobDescription&), except that
     * this method submits multiple jobs to same endpoint. Submitted jobs will
     * be added to the registered consumer.
     * 
     * \see Submit(const Endpoint&, const JobDescription&)
     * \since Added in 3.0.0
     */
    SubmissionStatus Submit(const Endpoint& endpoint, const std::list<JobDescription>& descs);
    /**
     * Identical to Submit(const Endpoint&, const JobDescription&), with the
     * addition that the list of Job objects passed reference will filled with
     * the submitted jobs, and that multiple jobs are submitted to same
     * endpoint.
     * 
     * \see Submit(const Endpoint&, const JobDescription&)
     * \since Added in 3.0.0
     */
    SubmissionStatus Submit(const Endpoint& endpoint, const std::list<JobDescription>& descs, std::list<Job>& jobs);
    /**
     * Submit multiple jobs to a list of submission endpoints to computing
     * services. For each JobDescription object submission is tried against the
     * list of submission endpoints in order. If submission to a endpoint fails
     * the next in the list is tried - no ranking of endpoints will be done.
     * Also note that a job is only submitted once, and not to multiple
     * computing services. Submitted Job objects is passed to the registered
     * consumer objects.
     * 
     * \return A SubmissionStatus object is returned which indicates the
     *  outcome of the submission.
     * \see addConsumer
     * \see GetDescriptionsNotSubmitted
     * \see GetEndpointQueryingStatuses
     * \see GetEndpointSubmissionStatuses
     * \since Added in 3.0.0
     */
    SubmissionStatus Submit(const std::list<Endpoint>& endpoint, const std::list<JobDescription>& descs);
    /**
     * Identical to Submit(const Endpoint&, const std::list<JobDescription>&, std::list<Job>&)
     * with the addition that submitted jobs are also added to the passed list
     * of Job objects.
     * 
     * \see Submit(const Endpoint&, const std::list<JobDescription>&, std::list<Job>&)
     * \since Added in 3.0.0
     */
    SubmissionStatus Submit(const std::list<Endpoint>& endpoint, const std::list<JobDescription>& descs, std::list<Job>& jobs);
    
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

    SubmissionStatus SubmitNoClear(const Endpoint& endpoint, const std::list<JobDescription>& descs);

    const UserConfig& uc;

    EndpointStatusMap queryingStatusMap;
    std::map<Endpoint, EndpointSubmissionStatus> submissionStatusMap;
    
    std::list<const JobDescription*> notsubmitted;
    
    std::list<EntityConsumer<Job>*> consumers;
  
    static SubmitterPluginLoader& getLoader();

    static Logger logger;
  };
}

#endif // __ARC_SUBMITTER_H__
