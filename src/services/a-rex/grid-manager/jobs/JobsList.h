#ifndef GRID_MANAGER_STATES_H
#define GRID_MANAGER_STATES_H

#include <sys/types.h>
#include <list>
#include <glib.h>

#include <arc/Thread.h>

#include "../conf/StagingConfig.h"

#include "GMJob.h"
#include "JobDescriptionHandler.h"
#include "DTRGenerator.h"

namespace ARex {

class JobFDesc;
class GMConfig;

/// ZeroUInt is a wrapper around unsigned int. It provides a consistent default
/// value, as int type variables have no predefined value assigned upon
/// creation. It also protects from potential counter underflow, to stop
/// counter jumping to MAX_INT. TODO: move to common lib?
class ZeroUInt {
private:
 unsigned int value_;
public:
 ZeroUInt(void):value_(0) { };
 ZeroUInt(unsigned int v):value_(v) { };
 ZeroUInt(const ZeroUInt& v):value_(v.value_) { };
 ZeroUInt& operator=(unsigned int v) { value_=v; return *this; };
 ZeroUInt& operator=(const ZeroUInt& v) { value_=v.value_; return *this; };
 ZeroUInt& operator++(void) { ++value_; return *this; };
 ZeroUInt operator++(int) { ZeroUInt temp(value_); ++value_; return temp; };
 ZeroUInt& operator--(void) { if(value_) --value_; return *this; };
 ZeroUInt operator--(int) { ZeroUInt temp(value_); if(value_) --value_; return temp; };
 operator unsigned int(void) const { return value_; };
};


/// List of jobs. This class contains the main job management logic which moves
/// jobs through the state machine. New jobs found through Scan methods are
/// held in memory until reaching FINISHED state.
class JobsList {
 private:
  bool valid;

  // List of jobs currently tracked in memory conveniently indexed by identifier.
  // TODO: It would be nice to remove it and use status files distribution among
  // subfolders in controldir.
  std::map<JobId,GMJobRef> jobs;         

  mutable Glib::RecMutex jobs_lock;

  GMJobQueue jobs_processing;   // List of jobs currently scheduled for processing

  GMJobQueue jobs_attention;    // List of jobs which need attention
  Arc::SimpleCondition jobs_attention_cond;

  GMJobQueue jobs_polling;      // List of jobs which need polling soon

  GMJobQueue jobs_wait_for_running; // List of jobs waiting for limit on running jobs


  time_t job_slow_polling_last;
  static time_t const job_slow_polling_period = 24UL*60UL*60UL; // todo: variable
  Glib::Dir* job_slow_polling_dir;

  // GM configuration
  const GMConfig& config;
  // Staging configuration
  StagingConfig staging_config;
  // Generator for handling data staging
  DTRGenerator dtr_generator;
  // Job description handler
  JobDescriptionHandler job_desc_handler;
  // number of jobs for every state
  int jobs_num[JOB_STATE_NUM];
  int jobs_scripts;
  // map of number of active jobs for each DN
  std::map<std::string, ZeroUInt> jobs_dn;
  // number of jobs currently in pending state
  int jobs_pending;

  // Add job into list. It is supposed to be called only for jobs which are not in main list.
  bool AddJob(const JobId &id,uid_t uid,gid_t gid,job_state_t state,const char* reason = NULL);

  bool AddJob(const JobId &id,uid_t uid,gid_t gid,const char* reason = NULL) {
    return AddJob(id, uid, gid, JOB_STATE_UNDEFINED, reason);
  }

  // Perform all actions necessary in case of job failure
  bool FailedJob(GMJobRef i,bool cancel);

