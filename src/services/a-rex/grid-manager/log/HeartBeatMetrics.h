/* write essential inforamtion about job started/finished */
#ifndef __GM_HEARTBEAT_METRICS_H__
#define __GM_HEARTBEAT_METRICS_H__

#include <string>
#include <list>
#include <fstream>
#include <ctime>

#include <arc/Run.h>

#include "../jobs/GMJob.h"

#define GMETRIC_STATERATE_UPDATE_INTERVAL 5//to-fix this value could be set in arc.conf to be tailored to site


namespace ARex {

class HeartBeatMetrics {
 private:
  Glib::RecMutex lock;
  bool enabled;
  std::string config_filename;
  std::string tool_path;

  time_t time_delta;


  double free;
  double totalfree;

  bool time_update;
  
  Arc::Run *proc;
  std::string proc_stderr;

  bool RunMetrics(const std::string name, const std::string& value, const std::string unit_type, const std::string unit);
  bool CheckRunMetrics(void);
  static void RunMetricsKicker(void* arg);
  static void SyncAsync(void* arg);

 public:
  HeartBeatMetrics(void);
  ~HeartBeatMetrics(void);

  void SetEnabled(bool val);

  /* Set path of configuration file */
  void SetConfig(const char* fname);

  /* Set path/name of gmetric  */
  void SetGmetricPath(const char* path);

  void ReportHeartBeatChange(const GMConfig& config);
  void Sync(void);

};

} // namespace ARex

#endif
