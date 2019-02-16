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

  /// Class for submitting jobs.
  /**
   * As the name indicates this class is used for submitting jobs. It has a
   * number of different submit methods which can be used directly for different
   * job submission purposes. The class it self can be considered as a frontend 
   * to the SubmitterPlugin class, an abstract class which is extended by
   * specialised plugins providing the actual coupling to a particular type of
   * computing service. As a frontend, this class also takes care of loading
   * the specialised plugins and choosing the right plugin for a given computing
   * service, however that can also be done manually with the
   * SubmitterPluginLoader class. In order to use the Submitter class a
   * reference to a UserConfig object must be provided, it should exist
   * throughout the lifetime of the created Submitter object, and the UserConfig
   * object should contain configuration details such as the path to user
   * credentials.
   * 
   * Generally there are two types of submit methods. One which doesn't
   * accept a reference to a Job or list of Job objects, and one which does.
   * This is because the Submitter class is able to pass submitted Job objects
   * to consumer objects. Registering a consumer object is done using the
   * \ref Submitter::addConsumer "addConsumer" method passing a reference to an
   * EntityConsumer<Job> object. An
   * example of such a consumer is the JobSupervisor class. Multiple consumers
   * can be registered for the same Submitter object. Every submit method will
   * then pass submitted Job objects to the registered consumer objects. A
   * registered consumer can be removed using the
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
   * An example of submitting a Grid job using C++ is shown below:
   * \include basic_job_submission.cpp
   * This code can be compiled with
   * \code
   * g++ -o submit -I/usr/include/libxml2 `pkg-config --cflags glibmm-2.4` -l arccompute submit.cpp
   * \endcode

   * Same example using Python shown below:
   * \include basic_job_submission.py
   * 
   * \since Added in 2.0.0
   * \ingroup compute
   * \headerfile Submitter.h arc/compute/Submitter.h 
   */
  class Submitter {
  public:
     /// Create a Submitter object
     /**
      * Stores a reference to the passed UserConfig object which will be used
      * for obtaining among others location of user credentials.
      * \note The UserConfig object must exist throughout the life time of the
      * created Submitter object.
      */
    Submitter(const UserConfig& uc) : uc(uc) {}
    ~Submitter() {}

    // === Using the consumer concept as in the EntityRetriever ===
    /// Add a Job consumer
    /**
     * Add a consumer object which will be called every time a job is submitted.
     * 
     * Example use of consumer concept:
     * \param[in] addConsumer_consumer consumer object receiving newly submitted jobs.
     * \see removeConsumer
     */
    void addConsumer(EntityConsumer<Job>& addConsumer_consumer /* The name 'addConsumer_consumer' is important for Swig when matching methods */) { consumers.push_back(&addConsumer_consumer); }
    /// Remove a previous added consumer object.
    /**
     * \param[in] removeConsumer_consumer consumer object which should be removed.
     * \see addConsumer
     */
    void removeConsumer(EntityConsumer<Job>& removeConsumer_consumer /* The name 'removeConsumer_consumer' is important for Swig when matching methods */);
    // ===

    // === No brokering ===

    /// Submit job to endpoint
    /**
     * Submit a job described by the passed JobDescription object to the
     * specified submission endpoint of a computing service. The method will
     * load the specialised \ref SubmitterPlugin "submitter plugin" which
     * corresponds to the specified \ref Endpoint::InterfaceName "interface name".
     * If no such plugin is found submission is unsuccessful. If however the
     * the interface name is unspecified (empty), then all available submitter
     * plugins will be tried. If submission is successful, the submitted job
     * will be added to the registered consumer object. If unsuccessful, more
     * details can be obtained from the returned SubmissionStatus object, or by
     * using the \ref GetDescriptionsNotSubmitted,
     * \ref GetEndpointQueryingStatuses and \ref GetEndpointSubmissionStatuses.
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
    /// Submit job to endpoint
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
    /// Submit jobs to endpoint
    /**
     * Identical to Submit(const Endpoint&, const JobDescription&), except that
     * this method submits multiple jobs to same endpoint. Submitted jobs will
     * be added to the registered consumer.
     * 
     * \see Submit(const Endpoint&, const JobDescription&)
     * \since Added in 3.0.0
     */
    SubmissionStatus Submit(const Endpoint& endpoint, const std::list<JobDescription>& descs);
    /// Submit jobs to endpoint
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
    /// Submit jobs to any endpoints
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
    /// Submit jobs to any endpoints
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
    /// Submit job to ExecutionTarget (computing service)
    /**
     * Submit a job to a computing service, represented by a ExecutionTarget
     * object. This is useful when resource discovery is carried out manually,
     * not using the \ref BrokeredSubmit methods, but using the
     * \ref ComputingServiceRetriever class. The
     * \ref SubmitterPlugin "submitter plugin" corresponding to the
     * \ref ComputingEndpointAttributes::InterfaceName "interface name" will be
     * loaded. If that plugin cannot be loaded, submission will be unsuccessful.
     * When loaded the ExecutionTarget and JobDescription object will be passed
     * to the \ref SubmitterPlugin::Submit(const std::list<JobDescription>&, const ExecutionTarget&, EntityConsumer<Job>&, std::list<const JobDescription*>&) "SubmitterPlugin::Submit"
     * method of the loaded plugin and the status of that method is returned.
     * 
     * \param[in] et the ExecutionTarget to which job should be submitted.
     * \param[in] desc the JobDescription object describing the job to be
     *  submitted.
     * \return A SubmissionStatus object is returned indicating the status of
     *  submission.
     * \see addConsumer
     * \see GetDescriptionsNotSubmitted
     * \see GetEndpointQueryingStatuses
     * \see GetEndpointSubmissionStatuses
     */
    SubmissionStatus Submit(const ExecutionTarget& et, const JobDescription& desc) { return Submit(et, std::list<JobDescription>(1, desc)); }
    /// Submit job to ExecutionTarget (computing service)
    /**
     * Identical to Submit(const ExecutionTarget&, const JobDescription&), with
     * the addition that the Job object passed as reference will also be filled
     * with job information if submission was successful.
     * 
     * \param[in] et the ExecutionTarget to which job should be submitted.
     * \param[in] desc the JobDescription object describing the job to be
     *  submitted.
     * \return A SubmissionStatus object is returned indicating the status of
     *  submission.
     * \see Submit(const ExecutionTarget&, const JobDescription&)
     */
    SubmissionStatus Submit(const ExecutionTarget& et, const JobDescription& desc, Job& job);
    // =====
    // ===== Multiple jobs =====
    /// Submit jobs to ExecutionTarget (computing service)
    /**
     * Identical to Submit(const ExecutionTarget&, const JobDescription&),
     * except that this method submits multiple jobs to the same computing
     * service. Submitted jobs will be added to the registered consumer.
     * 
     * \param[in] et the ExecutionTarget to which job should be submitted.
     * \param[in] desc the JobDescription object describing the job to be
     *  submitted.
     * \return A SubmissionStatus object is returned indicating the status of
     *  submission.
     * \see Submit(const ExecutionTarget&, const JobDescription&)
     */
    SubmissionStatus Submit(const ExecutionTarget& et, const std::list<JobDescription>& descs);
    /// Submit jobs to ExecutionTarget (computing service)
    /**
     * Identical to Submit(const ExecutionTarget&, const JobDescription&)
     * with the addition that this method submits multiple jobs to the same
     * computing service, and that submitted jobs are also added to the passed
     * list of Job objects.
     * 
     * \param[in] et the ExecutionTarget to which job should be submitted.
     * \param[in] desc the JobDescription object describing the job to be
     *  submitted.
     * \return A SubmissionStatus object is returned indicating the status of
     *  submission.
     * \see Submit(const ExecutionTarget&, const JobDescription&)
     */
    SubmissionStatus Submit(const ExecutionTarget& et, const std::list<JobDescription>& descs, std::list<Job>& jobs);
    // =====
    // ====
    // ===

    // === Brokering with service discovery (multiple endpoints) ===
    // ==== Using provided JobDescription objects for brokering ====
    /// Submit jobs to matching and ranked computing services
    /**
     * The passed job descriptions will be submitted to any of the matching
     * computing services in ranked order, which have been discovered using the
     * provided information endpoints.
     * 
     * First all previously set statuses will be cleared by invoking the
     * \ref Submitter::ClearAll "ClearAll" method. Then resource discovery is
     * invoked using the ComputingServiceRetriever class to query the provided
     * information endpoints, then for each JobDescription object the discovered
     * computing services is matched against the job description, then
     * matching computing services is ranked according to the broker algorithm
     * (specified in the UserConfig object of the Submitter). Then if any
     * requested submission interfaces has been specified in the optional
     * parameter 'requestedSubmissionInterfaces', then computing services which
     * doesn't have a matching submission interface will be ignored. Lastly the
     * job description is tried submitted to the computing services in the
     * ranked order, and upon the first successful submission a corresponding
     * Job object is propagated to the registered consumers, and then the next
     * job description is processed. If a job description cannot be submitted a
     * pointer to it will be added to an internal list, which afterwards can be
     * obtained using the \ref GetDescriptionsNotSubmitted method. If any
     * problems was encountered during submission, more details can be obtained
     * from the returned SubmissionStatus object, or by using the
     * \ref GetDescriptionsNotSubmitted, \ref GetEndpointQueryingStatuses
     * and \ref GetEndpointSubmissionStatuses.
     * 
     * \param[in] endpoints the information endpoints which will be used to
     *  initiate resource discovery.
     * \param[in] descs the JobDescription objects describing the jobs to be
     *  submitted.
     * \param[in] requestedSubmissionInterfaces an optional list of submission
     *  interfaces to use for submission.
     * \return A SubmissionStatus object is returned indicating the status of
     *  the submissions.
     * \see addConsumer
     * \see GetDescriptionsNotSubmitted
     * \see GetEndpointQueryingStatuses
     * \see GetEndpointSubmissionStatuses
     * \since Added in 3.0.0
     */
    SubmissionStatus BrokeredSubmit(const std::list<std::string>& endpoints, const std::list<JobDescription>& descs, const std::list<std::string>& requestedSubmissionInterfaces = std::list<std::string>());
    /// Submit jobs to matching and ranked computing services
    /**
     * Identical to \ref BrokeredSubmit(const std::list<std::string>& endpoints, const std::list<JobDescription>& descs, const std::list<std::string>& requestedSubmissionInterfaces) "BrokeredSubmit"
     * except that submitted jobs are added to the referenced list of Job
     * objects.
     * 
     * \param[in] endpoints the information endpoints which will be used to
     *  initiate resource discovery.
     * \param[in] descs the JobDescription objects describing the jobs to be
     *  submitted.
     * \param[in] jobs reference to a list of Job obects for which to add
     *  submitted jobs.
     * \param[in] requestedSubmissionInterfaces an optional list of submission
     *  interfaces to use for submission.
     * \return A SubmissionStatus object is returned indicating the status of
     *  the submissions.
     * \see addConsumer
     * \see GetDescriptionsNotSubmitted
     * \see GetEndpointQueryingStatuses
     * \see GetEndpointSubmissionStatuses
     * \since Added in 3.0.0
     */
    SubmissionStatus BrokeredSubmit(const std::list<std::string>& endpoints, const std::list<JobDescription>& descs, std::list<Job>& jobs, const std::list<std::string>& requestedSubmissionInterfaces = std::list<std::string>());
    /// Submit jobs to matching and ranked computing services
    /**
     * Identical to \ref BrokeredSubmit(const std::list<std::string>& endpoints, const std::list<JobDescription>& descs, const std::list<std::string>& requestedSubmissionInterfaces) "BrokeredSubmit"
     * except that the endpoints are not strings but Endpoint objects, which
     * can be used to provide detailed information about endpoints making
     * resource discovery more performant.
     * 
     * \param[in] endpoints the information endpoints which will be used to
     *  initiate resource discovery.
     * \param[in] descs the JobDescription objects describing the jobs to be
     *  submitted.
     * \param[in] requestedSubmissionInterfaces an optional list of submission
     *  interfaces to use for submission.
     * \return A SubmissionStatus object is returned indicating the status of
     *  the submissions.
     * \see addConsumer
     * \see GetDescriptionsNotSubmitted
     * \see GetEndpointQueryingStatuses
     * \see GetEndpointSubmissionStatuses
     * \since Added in 3.0.0
     */
    SubmissionStatus BrokeredSubmit(const std::list<Endpoint>& endpoints, const std::list<JobDescription>& descs, const std::list<std::string>& requestedSubmissionInterfaces = std::list<std::string>());
    /// Submit jobs to matching and ranked computing services
    /**
     * Identical to \ref BrokeredSubmit(const std::list<std::string>& endpoints, const std::list<JobDescription>& descs, const std::list<std::string>& requestedSubmissionInterfaces) "BrokeredSubmit"
     * except that submitted jobs are added to the referenced list of Job
     * objects and that the endpoints are not strings but Endpoint objects,
     * which can be used to provide detailed information about endpoints making
     * resource discovery more performant.
     * 
     * \param[in] endpoints the information endpoints which will be used to
     *  initiate resource discovery.
     * \param[in] descs the JobDescription objects describing the jobs to be
     *  submitted.
     * \param[in] jobs reference to a list of Job obects for which to add
     *  submitted jobs.
     * \param[in] requestedSubmissionInterfaces an optional list of submission
     *  interfaces to use for submission.
     * \return A SubmissionStatus object is returned indicating the status of
     *  the submissions.
     * \see addConsumer
     * \see GetDescriptionsNotSubmitted
     * \see GetEndpointQueryingStatuses
     * \see GetEndpointSubmissionStatuses
     * \since Added in 3.0.0
     */
    SubmissionStatus BrokeredSubmit(const std::list<Endpoint>& endpoints, const std::list<JobDescription>& descs, std::list<Job>& jobs, const std::list<std::string>& requestedSubmissionInterfaces = std::list<std::string>());
    // ====
    // ===
  
    // === Methods for handling errors ===
    /// Get job descriptions not submitted
    /**
     * \return A reference to the list of the not submitted job descriptions is
     * returned.
     */
    const std::list<const JobDescription*>& GetDescriptionsNotSubmitted() const { return notsubmitted; }
    /// Clear list of not submitted job descriptions
    void ClearNotSubmittedDescriptions() { notsubmitted.clear(); }
    /// Get status map for queried endpoints
    /**
     * The returned map contains EndpointQueryingStatus objects of all the
     * information endpoints which were queried during resource discovery.
     * 
     * \return A reference to the status map of queried endpoints.
     * \since Added in 3.0.0
     */
    const EndpointStatusMap& GetEndpointQueryingStatuses() const { return queryingStatusMap; }
    /// Clear endpoint status querying map.
    /**
     * \since Added in 3.0.0
     */
    void ClearEndpointQueryingStatuses() { queryingStatusMap.clear(); }
    /// Get submission status map
    /**
     * The returned map contains EndpointSubmissionStatus objects for all the
     * submission endpoints which were tried for job submission.
     * 
     * \return A reference to the submission status map.
     * \since Added in 3.0.0
     */
    const std::map<Endpoint, EndpointSubmissionStatus>& GetEndpointSubmissionStatuses() const { return submissionStatusMap; }
    /// Clear submission status map
    /**
     * \since Added in 3.0.0
     */
    void ClearEndpointSubmissionStatuses() { submissionStatusMap.clear(); }
    
    /// Clear all status maps
    /**
     * Convenience method which calls \ref ClearEndpointQueryingStatuses and
     * \ref ClearEndpointSubmissionStatuses.
     * \since Added in 3.0.0
     */
    void ClearAllStatuses() { queryingStatusMap.clear(); submissionStatusMap.clear(); }
    /// Clear all
    /**
     * Convenience method which calls \ref ClearNotSubmittedDescriptions and
     * \ref ClearEndpointQueryingStatuses and
     * \ref ClearEndpointSubmissionStatuses.
     * 
     * \since Added in 3.0.0
     */
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
