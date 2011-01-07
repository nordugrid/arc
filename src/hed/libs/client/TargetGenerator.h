// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_TARGETGENERATOR_H__
#define __ARC_TARGETGENERATOR_H__

#include <list>
#include <string>


#include <arc/Thread.h>
#include <arc/UserConfig.h>
#include <arc/client/ExecutionTarget.h>
#include <arc/client/Job.h>
#include <arc/client/TargetRetriever.h>

namespace Arc {

  class Config;
  class Logger;
  class URL;

  /// %Target generation class
  /**
   * The TargetGenerator class is the umbrella class for resource
   * discovery and information retrieval (index servers and computing
   * clusters). It can also be used to locate user Grid jobs but does
   * not collect the job details. The
   * TargetGenerator loads TargetRetriever plugins (which implements
   * the actual information retrieval) from URL objects found in the
   * UserConfig object passed to its constructor using the custom
   * TargetRetrieverLoader.
   **/

  class TargetGenerator {
  public:
    /// Create a TargetGenerator object
    /**
     * Default constructor to create a TargeGenerator. The constructor
     * reads the computing and index service URL objects from the
     * passed UserConfig object using the
     * UserConfig:GetSelectedServices method. From each URL a matching
     * specialized TargetRetriever plugin is loaded using the
     * TargetRetrieverLoader. If the second parameter, startDiscovery, is
     * specified, and matches bitwise either a value of 1, 2 or both, discovery
     * of CEs, jobs or both will be initiated.
     *
     * @param usercfg is a reference to a UserConfig object from which endpoints
     *        to computing and/or index services will be used. The object also
     *        hold information about user credentials.
     * @param startDiscovery specifies whether discovery should be started
     *        directly. It will be parsed bitwise. A value of 1 will start
     *        CE discovery, 2 jobs, and 3 both, while 0 will not start discovery
     *        at all. If this parameter is not specified, it defaults to 0.
     **/
    TargetGenerator(const UserConfig& usercfg, unsigned int startDiscovery = 0); ~TargetGenerator();

    /// DEPRECATED: Find available targets
    /**
     * This method is DEPRECATED, use the GetExecutionTargets() or GetJobs()
     * method instead.
     * Method to prepare a list of chosen Targets with a specified
     * detail level. Current implementation supports finding computing
     * elements (ExecutionTarget) with full detail level and jobs
     * with limited detail level.
     *
     * @param targetType 0 = ExecutionTarget, 1 = Grid jobs
     * @param detailLevel
     * @see GetExecutionsTargets()
     * @see GetJobs()
     **/
    void GetTargets(int targetType, int detailLevel);

    /// Find available Execution Services
    /**
     * The endpoints specified in the UserConfig object passed to this object
     * will be used to discover Computing Elements (ExecutionTarget) and
     * information about the discovered CEs will be fetched. The discovery and
     * information retrieval of targets is carried out in parallel threads to
     * speed up the process. If a endpoint is a index service each CE
     * registered at that service will be queried.
     *
     * @see GetJobs
     **/
    void GetExecutionTargets();

    /// Find jobs at Execution Services
    /**
     * The endpoints specified in the UserConfig object passed to this object
     * will be used to discover jobs at these endpoints owned by the user
     * which is identified by the credentials specified in the passed UserConfig
     * object. When discovering a job, the available information about this
     * job is fetched as well. If a endpoint is a index service, each CE
     * registered at that service will be queried.
     *
     * @see GetExecutionTargets
     **/
    void GetJobs();

    /// Return targets found by GetTargets
    /**
     * Method to return a const list of ExecutionTarget objects (currently
     * only supported Target type) found by the GetTarget method.
     **/
    const std::list<ExecutionTarget>& FoundTargets() const;

    /// DEPRECATED: Return targets found by GetTargets
    /**
     * This method is DEPRECATED, use the FoundTargets() instead.
     * Method to return the list of ExecutionTarget objects (currently
     * only supported Target type) found by the GetTarget method.
     **/
    std::list<ExecutionTarget>& ModifyFoundTargets();

