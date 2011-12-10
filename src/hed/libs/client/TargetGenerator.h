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
   * The TargetGenerator class is the umbrella class for resource discovery and
   * information retrieval (index servers and execution services). It can also
   * be used to discover user Grid jobs and detailed information. The
   * TargetGenerator loads TargetRetriever plugins (which implements the actual
   * information retrieval) from URL objects found in the UserConfig object
   * passed to its constructor using the custom TargetRetrieverLoader.
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
     * TargetRetrieverLoader. If the second parameter, startRetrieval, is
     * specified, and matches bitwise either a value of 1, 2 or both, retrieval
     * of execution services, jobs or both will be initiated.
     *
     * @param usercfg is a reference to a UserConfig object from which endpoints
     *        to execution and/or index services will be used. The object also
     *        hold information about user credentials.
     * @param startRetrival specifies whether retrival should be started
     *        directly. It will be parsed bitwise. A value of 1 will start
     *        execution service retrieval (RetrieveExecutionTargets), 2 jobs
     *        (RetrieveJobs), and 3 both, while 0 will not start retrieval at
     *        all. If not specified, default is 0.
     **/
    TargetGenerator(const UserConfig& usercfg, unsigned int startRetrieval = 0);
    ~TargetGenerator();

    /// Retrieve available execution services
    /**
     * The endpoints specified in the UserConfig object passed to this object
     * will be used to retrieve information about execution services
     * (ExecutionTarget objects). The discovery and information retrieval of
     * targets is carried out in parallel threads to speed up the process. If a
     * endpoint is a index service each execution service registered will be
     * queried.
     *
     * @see RetrieveJobs
     * @see GetExecutionTargets
     **/
    void RetrieveExecutionTargets();

    /// Retrieve job information from execution services
    /**
     * The endpoints specified in the UserConfig object passed to this object
     * will be used to retrieve job information from these endpoints. Only jobs
     * owned by the user which is identified by the credentials specified in the
     * passed UserConfig object will be considered (exception being services
     * which has no user authentification). If a endpoint is a index service,
     * each execution service registered will be queried, and searched for job
     * information.
     *
     * @see RetrieveExecutionTargets
     **/
    void RetrieveJobs();

    /// Return targets fetched by RetrieveExecutionTargets method
    /**
     * Method to return a const list of ExecutionTarget objects retrieved by the
     * RetrieveExecutionTargets method.
     *
     * @see RetrieveExecutionTargets
     * @see GetExecutionTargets
     **/
    const std::list<ExecutionTarget>& GetExecutionTargets() const { return foundTargets; }
    std::list<ExecutionTarget>& GetExecutionTargets() { return foundTargets; }

    /// Return jobs retrieved by RetrieveJobs method
    /**
     * Method to return the list of jobs found by a call to the GetJobs method.
     *
     * @return A list of the discovered jobs as Job objects is returned
     * @see RetrieveJobs
     **/
    const std::list<Job>& GetJobs() const { return foundJobs; }

    /// Add a new computing service to the foundServices list
    /**
     * Method to add a new service to the list of foundServices in a
     * thread secure way. Compares the argument URL against the
     * services returned by UserConfig::GetRejectedServices and only
     * allows to add the service if not specifically rejected.
     *
     * @param flavour The flavour if the the computing service.
     * @param url URL pointing to the information system of the computing service.
     **/
    bool AddService(const std::string& Flavour, const URL& url);

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
    bool AddIndexServer(const std::string& Flavour, const URL& url);

    /// Add a new ExecutionTarget to the foundTargets list
    /**
     * Method to add a new ExecutionTarget (usually discovered by a TargetRetriever) to the list of
     * foundTargets in a thread secure way.
     *
     * @param target ExecutionTarget to be added.
     *
     **/
    void AddTarget(const ExecutionTarget& target);

    /// Add a new Job to this object
    /**
     * Method to add a new Job (usually discovered by a TargetRetriever) to the
     * internal list of jobs in a thread secure way.
     *
     * @param job Job describing the job.
     * @see AddJob(const Job&)
     **/
    void AddJob(const Job& job);

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

    Glib::Mutex serviceMutex;
    Glib::Mutex indexServerMutex;
    Glib::Mutex targetMutex;
    Glib::Mutex jobMutex;

    SimpleCounter threadCounter;

    static Logger logger;
  };

} // namespace Arc

#endif // __ARC_TARGETGENERATOR_H__
