// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_JOBSUPERVISOR_H__
#define __ARC_JOBSUPERVISOR_H__

#include <list>
#include <string>

#include <arc/URL.h>
#include <arc/client/JobControllerPlugin.h>
#include <arc/client/EntityRetriever.h>

namespace Arc {

  class Logger;
  class Endpoint;
  class UserConfig;

  /// % JobSupervisor class
  /**
   * The JobSupervisor class is tool for loading JobControllerPlugin plugins
   * for managing Grid jobs.
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
     * Before identifying jobs for which to retrieve output files, the
     * JobControllerPlugin::UpdateJobs method is called for each loaded
     * JobControllerPlugin in order to retrieve the most up to date job information.
     * If an empty status-filter is specified, all jobs managed by this
     * JobSupervisor will be considered for retrieval, except jobs in the
     * undefined state (see JobState). If the status-filter is not empty, then
     * only jobs with a general or specific state (see JobState) identical to
     * any of the entries in the status-filter will be considered, except jobs
     * in the undefined state. Jobs in the state JobState::DELETED and
     * unfinished jobs (see JobState::IsFinished) will also not be considered.
     *
     * For each of the jobs considered for retrieval, the files will be
     * downloaded to a directory named either as the last part of the job ID
     * or the job name, which is determined by the 'usejobname' argument. The
     * download directories will be located in the directory specified by the
     * 'downloaddir' argument, as either a relative or absolute path. If the
     * 'force' argument is set to 'true', and a download directory for a
     * given job already exist it will be overwritten, otherwise files for
     * that job will not be downloaded. This method calls the
     * JobControllerPlugin::GetJob method in order to download jobs, and if a job
     * is successfully retrieved the job ID will be appended to the
     * 'retrievedJobs' list. If all jobs are successfully retrieved this
     * method returns true, otherwise false.
     *
     * @param downloaddir specifies the path to in which job download
     *   directories will be located.
     * @param usejobname specifies whether to use the job name or job ID as
     *   directory name to store job output files in.
     * @param force indicates whether existing job directories should be
     *   overwritten or not.
     * @see JobControllerPlugin::RetrieveJob.
     * @return true if all jobs are successfully retrieved, otherwise false.
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

    /// Resubmit jobs
    /**
     * Jobs managed by this JobSupervisor will be resubmitted when invoking this
     * method, that is the job description of a job will be tried obtained, and
     * if successful a new job will be submitted.
     *
     * Before identifying jobs to be resubmitted, the
     * JobControllerPlugin::UpdateJobs method is called for each loaded
     * JobControllerPlugin in order to retrieve the most up to date job information.
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
     * @param destination specifies how target destination should be determined
     *  (1 = same target, 2 = not same, any other = any target).
     * @param resubmittedJobs list of Job objects which resubmitted jobs will be
     *  appended to.
     * @param TODO
     * @return false if any error is encountered, otherwise true.
     **/
    bool Resubmit(int destination, const std::list<Endpoint>&, std::list<Job>& resubmittedJobs, const std::list<std::string>& = std::list<std::string>());

    /// Migrate jobs
    /**
     * Jobs managed by this JobSupervisor will be migrated when invoking this
     * method, that is the job description of a job will be tried obtained, and
     * if successful a job migration request will be sent, based on that job
     * description.
     *
     * Before identifying jobs to be migrated, the
     * JobControllerPlugin::UpdateJobs method is called for each loaded
     * JobControllerPlugin in order to retrieve the most up to date job information.
     * Only jobs for which the State member of the Job object has the value
     * JobState::QUEUEING, will be considered for migration. Furthermore the job
     * description must be obtained (either locally or remote) and successfully
     * parsed in order for a job to be migrated. If the job description cannot
     * be obtained or parsed an ERROR log message is reported, and the
     * IDFromEndpoint URL of the Job object is appended to the notmigrated list.
     * If no jobs have been identified for migration, false will be returned in
     * case ERRORs were reported, otherwise true is returned.
     *
     * The execution services which can be targeted for migration are those
     * specified in the UserConfig object of this class, as selected services.
     * Before initiating any job migration request, resource discovery and
     * broker* loading is carried out using the TargetGenerator and Broker
     * classes, initialised by the UserConfig object of this class. If Broker
     * loading fails, or no ExecutionTargets are found, an ERROR log message is
     * reported and all IDFromEndpoint URLs for job considered for migration
     * will be appended to the notmigrated list and then false will be returned.
     *
     * When the above checks have been carried out successfully, the following
     * is done for each job considered for migration. The ActivityOldID member
     * of the Identification member in the job description will be set to that
     * of the Job object, and the IDFromEndpoint URL will be appended to
     * ActivityOldID member of the job description. After that the Broker object
     * will be used to find a suitable ExecutionTarget object, and if found a
     * migrate request will tried sent using the ExecutionTarget::Migrate
     * method, passing the UserConfig object of this class. The passed
     * forcemigration boolean indicates whether the migration request at the
     * service side should ignore failures in cancelling the existing queuing
     * job. If the request succeeds, the corresponding new Job object is
     * appended to the migratedJobs list. If no suitable ExecutionTarget objects
     * are found an ERROR log message is reported and the IDFromEndpoint URL of
     * the Job object is appended to the notmigrated list. When all jobs have
     * been processed, false is returned if any ERRORs were reported, otherwise
     * true.
     *
     * @param forcemigration indicates whether migration should succeed if
     *  service fails to cancel the existing queuing job.
     * @param migratedJobs list of Job objects which migrated jobs will be
     *  appended to.
     * @param TODO
     * @return false if any error is encountered, otherwise true.
     **/
    bool Migrate(bool forcemigration, const std::list<Endpoint>&, std::list<Job>& migratedJobs, const std::list<std::string>& = std::list<std::string>());

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
    void SelectByID(const std::list<URL>& ids);

    void ClearSelection();
    
    const std::list<URL>& GetIDsProcessed() const { return processed; }
    const std::list<URL>& GetIDsNotProcessed() const { return notprocessed; }

  private:
    const UserConfig& usercfg;

    std::list<Job> jobs;
    // Selected and non-selected jobs.
    typedef std::map<JobControllerPlugin*, std::pair< std::list<Job*>, std::list<Job*> > > JobSelectionMap;
    JobSelectionMap jcJobMap;
    std::map<std::string, JobControllerPlugin*> loadedJCs;

    std::list<URL> processed, notprocessed;

    JobControllerPluginLoader loader;

    static Logger logger;
  };

} //namespace ARC

#endif // __ARC_JOBSUPERVISOR_H__
