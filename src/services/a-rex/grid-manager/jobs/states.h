#ifndef GRID_MANAGER_STATES_H
#define GRID_MANAGER_STATES_H

#include <sys/types.h>
#include <list>
#include <glibmm.h>

#include <arc/URL.h>

#include "../jobs/job.h"
#include "../conf/environment.h"

class JobUser;
class JobLocalDescription;
class ContinuationPlugins;
class RunPlugin;
class JobsListConfig;
class JobFDesc;
class DTRGenerator;

/*
  List of jobs. This object is cross-linked to JobUser object, which
  represents owner of these jobs. This class contains the main job
  management logic which moves jobs through the state machine.
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
  Glib::Dir* old_dir;
  DTRGenerator* dtr_generator;
  /* Add job into list without checking if it is already there
     'i' will be set to iterator pointing at new job */
  bool AddJobNoCheck(const JobId &id,iterator &i,uid_t uid,gid_t gid);
  bool AddJobNoCheck(const JobId &id,uid_t uid,gid_t gid);
  /* Perform all actions necessary in case of job failure */
  bool FailedJob(const iterator &i,bool cancel);
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
  /// Check if job is allowed to progress to a staging state. up is true
  /// for uploads (FINISHING) and false for downloads (PREPARING).
  bool CanStage(const JobsList::iterator &i, const JobsListConfig& jcfg, bool up);
  bool JobPending(JobsList::iterator &i);
  job_state_t JobFailStateGet(const iterator &i);
  bool JobFailStateRemember(const iterator &i,job_state_t state,bool internal = true);
  bool RecreateTransferLists(const JobsList::iterator &i);
  bool ScanJobs(const std::string& cdir,std::list<JobFDesc>& ids);
  bool ScanMarks(const std::string& cdir,const std::list<std::string>& suffices,std::list<JobFDesc>& ids);
  bool RestartJobs(const std::string& cdir,const std::string& odir);
  bool RestartJob(const std::string& cdir,const std::string& odir,const std::string& id);
 public:
  /* Constructor. 'user' contains associated user */ 
  JobsList(JobUser &user,ContinuationPlugins &plugins);
  ~JobsList(void);
  void SetDataGenerator(DTRGenerator* generator) { dtr_generator = generator; };
  iterator FindJob(const JobId &id);
  iterator begin(void) { return jobs.begin(); };
  iterator end(void) { return jobs.end(); };
  size_t size(void) const { return jobs.size(); };
  iterator erase(iterator& i) { return jobs.erase(i); };

  /* Analyze current state of job, perform necessary actions and
     advance state or remove job if needed. Iterator 'i' is 
     advanced inside this function */
  bool ActJob(const JobId &id); /* analyze job */
  bool ActJob(iterator &i); /* analyze job */
  void CalculateShares();
  bool ActJobs(void); /* analyze all jobs */
  /* Look for new (or old FINISHED) jobs. Jobs are added to list
     with state undefined */
  bool ScanNewJobs(void);
  bool ScanAllJobs(void);
  /* Picks jobs which have attention marks. */
  bool ScanNewMarks(void);
  /* Picks jobs from finished. Returns false if failed or scanning finished. */
  bool ScanOldJobs(int max_scan_time,int max_scan_jobs);
  /* Rearange status files on service restart */
  bool RestartJobs(void);
  void UnlockDelegation(JobsList::iterator &i);
  /*
    Destroy all jobs in list according to 'finished' an 'active'.
    (See DestroyJob).
  */
  bool DestroyJobs(bool finished=true,bool active=true);
  /* (See GetLocalDescription of JobDescription object) */
  bool GetLocalDescription(const JobsList::iterator &i);
  void ActJobUndefined(iterator &i,bool& once_more,bool& delete_job,bool& job_error,bool& state_changed);
  void ActJobAccepted(iterator &i,bool& once_more,bool& delete_job,bool& job_error,bool& state_changed);
  void ActJobPreparing(iterator &i,bool& once_more,bool& delete_job,bool& job_error,bool& state_changed);
  void ActJobSubmitting(iterator &i,bool& once_more,bool& delete_job,bool& job_error,bool& state_changed);
  void ActJobCanceling(iterator &i,bool& once_more,bool& delete_job,bool& job_error,bool& state_changed);
  void ActJobInlrms(iterator &i,bool& once_more,bool& delete_job,bool& job_error,bool& state_changed);
  void ActJobFinishing(iterator &i,bool& once_more,bool& delete_job,bool& job_error,bool& state_changed);
  void ActJobFinished(iterator &i,bool& once_more,bool& delete_job,bool& job_error,bool& state_changed);
  void ActJobDeleted(iterator &i,bool& once_more,bool& delete_job,bool& job_error,bool& state_changed);
  void PrepareToDestroy(void);

};

#endif
