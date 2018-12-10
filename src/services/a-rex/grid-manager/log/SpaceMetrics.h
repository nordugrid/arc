/* write essential inforamtion about job started/finished */
#ifndef __GM_SPACE_METRICS_H__
#define __GM_SPACE_METRICS_H__

#include <string>
#include <list>
#include <fstream>
#include <ctime>

#include <arc/Run.h>

#include "../jobs/GMJob.h"

#define GMETRIC_STATERATE_UPDATE_INTERVAL 5//to-fix this value could be set in arc.conf to be tailored to site


namespace ARex {

class SpaceMetrics {
 private:
  Glib::RecMutex lock;
  bool enabled;
  std::string config_filename;
  std::string tool_path;

  double freeCache;
  double totalFreeCache;
  bool freeCache_update;
  
  Arc::Run *proc;
  std::string proc_stderr;

  bool RunMetrics(const std::string name, const std::string& value, const std::string unit_type, const std::string unit);
  bool CheckRunMetrics(void);
  static void RunMetricsKicker(void* arg);
  static void SyncAsync(void* arg);

 public:
  SpaceMetrics(void);
  ~SpaceMetrics(void);

  void SetEnabled(bool val);

  /* Set path of configuration file */
  void SetConfig(const char* fname);

  /* Set path/name of gmetric  */
  void SetGmetricPath(const char* path);

  void ReportSpaceChange(const GMConfig& config);
  void Sync(void);

};

} // namespace ARex

#endif