  // Cleaning reference to running child process
  void CleanChildProcess(GMJobRef i);
  // Remove Job from list. All corresponding files are deleted and pointer is
  // advanced. If finished is false - job is not destroyed if it is FINISHED
  // If active is false - job is not destroyed if it is not UNDEFINED. Returns
  // false if external process is still running.
  //bool DestroyJob(iterator &i,bool finished=true,bool active=true);
  // Perform actions necessary in case job goes to/is in SUBMITTING/CANCELING state
  bool state_submitting(GMJobRef i,bool &state_changed);
  bool state_submitting_success(GMJobRef i,bool &state_changed,std::string local_id);
  bool state_canceling(GMJobRef i,bool &state_changed);
  bool state_canceling_success(GMJobRef i,bool &state_changed);
  // Same for PREPARING/FINISHING
  bool state_loading(GMJobRef i,bool &state_changed,bool up);
  // Get the state in which the job failed from .local file
  job_state_t JobFailStateGet(GMJobRef i);
  // Write the state in which the job failed to .local file
  bool JobFailStateRemember(GMJobRef i,job_state_t state,bool internal = true);
  // In case of job restart, recreates lists of input and output files taking
  // into account what was already transferred
  bool RecreateTransferLists(GMJobRef i);
  // Read into ids all jobs in the given dir except those already being handled
  bool ScanJobDescs(const std::string& cdir,std::list<JobFDesc>& ids) const;
  // Check and read into id information about job in the given dir 
  // (id has job id filled on entry) unless job is already handled
  bool ScanJobDesc(const std::string& cdir,JobFDesc& id);
  // Read into ids all jobs in the given dir with marks given by suffices
  // (corresponding to file suffixes) except those of jobs already handled
  bool ScanMarks(const std::string& cdir,const std::list<std::string>& suffices,std::list<JobFDesc>& ids);
  // Called after service restart to move jobs that were processing to a
  // restarting state
  bool RestartJobs(const std::string& cdir,const std::string& odir);
  // Release delegation after job finishes
  void UnlockDelegation(GMJobRef i);
  // Calculate job expiration time from last state change and configured lifetime
  time_t PrepareCleanupTime(GMJobRef i, time_t& keep_finished);
  // Read in information from .local file
  bool GetLocalDescription(GMJobRef i) const;
  // Modify job state, log that change and optionally log modification reson
  void SetJobState(GMJobRef i, job_state_t new_state, const char* reason = NULL);
  // Modify job state to set is as waiting on some condition or limit before
  // progressing to the next state
  void SetJobPending(GMJobRef i, const char* reason);
  // Update content of job proxy file with one stored in delegations store
  void UpdateJobCredentials(GMJobRef i);

  // Main job processing method. Analyze current state of job, perform
  // necessary actions and advance state or remove job if needed. Iterator 'i'
  // is advanced or erased inside this function.
  bool ActJob(GMJobRef& i);

  // Helper method for ActJob. Finishes processing of job.
  bool NextJob(GMJobRef i, job_state_t old_state, bool old_pending);

  // Helper method for ActJob. Finishes processing of job, removes it from list.
  bool DropJob(GMJobRef& i, job_state_t old_state, bool old_pending);

  enum ActJobResult {
    JobSuccess,
    JobFailed,
    JobDropped,
  };
  // ActJob() calls one of these methods depending on the state of the job.
  // Each ActJob*() returns processing result:
  //   JobSuccess - job was passed to other module (usually DTR) and does
  //      not require any additional processing. The module must later
  //      pass job to one of RequestAttention/RequestPolling/RequestSlowPolling
  //      queues. If the module fails to do that job may be lost.
  //      It is also possible to return JobSuccess if ActJob*() methods
  //      already passed job to one of the queues.
  //   JobFailed  - job processing failed. Job must be moved to FAILED.
  //      This result to be removed in a future.
  //   JobDropped - job does not need any further processing and should
  //      be removed from memory. This result to be removed in a future
  //      when automatic job unloading from RAM is implemented.
  ActJobResult ActJobUndefined(GMJobRef i);
  ActJobResult ActJobAccepted(GMJobRef i);
  ActJobResult ActJobPreparing(GMJobRef i);
  ActJobResult ActJobSubmitting(GMJobRef i);
  ActJobResult ActJobCanceling(GMJobRef i);
  ActJobResult ActJobInlrms(GMJobRef i);
  ActJobResult ActJobFinishing(GMJobRef i);
  ActJobResult ActJobFinished(GMJobRef i);
  ActJobResult ActJobDeleted(GMJobRef i);

  // Special processing method for job processing failure
  ActJobResult ActJobFailed(GMJobRef i);

  // Checks and processes user's request to cancel job.
  // Returns false if job was not modified (canceled or
  // failed) and true if canceling/modification took place.
  bool CheckJobCancelRequest(GMJobRef i);

  // Checks job state against continuation plugins.
  // Returns false if job is not allowed to continue.
  bool CheckJobContinuePlugins(GMJobRef i);

  // Call ActJob for all jobs in processing queue
  bool ActJobsProcessing(void);

  // Inform this instance that job with specified id needs immediate re-processing
  bool RequestReprocess(GMJobRef i);

  // Similar to RequestAttention but jobs does not need immediate attention.
  // These jobs will be processed when polling period elapses.
  // This method/queue to be removed when even driven processing implementation
  // becomes stable.
  bool RequestPolling(GMJobRef i);

  // Register job as waiting for limit for running jobs to be cleared.
  // This method does not have corresponding ActJobs*. Instead these jobs
  // are handled by ActJobsAttention if corresponding condition is met.
  bool RequestWaitForRunning(GMJobRef i);

  // Even slower polling. Typically once per 24 hours.
  // This queue is meant for FINISHED and DELETED jobs.
  // Jobs which are put into this queue are also removed from RAM and
  // queue content is backed by file/database.
  // There is no corresponding ActJobs*() method. Instead jobs put into
  // slow queue are handled in WaitAttention() while there is nothing
  // assigned for immediate processing.
  // Note: In current implementation this method does nothing because
  // <control_dir>/finished/ is used as slow queue.
  bool RequestSlowPolling(GMJobRef i);
  
