// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_JOBSUPERVISOR_H__
#define __ARC_JOBSUPERVISOR_H__

#include <list>
#include <string>

#include <arc/URL.h>
#include <arc/client/JobController.h>

namespace Arc {

  class Logger;
  class UserConfig;

  /// % JobSupervisor class
  /**
   * The JobSupervisor class is tool for loading JobController plugins
   * for managing Grid jobs.
   **/

  class JobSupervisor {
  public:
    /// Create a JobSupervisor object
    /**
     * Default constructor to create a JobSupervisor. Automatically
     * loads JobController plugins based upon the input jobids.
     *
     * @param usercfg Reference to UserConfig object with information
     * about user credentials and joblistfile.
     *
     * @param jobs List of jobs(jobid or job name) to be managed.
     *
     **/
    JobSupervisor(const UserConfig& usercfg, const std::list<std::string>& jobs);

    /// Create a JobSupervisor
    /**
     * The list of Job objects passed to the constructor will be managed by this
     * JobSupervisor, through the JobController class. It is important that the
     * Flavour member of each Job object is set and correspond to the
     * JobController plugin which are capable of managing that specific job. The
     * JobController plugin will be loaded using the JobControllerLoader class,
     * loading a plugin of type "HED:JobController" and name specified by the
     * Flavour member, and the a reference to the UserConfig object usercfg will
     * be passed to the plugin. Additionally a reference to the UserConfig
     * object usercfg will be stored, thus usercfg must exist throughout the
     * scope of the created object. If the Flavour member of a Job object is
     * unset, a VERBOSE log message will be reported and that Job object will
     * be ignored. If the JobController plugin for a given Flavour cannot be
     * loaded, a WARNING log message will be reported and any Job object with
     * that Flavour will be ignored. If loading of a specific plugin failed,
     * that plugin will not be tried loaded for subsequent Job objects
     * requiring that plugin.
     * Job objects, for which the corresponding JobController plugin loaded
     * successfully, will be added to that plugin using the
     * JobController::FillJobStore(const Job&) method.
     *
     * @param usercfg UserConfig object to pass to JobController plugins and to
     *  use in member methods.
     * @param jobs List of Job objects which will be managed by the created
     *  object.
     **/
    JobSupervisor(const UserConfig& usercfg, const std::list<Job>& jobs);

    ~JobSupervisor();

    /// Get list of JobControllers
    /**
     * Method to get the list of JobControllers loaded by constructor.
     *
     **/
    const std::list<JobController*>& GetJobControllers() {
      return loader.GetJobControllers();
    }

    bool JobsFound() const { return jobsFound; }

  private:
    static Logger logger;
    JobControllerLoader loader;
    const UserConfig& usercfg;
    bool jobsFound;
  };

} //namespace ARC

#endif // __ARC_JOBSUPERVISOR_H__
