#ifndef GRID_MANAGER_STATES_H
#define GRID_MANAGER_STATES_H

#include <sys/types.h>
#include <list>
#include "../jobs/job.h"
#include "../conf/environment.h"

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
/* default maximum down/upload retries */
#define DEFAULT_MAX_RETRIES (10)

class JobUser;
class JobLocalDescription;
class ContinuationPlugins;
class RunPlugin;
class JobsListConfig;
class JobFDesc;

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
  std::list<JobDescription> jobs;
 /* counters of share for preparing/finishing states */
  std::map<std::string, int> preparing_job_share;
  std::map<std::string, int> finishing_job_share;
 /* current max share for preparing/finishing */
  std::map<std::string, int> preparing_max_share;
  std::map<std::string, int> finishing_max_share;
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
  bool state_submitting(const iterator &i,bool &state_changed,bool cancel=false);
  /* Same for PREPARING/FINISHING */
  bool state_loading(const iterator &i,bool &state_changed,bool up,bool &retry);
  bool JobPending(JobsList::iterator &i);
  job_state_t JobFailStateGet(const iterator &i);
  bool JobFailStateRemember(const iterator &i,job_state_t state);
  bool RecreateTransferLists(const JobsList::iterator &i);
  bool ScanJobs(const std::string& cdir,std::list<JobFDesc>& ids);
  bool RestartJobs(const std::string& cdir,const std::string& odir);
 public:
  /* Constructor. 'user' contains associated user */ 
  JobsList(JobUser &user,ContinuationPlugins &plugins);
  ~JobsList(void);
  iterator FindJob(const JobId &id);
  iterator begin(void) { return jobs.begin(); };
  iterator end(void) { return jobs.end(); };
  size_t size(void) const { return jobs.size(); };

 /* Add job to list */
  bool AddJob(JobUser &user,const JobId &id,uid_t uid,gid_t gid);
  bool AddJob(const JobId &id,uid_t uid,gid_t gid);
  /* Analyze current state of job, perform necessary actions and
     advance state or remove job if needed. Iterator 'i' is 
     advanced inside this function */
  bool ActJob(const JobId &id,bool hard_job = false); /* analyze job */
  bool ActJob(iterator &i,bool hard_job = false); /* analyze job */
  void CalculateShares();
  bool ActJobs(bool hard_job = false); /* analyze all jobs */
  /* Look for new (or old FINISHED) jobs. Jobs are added to list
     with state undefined */
  bool ScanNewJobs(bool hard_job = false);
  /* Rearange status files on service restart */
  bool RestartJobs(void);
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
  void ActJobSubmitting(iterator &i,bool hard_job,bool& once_more,bool& delete_job,bool& job_error,bool& state_changed);
  void ActJobCanceling(iterator &i,bool hard_job,bool& once_more,bool& delete_job,bool& job_error,bool& state_changed);
  void ActJobInlrms(iterator &i,bool hard_job,bool& once_more,bool& delete_job,bool& job_error,bool& state_changed);
  void ActJobFinishing(iterator &i,bool hard_job,bool& once_more,bool& delete_job,bool& job_error,bool& state_changed);
  void ActJobFinished(iterator &i,bool hard_job,bool& once_more,bool& delete_job,bool& job_error,bool& state_changed);
  void ActJobDeleted(iterator &i,bool hard_job,bool& once_more,bool& delete_job,bool& job_error,bool& state_changed);

};


class ZeroUInt {
 private:
  unsigned int value_;
 public:
  ZeroUInt(void):value_(0) { };
  ZeroUInt(unsigned int v):value_(v) { };
  ZeroUInt(const ZeroUInt& v):value_(v.value_) { };
  ZeroUInt& operator=(unsigned int v) { value_=v; };
  ZeroUInt& operator=(const ZeroUInt& v) { value_=v.value_; };
  ZeroUInt& operator++(void) { ++value_; return *this; };
  ZeroUInt operator++(int) { ZeroUInt temp(value_); ++value_; return temp; };
  ZeroUInt& operator--(void) { if(value_) --value_; return *this; };
  ZeroUInt operator--(int) { ZeroUInt temp(value_); if(value_) --value_; return temp; };
  operator unsigned int(void) const { return value_; };
};

