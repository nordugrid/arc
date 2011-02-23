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

    /// Cancel jobs
    /**
     * This method will request cancellation of jobs, identified by their
     * IDFromEndpoint member, for which that URL is equal to any in the jobids
     * list. Only jobs corresponding to a Job object managed by this
     * JobSupervisor will be considered for cancellation. Job objects not in a
     * valid state (see JobState) will not be considered, and the IDFromEndpoint
     * URLs of those objects will be appended to the notcancelled URL list. For
     * jobs not in a finished state (see JobState::IsFinished), the
     * JobController::Cancel method will be called, passing the corresponding
     * Job object, in order to cancel the job. If the JobController::Cancel call
     * succeeds or if the job is in a finished state the IDFromEndpoint URL will
     * be appended to the list to be returned. If the JobController::Cancel call
     * fails the IDFromEndpoint URL is appended to the notkilled URL list.
     *
     * Note: If there is any URL in the jobids list for which there is no
     * corresponding Job object, then the size of the returned list plus the
     * size of the notcancelled list will not equal that of the jobids list.
     *
     * @param jobids List of Job::IDFromEndpoint URL objects for which a
     *  corresponding job, managed by this JobSupervisor should be cancelled.
     * @param notcancelled List of Job::IDFromEndpoint URL objects for which the
     *  corresponding job were not cancelled.
     * @return The list of Job::IDFromEndpoint URL objects of successfully
     *  cancelled or finished jobs is returned.
     **/
    std::list<URL> Cancel(const std::list<URL>& jobids, std::list<URL>& notcancelled);

    /// Clean jobs
    /**
     * This method will request cleaning of jobs, identified by their
     * IDFromEndpoint member, for which that URL is equal to any in the jobids
     * list. Only jobs corresponding to a Job object managed by this
     * JobSupervisor will be considered for cleaning. Job objects not in a valid
     * state (see JobState) will not be considered, and the IDFromEndpoint URLs
     * of those objects will be appended to the notcleaned URL list, otherwise
     * the JobController::Clean method will be called, passing the corresponding
     * Job object, in order to clean the job. If that method fails the
     * IDFromEndpoint URL of the Job object will be appended to the notcleaned
     * URL list, and if it succeeds the IDFromEndpoint URL will be appended
     * to the list of URL objects to be returned.
     *
     * Note: If there is any URL in the jobids list for which there is no
     * corresponding Job object, then the size of the returned list plus the
     * size of the notcleaned list will not equal that of the jobids list.
     *
     * @param jobids List of Job::IDFromEndpoint URL objects for which a
     *  corresponding job, managed by this JobSupervisor should be cleaned.
     * @param notcleaned List of Job::IDFromEndpoint URL objects for which the
     *  corresponding job were not cleaned.
     * @return The list of Job::IDFromEndpoint URL objects of successfully
     *  cleaned jobs is returned.
     **/
    std::list<URL> Clean(const std::list<URL>& jobids, std::list<URL>& notcleaned);

    /// Get list of JobControllers
    /**
     * Method to get the list of JobControllers loaded by constructor.
     *
     **/
    const std::list<JobController*>& GetJobControllers() {
      return loader.GetJobControllers();
    }

    bool JobsFound() const;

  private:
    static Logger logger;
    JobControllerLoader loader;
    const UserConfig& usercfg;
  };

} //namespace ARC

#endif // __ARC_JOBSUPERVISOR_H__