    /// DEPRECATED: Return jobs found by GetTargets
    /**
     * This method is DEPRECATED, use the GetFoundJobs method instead.
     * Method to return the list of jobs found by a call to the GetJobs method.
     *
     * @return A list of jobs in XML format is returned.
     **/
    const std::list<XMLNode*>& FoundJobs() const;

    /// Return jobs found by GetJobs
    /**
     * Method to return the list of jobs found by a call to the GetJobs method.
     *
     * @return A list of the discovered jobs as Job objects is returned
     **/
    const std::list<Job>& GetFoundJobs() const;

    /// Add a new computing service to the foundServices list
    /**
     * Method to add a new service to the list of foundServices in a
     * thread secure way. Compares the argument URL against the
     * services returned by UserConfig::GetRejectedServices and only
     * allows to add the service if not specifically rejected.
     *
     * @param flavour The flavour if the the computing service.
     * @param url URL pointing to the information system of the computing service.
     *
     **/
    bool AddService(const std::string Flavour, const URL& url);

    /// Add a new index server to the foundIndexServers list
    /**
     * Method to add a new index server to the list of
     * foundIndexServers in a thread secure way. Compares the argument
     * URL against the servers returned by
     * UserConfig::GetRejectedServices and only allows to add the
     * service if not specifically rejected.
     *
     * @param flavour The flavour if the the index server.
     * @param url URL pointing to the index server.
     *
     **/
    bool AddIndexServer(const std::string Flavour, const URL& url);

    /// Add a new ExecutionTarget to the foundTargets list
    /**
     * Method to add a new ExecutionTarget (usually discovered by a TargetRetriever) to the list of
     * foundTargets in a thread secure way.
     *
     * @param target ExecutionTarget to be added.
     *
     **/
    void AddTarget(const ExecutionTarget& target);

    /// DEPRECATED: Add a new Job to this object
    /**
     * This method is DEPRECATED, use the AddJob(const Job&) method instead.
     * Method to add a new Job (usually discovered by a TargetRetriever) to the
     * internal list of jobs in a thread secure way.
     *
     * @param job XMLNode describing the job.
     **/
    // XMLNode is reference by itself - passing it as const& has no sense
    void AddJob(const XMLNode& job);

    /// Add a new Job to this object
    /**
     * Method to add a new Job (usually discovered by a TargetRetriever) to the
     * internal list of jobs in a thread secure way.
     *
     * @param job Job describing the job.
     * @see AddJob(const Job&)
     **/
    void AddJob(const Job& job);

    /// DEPRECATED: Prints target information
    /**
     * This method is DEPRECATED, use the SaveTargetInfoToStream method instead.
     * Method to print information of the found targets to std::cout.
     *
     * @param longlist false for minimal information, true for detailed information
     * @see SaveTargetInfoToStream
     **/
    void PrintTargetInfo(bool longlist) const;

    /// Prints target information
    /**
     * Method to print information of the found targets to std::cout.
     *
     * @param out is a std::ostream object which to direct target informetion to.
     * @param longlist false for minimal information, true for detailed information
     **/
    void SaveTargetInfoToStream(std::ostream& out, bool longlist) const;

    /// Returns reference to worker counter
    /**
     * This method returns reference to counter which keeps
     * amount of started worker threads communicating with
     * services asynchronously. The counter must be incremented
     * for every thread started and decremented when thread
     * exits. Main thread will then wait till counters
     * drops to zero.
     **/
    SimpleCounter& ServiceCounter(void);

  private:
    TargetRetrieverLoader loader;

    const UserConfig& usercfg;

    std::map<std::string, std::list<URL> > foundServices;
    std::map<std::string, std::list<URL> > foundIndexServers;
    std::list<ExecutionTarget> foundTargets;
    std::list<Job> foundJobs;
    std::list<XMLNode*> xmlFoundJobs;

    Glib::Mutex serviceMutex;
    Glib::Mutex indexServerMutex;
    Glib::Mutex targetMutex;
    Glib::Mutex jobMutex;

    SimpleCounter threadCounter;

    static Logger logger;
  };

} // namespace Arc

#endif // __ARC_TARGETGENERATOR_H__
