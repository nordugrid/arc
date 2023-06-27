// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_JOBSUPERVISOR_H__
#define __ARC_JOBSUPERVISOR_H__

#include <list>
#include <string>

#include <arc/URL.h>
#include <arc/compute/JobControllerPlugin.h>
#include <arc/compute/EntityRetriever.h>

namespace Arc {

  class Logger;
  class Endpoint;
  class UserConfig;

  /// Abstract class used for selecting jobs with JobSupervisor
  /**
   * Only the select method is meant to be extended.
   * \since Added in 5.1.0.
   **/
  class JobSelector {
    public:
    JobSelector() {}
    virtual ~JobSelector() {}

    /// Indicate whether a job should be selected or not
    /**
     * This method should be extended and should indicate whether the passed job
     * should be selected or not by returning a boolean.
     * @param job A Job object which should either be selected or rejected.
     * @return true if passed job should be selected otherwise false is
     * returned.
     **/
    virtual bool Select(const Job& job) const = 0;
  };

  /// JobSupervisor class
  /**
   * The JobSupervisor class is tool for loading JobControllerPlugin plugins
   * for managing Grid jobs.
   *
   * \ingroup compute
   * \headerfile JobSupervisor.h arc/compute/JobSupervisor.h
   **/
  class JobSupervisor : public EntityConsumer<Job> {
  public:
    /// Create a JobSupervisor
    /**
     * The list of Job objects passed to the constructor will be managed by this
     * JobSupervisor, through the JobControllerPlugin class. It is important that the
     * InterfaceName member of each Job object is set and names a interface
     * supported by one of the available JobControllerPlugin plugins. The
     * JobControllerPlugin plugin will be loaded using the JobControllerPluginLoader class,
     * loading a plugin of type "HED:JobControllerPlugin" which supports the particular
     * interface, and the a reference to the UserConfig object usercfg will
     * be passed to the plugin. Additionally a reference to the UserConfig
     * object usercfg will be stored, thus usercfg must exist throughout the
     * scope of the created object. If the InterfaceName member of a Job object is
     * unset, a VERBOSE log message will be reported and that Job object will
     * be ignored. If the JobControllerPlugin plugin for a given interface cannot be
     * loaded, a WARNING log message will be reported and any Job object requesting
     * that interface will be ignored. If loading of a specific plugin failed,
     * that plugin will not be tried loaded for subsequent Job objects
     * requiring that plugin.
     * Job objects will be added to the corresponding JobControllerPlugin plugin, if
     * loaded successfully.
     *
     * @param usercfg UserConfig object to pass to JobControllerPlugin plugins and to
     *  use in member methods.
     * @param jobs List of Job objects which will be managed by the created
     *  object.
     **/
    JobSupervisor(const UserConfig& usercfg, const std::list<Job>& jobs = std::list<Job>());

    ~JobSupervisor() {}

    /// Add job
    /**
     * Add Job object to this JobSupervisor for job management. The Job
     * object will be passed to the corresponding specialized JobControllerPlugin.
     *
     * @param job Job object to add for job management
     * @return true is returned if the passed Job object was added to the
     *  underlying JobControllerPlugin, otherwise false is returned and a log
     *  message emitted with the reason.
     **/
    bool AddJob(const Job& job);

    void addEntity(const Job& job) { AddJob(job); }

    /// Update job information
    /**
     * When invoking this method the job information for the jobs managed by
     * this JobSupervisor will be updated. Internally, for each loaded
     * JobControllerPlugin the JobControllerPlugin::UpdateJobs method will be
     * called, which will be responsible for updating job information.
     **/
    void Update();

    /// Retrieve job output files
    /**
     * This method retrieves output files of jobs managed by this JobSupervisor.
     *
     * For each of the selected jobs, the job files will be downloaded to a
     * directory named either as the last part of the job ID or the job name,
     * which is determined by the 'usejobname' argument. The download
     * directories will be located in the directory specified by the
     * 'downloaddirprefix' argument, as either a relative or absolute path. If
     * the 'force' argument is set to 'true', and a download directory for a
     * given job already exist it will be overwritten, otherwise files for
     * that job will not be downloaded. This method calls the
     * JobControllerPlugin::GetJob method in order to download jobs, and if a
     * job is successfully retrieved and a corresponding directory exist on
     * disk, the path to the directory will be appended to the
     * 'downloaddirectories' list. If all jobs are successfully retrieved this
     * method returns true, otherwise false.
     *
     * @param downloaddirprefix specifies the path to in which job download
     *   directories will be located.
     * @param usejobname specifies whether to use the job name or job ID as
     *   directory name to store job output files in.
     * @param force indicates whether existing job directories should be
     *   overwritten or not.
     * @param downloaddirectories filled with a list of directories to which
     *   jobs were downloaded.
     * @see JobControllerPlugin::RetrieveJob.
     * @return true if all jobs are successfully retrieved, otherwise false.
     * \since Changed in 4.1.0. The path to download directory is only appended
     *  to the 'downloaddirectories' list if the directory exist.
     **/
    bool Retrieve(const std::string& downloaddirprefix, bool usejobname, bool force, std::list<std::string>& downloaddirectories);