/**
 * Class to represent information read from configuration.
 */
class JobsListConfig {
 friend class JobsList;
 private:
  /* number of jobs for every state */
  int jobs_num[JOB_STATE_NUM];
  /* map of number of active jobs for each DN */
  std::map<std::string, ZeroUInt> jobs_dn;
  int jobs_pending;
  /* maximal allowed values */
  int max_jobs_running;
  int max_jobs_total;
  int max_jobs_processing;
  int max_jobs_processing_emergency;
  int max_jobs;
  int max_jobs_per_dn;
  unsigned int max_processing_share;
  std::string share_type;
  unsigned long long int min_speed;
  time_t min_speed_time;
  unsigned long long int min_average_speed;
  time_t max_inactivity_time;
  int max_downloads;
  int max_retries;
  bool use_secure_transfer;
  bool use_passive_transfer;
  bool use_local_transfer;
  unsigned int wakeup_period;
  std::string preferred_pattern;
  /* the list of shares with defined limits */
  std::map<std::string, int> limited_share;
 public:
  JobsListConfig(void);
  void SetMaxJobs(int max = -1,int max_running = -1, int max_per_dn = -1, int max_total = -1) {
    max_jobs=max;
    max_jobs_running=max_running;
    max_jobs_per_dn=max_per_dn;
    max_jobs_total=max_total;
  }
  void SetMaxJobsLoad(int max_processing = -1,int max_processing_emergency = 1,int max_down = -1) {
    max_jobs_processing=max_processing;
    max_jobs_processing_emergency=max_processing_emergency;
    max_downloads=max_down;
  }
  void GetMaxJobs(int &max,int &max_running,int &max_per_dn,int &max_total) const {
    max=max_jobs;
    max_running=max_jobs_running;
    max_per_dn=max_jobs_per_dn;
    max_total=max_jobs_total;
  }
  void GetMaxJobsLoad(int &max_processing,int &max_processing_emergency,int &max_down) const {
    max_processing=max_jobs_processing;
    max_processing_emergency=max_jobs_processing_emergency;
    max_down=max_downloads;
  }
  void SetSpeedControl(unsigned long long int min=0,time_t min_time=300,unsigned long long int min_average=0,time_t max_time=300) {
    min_speed = min;
    min_speed_time = min_time;
    min_average_speed = min_average;
    max_inactivity_time = max_time;
  }
  void GetSpeedControl(unsigned long long int& min, time_t& min_time, unsigned long long int& min_average, time_t& max_time) const {
    min = min_speed;
    min_time = min_speed_time;
    min_average = min_average_speed;
    max_time = max_inactivity_time;
  }
  void SetSecureTransfer(bool val) {
    use_secure_transfer=val;
  }
  bool GetSecureTransfer() const {
    return use_secure_transfer;
  }
  void SetPassiveTransfer(bool val) {
    use_passive_transfer=val;
  }
  bool GetPassiveTransfer() const {
    return use_passive_transfer;
  }
  void SetLocalTransfer(bool val) {
    use_local_transfer=val;
  }
  bool GetLocalTransfer() const {
    return use_local_transfer;
  }
  void SetWakeupPeriod(unsigned int t) {
    wakeup_period=t;
  }
  unsigned int WakeupPeriod(void) const {
    return wakeup_period;
  }
  void SetMaxRetries(int r) {
    max_retries = r;
  }
  int MaxRetries() const {
    return max_retries;
  }
  void SetPreferredPattern(const std::string& pattern) {
    preferred_pattern = pattern;
  }
  std::string GetPreferredPattern() const {
    return preferred_pattern;
  }
  void SetTransferShare(unsigned int max_share, std::string type){
    max_processing_share = max_share;
    share_type = type;
  };
  bool AddLimitedShare(std::string share_name, unsigned int share_limit) {
    if(max_processing_share == 0)
      return false;
    if(share_limit == 0)
      share_limit = max_processing_share;
    limited_share[share_name] = share_limit;
    return true;
  }
};

#endif


