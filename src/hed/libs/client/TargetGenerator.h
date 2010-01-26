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
   * not collect the job details (see the arcsync CLI)). The
   * TargetGenerator loads TargetRetriever plugins (which implements
   * the actual information retrieval) from URL objects found in the
   * UserConfig object passed to its constructor using the custem
   * TargetRetrieverLoader. E.g. if an URL pointing to an ARC1
   * computing resource is found in the UserConfig object the
   * TargetRetrieverARC1 is loaded.
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
     * TargetRetrieverLoader.
     * 
     * @param usercfg Reference to UserConfig object with URL objects
     * to computing and/or index services and paths to user
     * credentials.
     **/
    TargetGenerator(const UserConfig& usercfg); ~TargetGenerator();

    /// Find available targets
    /**
     * Method to prepare a list of chosen Targets with a specified
     * detail level. Current implementation supports finding computing
     * clusters (ExecutionTarget) with full detail level and Grid jobs
     * with limited detail level.
     *
     * @param targetType 0 = ExecutionTarget, 1 = Grid jobs
     *
     * @param detailLevel 1 = All details, 2 = Limited details (not implemented)
     *
     **/
    void GetTargets(int targetType, int detailLevel);

    /// Return targets found by GetTargets
    /**
     * Method to return a const list of ExecutionTarget objects (currently
     * only supported Target type) found by the GetTarget method.
     **/
    const std::list<ExecutionTarget>& FoundTargets() const;
    
    /// Return targets found by GetTargets
    /**
     * Method to return the list of ExecutionTarget objects (currently
     * only supported Target type) found by the GetTarget method.
     **/
    std::list<ExecutionTarget>& ModifyFoundTargets();
    
    /// Return Grid jobs found by GetTargets
    /**
     * Method to return the list of Grid jobs found by a call to the
     * GetTargets method.
     **/
    const std::list<XMLNode*>& FoundJobs() const;
    
    /// Add a new computing service to the foundServices list
    /**
     * Method to add a new service to the list of foundServices in a
     * thread secure way. Compares the argument URL against the
     * services returned by UserConfig::GetRejectedServices and only
     * allows to add the service if not specifically rejected.
     *
     * @param url URL pointing to the information system of the computing service.
     *
     **/
    bool AddService(const URL& url);
    
    /// Add a new index server to the foundIndexServers list
    /**
     * Method to add a new index server to the list of
     * foundIndexServers in a thread secure way. Compares the argument
     * URL against the servers returned by
     * UserConfig::GetRejectedServices and only allows to add the
     * service if not specifically rejected.
     *
     * @param url URL pointing to the index server.
     *
     **/
    bool AddIndexServer(const URL& url);
    
    /// Add a new ExecutionTarget to the foundTargets list
    /**
     * Method to add a new ExecutionTarget (usually discovered by a TargetRetriever) to the list of
     * foundTargets in a thread secure way. 
     *
     * @param target ExecutionTarget to be added.
     *
     **/
    void AddTarget(const ExecutionTarget& target);
    
    /// Add a new Job to the foundJobs list
    /**
     * Method to add a new Job (usually discovered by a TargetRetriever) to the list of
     * foundJobs in a thread secure way. 
     *
     * @param job XMLNode describing the job.
     *
     **/
    // XMLNode is reference by itself - passing it as const& has no sense
    void AddJob(const XMLNode& job);
    
    /// Decrement the threadCounter by 1
    /**
     * Method to decrement the threadCounter by 1 in a thread secure way. 
     *
     **/   
    void RetrieverDone();

    /// Prints target information
    /**
     * Method to print information of the found targets to std::cout. 
     *
     * @param longlist false for minimal information, true for detailed information
     *
     **/
    void PrintTargetInfo(bool longlist) const;

  private:
    TargetRetrieverLoader loader;

    const UserConfig& usercfg;

    std::list<URL> foundServices;
    std::list<URL> foundIndexServers;
    std::list<ExecutionTarget> foundTargets;
    std::list<XMLNode*> foundJobs;

    Glib::Mutex serviceMutex;
    Glib::Mutex indexServerMutex;
    Glib::Mutex targetMutex;
    Glib::Mutex jobMutex;

    int threadCounter;
    Glib::Mutex threadMutex;
    Glib::Cond threadCond;

    static Logger logger;
  };

} // namespace Arc

#endif // __ARC_TARGETGENERATOR_H__