    /// Renew job credentials
    /**
     * This method will renew credentials of jobs managed by this JobSupervisor.
     *
     * Before identifying jobs for which to renew credentials, the
     * JobControllerPlugin::UpdateJobs method is called for each loaded
     * JobControllerPlugin in order to retrieve the most up to date job
     * information.
     *
     * Since jobs in the JobState::DELETED, JobState::FINISHED or
     * JobState::KILLED states is in a terminal state credentials for those
     * jobs will not be renewed. Also jobs in the JobState::UNDEFINED state
     * will not get their credentials renewed, since job information is not
     * available. The JobState::FAILED state is also a terminal state, but
     * since jobs in this state can be restarted, credentials for such jobs
     * can be renewed. If the status-filter is non-empty, a renewal of
     * credentials will be done for jobs with a general or specific state
     * (see JobState) identical to any of the entries in the status-filter,
     * excluding the already filtered states as mentioned above.
     *
     * For each job for which to renew credentials, the specialized
     * JobControllerPlugin::RenewJob method is called and is responsible for
     * renewing the credentials for the given job. If the method fails to
     * renew any job credentials, this method will return false (otherwise
     * true), and the job ID (IDFromEndpoint) of such jobs is appended to the
     * notrenewed list. The job ID of successfully renewed jobs will be appended
     * to the passed renewed list.
     *
     * @return false if any call to JobControllerPlugin::RenewJob fails, true
     *  otherwise.
     * @see JobControllerPlugin::RenewJob.
     **/
    bool Renew();

    /// Resume jobs by status
    /**
     * This method resumes jobs managed by this JobSupervisor.
     *
     * Before identifying jobs to resume, the
     * JobControllerPlugin::UpdateJobs method is called for each loaded
     * JobControllerPlugin in order to retrieve the most up to date job
     * information.
     *
     * Since jobs in the JobState::DELETED, JobState::FINISHED or
     * JobState::KILLED states is in a terminal state credentials for those
     * jobs will not be renewed. Also jobs in the JobState::UNDEFINED state
     * will not be resumed, since job information is not available. The
     * JobState::FAILED state is also a terminal state, but jobs in this
     * state are allowed to be restarted. If the status-filter is non-empty,
     * only jobs with a general or specific state (see JobState) identical
     * to any of the entries in the status-filter will be resumed, excluding
     * the already filtered states as mentioned above.
     *
     * For each job to resume, the specialized JobControllerPlugin::ResumeJob
     * method is called and is responsible for resuming the particular job.
     * If the method fails to resume a job, this method will return false,
     * otherwise true is returned. The job ID of successfully resumed jobs
     * will be appended to the passed resumedJobs list.
     *
     * @return false if any call to JobControllerPlugin::ResumeJob fails, true
     *  otherwise.
     * @see JobControllerPlugin::ResumeJob.
     **/
    bool Resume();

    /// Cancel jobs
    /**
     * This method cancels jobs managed by this JobSupervisor.
     *
     * Before identifying jobs to cancel, the JobControllerPlugin::UpdateJobs
     * method is called for each loaded JobControllerPlugin in order to retrieve
     * the most up to date job information.
     *
     * Since jobs in the JobState::DELETED, JobState::FINISHED,
     * JobState::KILLED or JobState::FAILED states is already in a terminal
     * state, a cancel request will not be send for those. Also no
     * request will be send for jobs in the JobState::UNDEFINED state, since job
     * information is not available. If the status-filter is non-empty, a
     * cancel request will only be send to jobs with a general or specific state
     * (see JobState) identical to any of the entries in the status-filter,
     * excluding the states mentioned above.
     *
     * For each job to be cancelled, the specialized JobControllerPlugin::CancelJob
     * method is called and is responsible for cancelling the given job. If the
     * method fails to cancel a job, this method will return false (otherwise
     * true), and the job ID (IDFromEndpoint) of such jobs is appended to the
     * notcancelled list. The job ID of successfully cancelled jobs will be
     * appended to the passed cancelled list.
     *
     * @return false if any call to JobControllerPlugin::CancelJob failed, true
     *  otherwise.
     * @see JobControllerPlugin::CancelJob.
     **/
    bool Cancel();

    /// Clean jobs
    /**
     * This method removes from services jobs managed by this JobSupervisor.
     * Before cleaning jobs, the JobController::GetInformation method is called in
     * order to update job information, and that jobs are selected by job status
     * instead of by job IDs. The status list argument should contain states
     * for which cleaning of job in any of those states should be carried out.
     * The states are compared using both the JobState::operator() and
     * JobState::GetGeneralState() methods. If the status list is empty, all
     * jobs will be selected for cleaning.
     *
     * @return false if calls to JobControllerPlugin::CleanJob fails, true otherwise.
     **/
    bool Clean();

    const std::list<Job>& GetAllJobs() const { return jobs; }
    std::list<Job> GetSelectedJobs() const;

    void SelectValid();
    void SelectByStatus(const std::list<std::string>& status);
    void SelectByID(const std::list<std::string>& ids);

    /// Select jobs based on custom selector
    /**
     * Used to do more advanced job selections. NOTE: operations will only be
     * done on selected jobs.
     * \param js A JobSelector object which will be used for selecting jobs.
     * \see Arc::JobSelector
     * \since Added in 5.1.0.
     **/
    void Select(const JobSelector& js);

    void ClearSelection();

    const std::list<std::string>& GetIDsProcessed() const { return processed; }
    const std::list<std::string>& GetIDsNotProcessed() const { return notprocessed; }

  private:
    const UserConfig& usercfg;

    std::list<Job> jobs;
    // Selected and non-selected jobs.
    typedef std::map<JobControllerPlugin*, std::pair< std::list<Job*>, std::list<Job*> > > JobSelectionMap;
    JobSelectionMap jcJobMap;
    std::map<std::string, JobControllerPlugin*> loadedJCs;

    std::list<std::string> processed, notprocessed;

    JobControllerPluginLoader loader;

    static Logger logger;
  };

} //namespace ARC

#endif // __ARC_JOBSUPERVISOR_H__
