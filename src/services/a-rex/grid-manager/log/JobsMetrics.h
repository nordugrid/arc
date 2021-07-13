/* write essential inforamtion about job started/finished */
#ifndef __GM_JOBS_METRICS_H__
#define __GM_JOBS_METRICS_H__

#include <string>
#include <list>
#include <fstream>
#include <ctime>

#include <arc/Run.h>

#include "../jobs/GMJob.h"

#define GMETRIC_STATERATE_UPDATE_INTERVAL 5//to-fix this value could be set in arc.conf to be tailored to site


namespace ARex {
  class JobStateList {
    /*Holds sucess or fail of last 100 jobs */

  private:
    class JobNode {

    public:

      std::string job_id;
      bool isfailed;

      JobNode(bool _isfailed=false, std::string _job_id="");
      ~JobNode(void);

    };


  private:
    const int limit;

    std::list<JobNode> nodes;

    JobStateList::JobNode* NodeInList(std::string _job_id);

  public:
    int failures;

    void setFailure(bool _isfailed, std::string _job_id);

    JobStateList(int _limit);
    ~JobStateList(void);


  };




class JobsMetrics {
 private:
  Glib::RecMutex lock;
  bool enabled;
  std::string config_filename;
  std::string tool_path;

  time_t time_lastupdate;

  unsigned long long int job_fail_counter;
  unsigned long long int jobs_in_state[JOB_STATE_UNDEFINED];
  unsigned long long int jobs_state_old_new[JOB_STATE_UNDEFINED+1][JOB_STATE_UNDEFINED];
  unsigned long long int jobs_state_accum[JOB_STATE_UNDEFINED+1];
  unsigned long long int jobs_state_accum_last[JOB_STATE_UNDEFINED+1];
  double jobs_rate[JOB_STATE_UNDEFINED];

  bool fail_changed;
  bool jobs_in_state_changed[JOB_STATE_UNDEFINED];
  bool jobs_state_old_new_changed[JOB_STATE_UNDEFINED+1][JOB_STATE_UNDEFINED];
  bool jobs_rate_changed[JOB_STATE_UNDEFINED];

  //id,state
  std::map<std::string,job_state_t> jobs_state_old_map;
  std::map<std::string,job_state_t> jobs_state_new_map;
  
  Arc::Run *proc;
  std::string proc_stderr;

  bool RunMetrics(const std::string name, const std::string& value, const std::string unit_type, const std::string unit);
  bool CheckRunMetrics(void);
  static void RunMetricsKicker(void* arg);
  static void SyncAsync(void* arg);

  JobStateList jobstatelist;
 public:
  JobsMetrics(void);
  ~JobsMetrics(void);

  void SetEnabled(bool val);

  /* Set path of configuration file */
  void SetConfig(const char* fname);

  /* Set path/name of gmetric  */
  void SetGmetricPath(const char* path);

  void ReportJobStateChange(const GMConfig& config, GMJobRef i, job_state_t old_state, job_state_t new_state);

  void Sync(void);

};

} // namespace ARex

#endif
