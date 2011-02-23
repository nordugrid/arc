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

    /// Resubmit jobs
    /**
     * Jobs managed by this JobSupervisor will be resubmitted when invoking this
     * method, that is the job description of a job will be tried obtained, and
     * if successful a new job will be submitted.
     *
     * Before identifying jobs to be resubmitted, the
     * JobController::GetJobInformation method is called for each loaded
     * JobController in order to retrieve the most up to date job information.
     * If an empty status-filter is specified, all jobs managed by this
     * JobSupervisor will be considered for resubmission, except jobs in the
     * undefined state (see JobState). If the status-filter is not empty, then
     * only jobs with a general or specific state (see JobState) identical to
     * any of the entries in the status-filter will be considered, except jobs
     * in the undefined state. Jobs for which a job description cannot be
     * obtained and successfully parsed will not be considered and an ERROR log
     * message is reported, and the IDFromEndpoint URL is appended to the
     * notresubmitted list. Job descriptions will be tried obtained either
     * from Job object itself, or fetching them remotely. Furthermore if a Job
     * object has the LocalInputFiles object set, then the checksum of each of
     * the local input files specified in that object (key) will be calculated
     * and verified to match the checksum LocalInputFiles object (value). If
     * checksums are not matching the job will be filtered, and an ERROR log
     * message is reported and the IDFromEndpoint URL is appended to the
     * notresubmitted list. If no job have been identified for resubmission,
     * false will be returned if ERRORs were reported, otherwise true is
     * returned.
     *
     * The destination for jobs is partly determined by the destination
     * parameter. If a value of 1 is specified a job will only be targeted to
     * the execution service (ES) on which it reside. A value of 2 indicates
     * that a job should not be targeted to the ES it currently reside.
     * Specifying any other value will target any ES. The ESs which can be
     * targeted are those specified in the UserConfig object of this class, as
     * selected services. Before initiating any job submission, resource
     * discovery and broker loading is carried out using the TargetGenerator and
     * Broker classes, initialised by the UserConfig object of this class. If
     * Broker loading fails, or no ExecutionTargets are found, an ERROR log
     * message is reported and all IDFromEndpoint URLs for job considered for
     * resubmission will be appended to the notresubmitted list and then false
     * will be returned.
     *
     * When the above checks have been carried out successfully, then the
     * Broker::Submit method will be invoked for each considered for
     * resubmission. If it fails the IDFromEndpoint URL for the job is appended
     * to the notresubmitted list, and an ERROR is reported. If submission
     * succeeds the new job represented by a Job object will be appended to the
     * resubmittedJobs list - it will not be added to this JobSupervisor. The
     * method returns false if ERRORs were reported otherwise true is returned.
     *
     * @param statusfilter list of job status used for filtering jobs.
     * @param destination specifies how target destination should be determined
     *  (1 = same target, 2 = not same, any other = any target).
     * @param resubmittedJobs list of Job objects which resubmitted jobs will be
     *  appended to.
     * @param notresubmitted list of URL object which the IDFromEndpoint URL
     *  will be appended to.
     * @return false if any error is encountered, otherwise true.
     **/
    bool Resubmit(const std::list<std::string>& statusfilter, int destination,
                  std::list<Job>& resubmittedJobs, std::list<URL>& notresubmitted);

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