  // Incrementally scan through old jobs in order to check for 
  // removal time
  // Returns true if scanning is going on, false if scanning cycle is over.
  bool ScanOldJobs(void);


  /// Class to run external processes (helper)
  class ExternalHelper {
   private:
    /// Command being run
    std::string command;
    /// Object representing running process
    Arc::Run *proc;
   public:
    ExternalHelper(const std::string &cmd);
    ~ExternalHelper();
    /// Start process if it is not running yet
    bool run(JobsList const& list);
    /// Stop process if it is running
    void stop();
  };

  /// Class for handling multiple external threads
  class ExternalHelpers: protected Arc::Thread {
   public:
    /// Construct instance from commands attached to jobs.
    ExternalHelpers(std::list<std::string> const& commands, JobsList const& jobs);
    /// Kill thread handling helpers and destroy this instance.
    ~ExternalHelpers();
    /// Start handling by spawning dedicated thread.
    void start();
   private:
    virtual void thread(void);
    std::list<ExternalHelper> helpers;
    JobsList const& jobs_list;
    Arc::SimpleCounter stop_cond;
    bool stop_request;
  };

  /// Associated external processes
  ExternalHelpers helpers;

  // Return iterator to object matching given id or null if not found
  GMJobRef FindJob(const JobId &id);

  bool HasJob(const JobId &id) const;

 public:
  static const int ProcessingQueuePriority = 3;
  static const int AttentionQueuePriority = 2;
  static const int WaitQueuePriority = 1;

  // Constructor.
  JobsList(const GMConfig& gmconfig);
  ~JobsList(void);
  operator bool(void) { return valid; };
  bool operator!(void) { return !valid; };

  // Information about jobs for external utilities
  // No of jobs in all active states from ACCEPTED and FINISHING
  int AcceptedJobs() const;
  // No of jobs in batch system or in process of submission to batch system
  bool RunningJobsLimitReached() const;
  // No of jobs in data staging
  //int ProcessingJobs() const;
  // No of jobs staging in data before job execution
  //int PreparingJobs() const;
  // No of jobs staging out data after job execution
  //int FinishingJobs() const;

  // Inform this instance that job with specified id needs attention
  bool RequestAttention(const JobId& id);
  bool RequestAttention(GMJobRef i);

  // Inform this instance that generic unscheduled attention is needed
  void RequestAttention();

  // Call ActJob for all current jobs
  bool ActJobs(void);

  // Call ActJob for all jobs for which RequestAttention and RequestWaitFor* - if condition allows - was called
  bool ActJobsAttention(void);

  // Call ActJob for all jobs for which RequestPolling was called
  bool ActJobsPolling(void);

  // Look for new or restarted jobs. Jobs are added to list with state UNDEFINED and requested for attention.
  bool ScanNewJobs(void);

  // Look for new job with specified id. Job is added to list with state UNDEFINED and requested for attention.
  bool ScanNewJob(const JobId& id);

  // Look for old job with specified id. Job is added to list with its current state and requested for attention.
  bool ScanOldJob(const JobId& id);

  // Pick jobs which have been marked for restarting, cancelling or cleaning
  bool ScanNewMarks(void);

  // Rearrange status files on service restart
  bool RestartJobs(void);

  // Send signals to external processes to shut down nicely (not implemented)
  void PrepareToDestroy(void);

  // Wait for attention request or polling time
  // While waiting may also perform slow scanning of old jobs
  void WaitAttention();




  // Class to be used in job scanning methods to filter out jobs by their id.
  class JobFilter {
  public:
    JobFilter() {};
    virtual ~JobFilter() {};
    // This method is called when jobs scanning needs to decide
    // if job to be picked up. Must return true for suitable job ids.
    virtual bool accept(const JobId &id) const = 0;
  };

  // Look for all jobs residing in specified control directories.
  // Fils ids with information about those jobs.
  // Uses filter to skip jobs which do not fit filter requirments.
  static bool ScanAllJobs(const std::string& cdir,std::list<JobFDesc>& ids, JobFilter const& filter);
  

  // Collect all jobs in all states and return references to their descriptions in alljobs.
  static bool GetAllJobs(const GMConfig& config, std::list<GMJobRef>& alljobs);

  // Collect all job ids in all states and return them in alljobs.
  static bool GetAllJobIds(const GMConfig& config, std::list<JobId>& alljobs);

  // Collect information about job with specified id. 
  // Returns valid reference if job was found.
  static GMJobRef GetJob(const GMConfig& config, const JobId& id);

  static int CountAllJobs(const GMConfig& config);

};

} // namespace ARex

#endif
