/* write essential inforamtion about job started/finished */
#ifndef __GM_JOBS_METRICS_H__
#define __GM_JOBS_METRICS_H__

#include <string>
#include <list>
#include <fstream>

#include <arc/Run.h>

#include "../jobs/GMJob.h"

namespace ARex {

class JobsMetrics {
 private:
  Glib::RecMutex lock;
  bool enabled;
  std::string config_filename;
  std::string tool_path;
  unsigned long long int jobs_processed[JOB_STATE_UNDEFINED];
  unsigned long long int jobs_in_state[JOB_STATE_UNDEFINED];
  unsigned long long int jobs_state_old_new[JOB_STATE_UNDEFINED+1][JOB_STATE_UNDEFINED];
  bool jobs_processed_changed[JOB_STATE_UNDEFINED];
  bool jobs_in_state_changed[JOB_STATE_UNDEFINED];
  bool jobs_state_old_new_changed[JOB_STATE_UNDEFINED+1][JOB_STATE_UNDEFINED];

  std::map<std::string,job_state_t> jobs_state_old_map;
  std::map<std::string,job_state_t> jobs_state_new_map;
  
  Arc::Run *proc;
  std::string proc_stderr;

  bool RunMetrics(const std::string name, const std::string& value);
  bool CheckRunMetrics(void);
  static void RunMetricsKicker(void* arg);

 public:
  JobsMetrics(void);
  ~JobsMetrics(void);

  void SetEnabled(bool val);

  /* chose name of configuration file */
  void SetConfig(const char* fname);

  /* chose name of configuration file */
  void SetPath(const char* path);

  void ReportJobStateChange(std::string job_id, job_state_t new_state, job_state_t old_state);

  void Sync(void);

};

} // namespace ARex

#endif
