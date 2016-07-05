#ifndef GRID_MANAGER_STATES_H
#define GRID_MANAGER_STATES_H

#include <sys/types.h>
#include <list>
#include <glib.h>

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
 public:
  typedef std::list<GMJob>::iterator iterator;
 private:
  bool valid;

  std::list<GMJob> jobs;              // List of jobs currently tracked in memory

  std::list<JobId> jobs_processing;   // List of jobs currently scheduled for processing
  Glib::Mutex jobs_processing_lock;

  std::list<JobId> jobs_attention;    // List of jobs which need attention
  Glib::Mutex jobs_attention_lock;
  Arc::SimpleCondition jobs_attention_cond;

  std::list<JobId> jobs_polling;      // List of jobs which need polling soon
  Glib::Mutex jobs_polling_lock;

  std::list<JobId> jobs_wait_for_running; // List of jobs waiting for limit on running jobs
  Glib::Mutex jobs_wait_for_running_lock;


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
  // map of number of active jobs for each DN
  std::map<std::string, ZeroUInt> jobs_dn;
  // number of jobs currently in pending state
  int jobs_pending;

  // Add job into list without checking if it is already there.
  // 'i' will be set to iterator pointing at new job
  bool AddJobNoCheck(const JobId &id,iterator &i,uid_t uid,gid_t gid,job_state_t state = JOB_STATE_UNDEFINED);

  // Add job into list without checking if it is already there
  bool AddJobNoCheck(const JobId &id,uid_t uid,gid_t gid,job_state_t state = JOB_STATE_UNDEFINED);

  // Perform all actions necessary in case of job failure
  bool FailedJob(const iterator &i,bool cancel);

  // Remove Job from list. All corresponding files are deleted and pointer is
  // advanced. If finished is false - job is not destroyed if it is FINISHED
  // If active is false - job is not destroyed if it is not UNDEFINED. Returns
  // false if external process is still running.
  //bool DestroyJob(iterator &i,bool finished=true,bool active=true);
  // Perform actions necessary in case job goes to/is in SUBMITTING/CANCELING state
  bool state_submitting(const iterator &i,bool &state_changed);
  bool state_canceling(const iterator &i,bool &state_changed);
  // Same for PREPARING/FINISHING
  bool state_loading(const iterator &i,bool &state_changed,bool up);
  // Returns true if job is waiting on some condition or limit before
  // progressing to the next state
  bool JobPending(JobsList::iterator &i);
  // Get the state in which the job failed from .local file
  job_state_t JobFailStateGet(const iterator &i);
  // Write the state in which the job failed to .local file
  bool JobFailStateRemember(const iterator &i,job_state_t state,bool internal = true);
  // In case of job restart, recreates lists of input and output files taking
  // into account what was already transferred
  bool RecreateTransferLists(const JobsList::iterator &i);
  // Read into ids all jobs in the given dir
  bool ScanJobs(const std::string& cdir,std::list<JobFDesc>& ids);
  // Check and read into id nformation about job in the given dir (id has job if filled on entry)
  bool ScanJob(const std::string& cdir,JobFDesc& id);
  // Read into ids all jobs in the given dir with marks given by suffices
  // (corresponding to file suffixes)
  bool ScanMarks(const std::string& cdir,const std::list<std::string>& suffices,std::list<JobFDesc>& ids);
  // Called after service restart to move jobs that were processing to a
  // restarting state
  bool RestartJobs(const std::string& cdir,const std::string& odir);
  // Release delegation after job finishes
  void UnlockDelegation(JobsList::iterator &i);
  // Calculate job expiration time from last state change and configured lifetime
  time_t PrepareCleanupTime(JobsList::iterator &i, time_t& keep_finished);
  // Read in information from .local file
  bool GetLocalDescription(const JobsList::iterator &i);
  // Modify job state, log that change and optionally log modification reson
  void SetJobState(JobsList::iterator &i, job_state_t new_state, const char* reason = NULL);
  // Update content of job proxy file with one stored in delegations store
  void UpdateJobCredentials(JobsList::iterator &i);

  // Main job processing method. Analyze current state of job, perform
  // necessary actions and advance state or remove job if needed. Iterator 'i'
  // is advanced or erased inside this function.
  bool ActJob(iterator &i);

  // Helper method for ActJob. Finishes processing of job and advances iterator to next one.
  iterator NextJob(iterator i, job_state_t old_state, bool old_pending);

  // Helper method for ActJob. Finishes processing of job, removes it from list and advances iterator to next one.
  iterator DropJob(iterator i, job_state_t old_state, bool old_pending);

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
  ActJobResult ActJobUndefined(iterator i);
  ActJobResult ActJobAccepted(iterator i);
  ActJobResult ActJobPreparing(iterator i);
  ActJobResult ActJobSubmitting(iterator i);
  ActJobResult ActJobCanceling(iterator i);
  ActJobResult ActJobInlrms(iterator i);
  ActJobResult ActJobFinishing(iterator i);
  ActJobResult ActJobFinished(iterator i);
  ActJobResult ActJobDeleted(iterator i);

  // Special processing method for job processing failure
  ActJobResult ActJobFailed(iterator i);

  // Checks and processes user's request to cancel job.
  // Returns false if job was not modified (canceled or
  // failed) and true if canceling/modification took place.
  bool CheckJobCancelRequest(JobsList::iterator i);

  // Checks job state against continuation plugins.
  // Returns false if job is not allowed to continue.
  bool CheckJobContinuePlugins(JobsList::iterator i);

  // Call ActJob for all jobs in processing queue
  bool ActJobsProcessing(void);

  // Inform this instance that job with specified id needs immediate re-processing
  bool RequestReprocess(const JobId& id);

  // Similar to RequestAttention but jobs does not need immediate attention.
  // These jobs will be processed when polling period elapses.
  // This method/queue to be removed when even driven processing implementation
  // becomes stable.
  bool RequestPolling(const JobId& id);

  // Register job as waiting for limit for running jobs to be cleared.
  // This method does not have corresponding ActJobs*. Instead these jobs
  // are handled by ActJobsAttention if corresponding condition is met.
  bool RequestWaitForRunning(const JobId& id);

  // Even slower polling. Typically once per 24 hours.
  // This queue is meant for FINISHED and DELETED jobs.
  // Jobs which are put into this queue are also removed from RAM and
  // queue content is backed by file/database.
  // There is no corresponding ActJobs*() method. Instead jobs put into
  // slow queue are handled in WaitAttention() while there is nothing
  // assigned for immediate processing.
  // Note: In current implementation this method does nothing because
  // <control_dir>/finished/ is used as slow queue.
  bool RequestSlowPolling(const JobId& id);
  
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
    bool run(JobsList& list);
    /// Stop process if it is running
    void stop();
  };

  friend class ExternalHelper;

  /// List of associated external processes
  std::list<ExternalHelper> helpers;

 public:
  // Constructor.
  JobsList(const GMConfig& gmconfig);
  ~JobsList(void);
  operator bool(void) { return valid; };
  bool operator!(void) { return !valid; };
  // std::list methods for using JobsList like a regular list
  iterator begin(void) { return jobs.begin(); };
  iterator end(void) { return jobs.end(); };
  size_t size(void) const { return jobs.size(); };
  iterator erase(iterator& i) { return jobs.erase(i); };

  // Return iterator to object matching given id or jobs.end() if not found
  iterator FindJob(const JobId &id);
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

  // Inform this instance that generic unscheduled attention is needed
  void RequestAttention();

  // Call ActJob for all current jobs
  bool ActJobs(void);

  // Call ActJob for all jobs for which RequestAttention and RequestWaitFor* - if condition allows - was called
  bool ActJobsAttention(void);

  // Call ActJob for all jobs for which RequestPolling was called
  bool ActJobsPolling(void);

  // Look for new or restarted jobs. Jobs are added to list with state UNDEFINED
  bool ScanNewJobs(void);

  // Look for new job with specified id. Job is added to list with state UNDEFINED
  bool ScanNewJob(const JobId& id);

  // Look for old job with specified id. Job is added to list with its current state
  bool ScanOldJob(const JobId& id);

  // Collect all jobs in all states
  bool ScanAllJobs(void);
  // Pick jobs which have been marked for restarting, cancelling or cleaning
  bool ScanNewMarks(void);

  // Add job with specified id. 
  // Returns true if job was found and added.
  // TODO: Only used in gm-jobs - remove.
  bool AddJob(const JobId& id);

  // Rearrange status files on service restart
  bool RestartJobs(void);

  // Send signals to external processes to shut down nicely (not implemented)
  void PrepareToDestroy(void);

  // Wait for attention request or polling time
  // While waiting may also perform slow scanning of old jobs
  void WaitAttention();

  /// Start/restart all helper processes
  bool RunHelpers();

};

} // namespace ARex

#endif
