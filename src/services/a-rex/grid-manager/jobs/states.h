#ifndef GRID_MANAGER_STATES_H
#define GRID_MANAGER_STATES_H

#include <sys/types.h>
#include <list>
#include "../jobs/job.h"

/* default job ttl after finished - 1 week */
#define DEFAULT_KEEP_FINISHED (7*24*60*60)
/* default job ttr after deleted - 1 month */
#define DEFAULT_KEEP_DELETED (30*24*60*60)
/* default maximal number of jobs in memory */
#define DEFAULT_MAX_JOBS (100)
/* default maximal allowed amount of reruns */
#define DEFAULT_JOB_RERUNS (5)
/* not used */
#define DEFAULT_DISKSPACE (200*1024L*1024L)

class JobUser;
class JobLocalDescription;
class ContinuationPlugins;
class RunPlugin;

/*
typedef struct {
  job_state_t id;
  char* str;
  char mail_flag;
} job_state_rec_t;
*/

/*
  List of jobs. This object is cross-linked to JobUser object, which
  represents owner of these jobs.
  Static members of this class store global parameters. In a future
  some of them can become user/job list specific.
*/
class JobsList {
 public:
  typedef std::list<JobDescription>::iterator iterator;
 private:
  /* these static members should be protected by lock, but job
     status change is single threaded, so not yet */
  /* number of jobs for every state */
  static long int jobs_num[JOB_STATE_NUM];
  /* maximal allowed values */
  static long int max_jobs_running;
  static long int max_jobs_processing;
  static long int max_jobs_processing_emergency;
  static long int max_jobs;
  static unsigned long long int min_speed;
  static time_t min_speed_time;
  static unsigned long long int min_average_speed;
  static time_t max_inactivity_time;
  static int max_downloads;
  static bool use_secure_transfer;
  static bool use_passive_transfer;
  static bool use_local_transfer;
  static bool cache_registration;
  static unsigned int wakeup_period;
  std::list<JobDescription> jobs;
  JobUser *user;
  ContinuationPlugins *plugins;
  /* Add job into list without checking if it is already there
     'i' will be set to iterator pointing at new job */
  bool AddJobNoCheck(const JobId &id,iterator &i,uid_t uid,gid_t gid);
  bool AddJobNoCheck(const JobId &id,uid_t uid,gid_t gid);
  /* Perform all actions necessary in case of job failure */
  bool FailedJob(const iterator &i);
  /*
     Remove Job from list. All corresponding files are deleted 
     and pointer is advanced. 
     if finished is not set - job is in not destroyed if it is FINISHED
     if active is not set - job is not destroyed if it is not UNDEFINED
  */
  bool DestroyJob(iterator &i,bool finished=true,bool active=true);
  /* Perform actions necessary in case job goes to/is in
     SUBMITTING/CANCELING state */
  bool state_submiting(const iterator &i,bool &state_changed,bool cancel=false);
  /* Same for PREPARING/FINISHING */
  bool state_loading(const iterator &i,bool &state_changed,bool up);
  bool JobPending(JobsList::iterator &i);
  job_state_t JobFailStateGet(const iterator &i);
  bool JobFailStateRemember(const iterator &i,job_state_t state);
  bool RecreateTransferLists(const JobsList::iterator &i);
 public:
  /* Constructor. 'user' conatins associated user */ 
  JobsList(JobUser &user,ContinuationPlugins &plugins);
  ~JobsList(void);
  iterator FindJob(const JobId &id);
  iterator begin(void) { return jobs.begin(); };
  iterator end(void) { return jobs.end(); };
  size_t size(void) const { return jobs.size(); };
  static void SetMaxJobs(int max = -1,int max_running = -1) {
    max_jobs=max;
    max_jobs_running=max_running;
  };
  static void SetMaxJobsLoad(int max_processing = -1,int max_processing_emergency = 1,int max_down = -1) {
    max_jobs_processing=max_processing;
    max_jobs_processing_emergency=max_processing_emergency;
    max_downloads=max_down;
  };
  static void GetMaxJobs(int &max,int &max_running) {
    max=max_jobs;
    max_running=max_jobs_running;
  };
  static void GetMaxJobsLoad(int &max_processing,int &max_processing_emergency,int &max_down) {
    max_processing=max_jobs_processing;
    max_processing_emergency=max_jobs_processing_emergency;
    max_down=max_downloads;
  };
  static void SetSpeedControl(unsigned long long int min=0,time_t min_time=300,unsigned long long int min_average=0,time_t max_time=300) {
    min_speed = min;
    min_speed_time = min_time;
    min_average_speed = min_average;
    max_inactivity_time = max_time;
  };
  static void SetSecureTransfer(bool val) {
    use_secure_transfer=val;
  };
  static void SetPassiveTransfer(bool val) {
    use_passive_transfer=val;
  };
  static void SetLocalTransfer(bool val) {
    use_local_transfer=val;
  };
  static void SetCacheRegistration(bool val) {
    cache_registration=val;
  };
  static bool CacheRegistration(void) { return cache_registration; };
  static void SetWakeupPeriod(unsigned int t) { wakeup_period=t; };
  static unsigned int WakeupPeriod(void) { return wakeup_period; };
 /* Add job to list */
  bool AddJob(JobUser &user,const JobId &id,uid_t uid,gid_t gid);
  bool AddJob(const JobId &id,uid_t uid,gid_t gid);
  /* Analyze current state of job, perform necessary actions and
     advance state or remove job if needed. Iterator 'i' is 
     advanced inside this function */
  bool ActJob(const JobId &id,bool hard_job = false); /* analyze job */
  bool ActJob(iterator &i,bool hard_job = false); /* analyze job */
  bool ActJobs(bool hard_job = false); /* analyze all jobs */
  /* Look for new (or old FINISHED) jobs. Jobs are added to list
     with state undefined */
  bool ScanNewJobs(bool hard_job = false);
  /*
    Destroy all jobs in list according to 'finished' an 'active'.
    (See DestroyJob).
  */
  bool DestroyJobs(bool finished=true,bool active=true);
  /* (See GetLocalDescription of JobDescription object) */
  bool GetLocalDescription(const JobsList::iterator &i);
  void ActJobUndefined(iterator &i,bool hard_job,bool& once_more,bool& delete_job,bool& job_error,bool& state_changed);
  void ActJobAccepted(iterator &i,bool hard_job,bool& once_more,bool& delete_job,bool& job_error,bool& state_changed);
  void ActJobPreparing(iterator &i,bool hard_job,bool& once_more,bool& delete_job,bool& job_error,bool& state_changed);
  void ActJobSubmiting(iterator &i,bool hard_job,bool& once_more,bool& delete_job,bool& job_error,bool& state_changed);
  void ActJobCanceling(iterator &i,bool hard_job,bool& once_more,bool& delete_job,bool& job_error,bool& state_changed);
  void ActJobInlrms(iterator &i,bool hard_job,bool& once_more,bool& delete_job,bool& job_error,bool& state_changed);
  void ActJobFinishing(iterator &i,bool hard_job,bool& once_more,bool& delete_job,bool& job_error,bool& state_changed);
  void ActJobFinished(iterator &i,bool hard_job,bool& once_more,bool& delete_job,bool& job_error,bool& state_changed);
  void ActJobDeleted(iterator &i,bool hard_job,bool& once_more,bool& delete_job,bool& job_error,bool& state_changed);

};

#endif


