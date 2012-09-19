#ifndef GRID_MANAGER_STATES_H
#define GRID_MANAGER_STATES_H

#include <sys/types.h>
#include <list>

#include "job.h"

class JobUser;
class ContinuationPlugins;
class JobsListConfig;
class JobFDesc;
class DTRGenerator;

// List of jobs. This object is cross-linked to JobUser object, which
// represents owner of these jobs. This class contains the main job management
// logic which moves jobs through the state machine. New jobs found through
// Scan methods are held in memory until reaching FINISHED state.
class JobsList {
 public:
  typedef std::list<JobDescription>::iterator iterator;
 private:
  // List of jobs currently tracked in memory
  std::list<JobDescription> jobs;
  // counters of share for preparing/finishing states
  std::map<std::string, int> preparing_job_share;
  std::map<std::string, int> finishing_job_share;
  // current max share for preparing/finishing
  std::map<std::string, int> preparing_max_share;
  std::map<std::string, int> finishing_max_share;
  // User who owns these jobs
  JobUser *user;
  // Plugins configured to run at certain state changes
  ContinuationPlugins *plugins;
  // Dir containing finished/deleted jobs which is scanned in ScanOldJobs.
  // Since this can happen over multiple calls a pointer is kept as a member
  // variable so scanning picks up where it finished last time.
  Glib::Dir* old_dir;
  // Generator for handling data staging
  DTRGenerator* dtr_generator;

  // Add job into list without checking if it is already there.
  // 'i' will be set to iterator pointing at new job
  bool AddJobNoCheck(const JobId &id,iterator &i,uid_t uid,gid_t gid);
  // Add job into list without checking if it is already there
  bool AddJobNoCheck(const JobId &id,uid_t uid,gid_t gid);
  // Perform all actions necessary in case of job failure
  bool FailedJob(const iterator &i,bool cancel);
  // Remove Job from list. All corresponding files are deleted and pointer is
  // advanced. If finished is false - job is not destroyed if it is FINISHED
  // If active is false - job is not destroyed if it is not UNDEFINED. Returns
  // false if external process is still running.
  bool DestroyJob(iterator &i,bool finished=true,bool active=true);
  // Perform actions necessary in case job goes to/is in SUBMITTING/CANCELING state
  bool state_submitting(const iterator &i,bool &state_changed,bool cancel=false);
  // Same for PREPARING/FINISHING
  bool state_loading(const iterator &i,bool &state_changed,bool up,bool &retry);
  // Check if job is allowed to progress to a staging state. up is true
  // for uploads (FINISHING) and false for downloads (PREPARING).
  bool CanStage(const JobsList::iterator &i, const JobsListConfig& jcfg, bool up);
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
  // Read into ids all jobs in the given dir with marks given by suffices
  // (corresponding to file suffixes)
  bool ScanMarks(const std::string& cdir,const std::list<std::string>& suffices,std::list<JobFDesc>& ids);
  // Called after service restart to move jobs that were processing to a
  // restarting state
  bool RestartJobs(const std::string& cdir,const std::string& odir);
  // Choose the share for a new job (for old staging only)
  void ChooseShare(JobsList::iterator& i, const JobsListConfig& jcfg, JobUser* user);
  // Calculate share information for data staging (downloader/uploader staging
  // only), in DTR this is done internally
  void CalculateShares();
  // Release delegation after job finishes
  void UnlockDelegation(JobsList::iterator &i);
  // Calculate job expiration time from last state change and configured lifetime
  time_t PrepareCleanupTime(JobsList::iterator &i, time_t& keep_finished);
  // Read in information from .local file
  bool GetLocalDescription(const JobsList::iterator &i);

  // Main job processing method. Analyze current state of job, perform
  // necessary actions and advance state or remove job if needed. Iterator 'i'
  // is advanced or erased inside this function.
  bool ActJob(iterator &i);

  // ActJob() calls one of these methods depending on the state of the job.
  // Parameters:
  //   once_more - if true then ActJob should process this job again
  //   delete_job - if true then ActJob should delete the job
  //   job_error - if true then an error happened in this processing state
  //   state_changed - if true then the job state was changed
  void ActJobUndefined(iterator &i,bool& once_more,bool& delete_job,bool& job_error,bool& state_changed);
  void ActJobAccepted(iterator &i,bool& once_more,bool& delete_job,bool& job_error,bool& state_changed);
  void ActJobPreparing(iterator &i,bool& once_more,bool& delete_job,bool& job_error,bool& state_changed);
  void ActJobSubmitting(iterator &i,bool& once_more,bool& delete_job,bool& job_error,bool& state_changed);
  void ActJobCanceling(iterator &i,bool& once_more,bool& delete_job,bool& job_error,bool& state_changed);
  void ActJobInlrms(iterator &i,bool& once_more,bool& delete_job,bool& job_error,bool& state_changed);
  void ActJobFinishing(iterator &i,bool& once_more,bool& delete_job,bool& job_error,bool& state_changed);
  void ActJobFinished(iterator &i,bool& once_more,bool& delete_job,bool& job_error,bool& state_changed);
  void ActJobDeleted(iterator &i,bool& once_more,bool& delete_job,bool& job_error,bool& state_changed);

 public:
  // Constructor. 'user' contains associated user
  JobsList(JobUser &user,ContinuationPlugins &plugins);
  // std::list methods for using JobsList like a regular list
  iterator begin(void) { return jobs.begin(); };
  iterator end(void) { return jobs.end(); };
  size_t size(void) const { return jobs.size(); };
  iterator erase(iterator& i) { return jobs.erase(i); };

  // Return iterator to object matching given id or jobs.end() if not found
  iterator FindJob(const JobId &id);

  // Set DTR Generator for data staging
  void SetDataGenerator(DTRGenerator* generator) { dtr_generator = generator; };
  // Call ActJob for all current jobs
  bool ActJobs(void);
  // Look for new or restarted jobs. Jobs are added to list with state UNDEFINED
  bool ScanNewJobs(void);
  // Collect all jobs in all states
  bool ScanAllJobs(void);
  // Pick jobs which have been marked for restarting, cancelling or cleaning
  bool ScanNewMarks(void);
  // Look for finished or deleted jobs and process them. Jobs which are
  // restarted will be added back into the main processing loop. This method
  // can be limited in the time it can run for and number of jobs it can scan.
  // It returns false if failed or scanning finished.
  bool ScanOldJobs(int max_scan_time,int max_scan_jobs);
  // Rearrange status files on service restart
  bool RestartJobs(void);
  // Send signals to external processes to shut down nicely (not implemented)
  void PrepareToDestroy(void);
};

#endif
