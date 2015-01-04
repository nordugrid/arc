/* write essential inforamtion about job started/finished */
#ifndef __GM_JOB_LOG_H__
#define __GM_JOB_LOG_H__

#include <string>
#include <list>
#include <fstream>

#include <arc/Run.h>

#include "../jobs/GMJob.h"

namespace ARex {

class GMConfig;
class JobLocalDescription;

///  Put short information into log when every job starts/finishes.
///  And store more detailed information for Reporter.
class JobLog {
 private:
  std::string filename;
  std::list<std::string> urls;
  std::list<std::string> report_config; // additional configuration for usage reporter
  std::string vo_filters;
  std::string certificate_path;
  std::string ca_certificates_dir;
  std::string logger_name;
  std::string logfile;
  Arc::Run *proc;
  time_t last_run;
  int period;
  time_t ex_period;
  bool open_stream(std::ofstream &o);
  static void initializer(void* arg);
 public:
  JobLog(void);
  //JobLog(const char* fname);
  ~JobLog(void);
  /* chose name of log file */
  void SetOutput(const char* fname);
  /* log job start information */
  bool start_info(GMJob &job,const GMConfig &config);
  /* log job finish iformation */
  bool finish_info(GMJob &job,const GMConfig& config);
  /* Run external utility to report gathered information to logger service */
  bool RunReporter(const GMConfig& config);
  /* Set period of running */
  bool SetPeriod(int period);
  /* Set name of the accounting reporter */
  bool SetLogger(const char* fname);
  /* Set name of the log file for accounting reporter */
  bool SetLogFile(const char* fname);
  /* Set filters of VO that allow to report to accounting service */
  bool SetVoFilters(const char* filters);
  /* Set url of service and local name to use */
  // bool SetReporter(const char* destination,const char* name = NULL);
  bool SetReporter(const char* destination);
  /* Set after which too old logger information is removed */
  void SetExpiration(time_t period = 0);
  /* Create data file for Reporter */
  bool make_file(GMJob &job,const GMConfig &config);
  /* Set credential file names for accessing logging service */
  void SetCredentials(std::string &key_path,std::string &certificate_path,std::string &ca_certificates_dir);
  /* Set accounting options (e.g. batch size for SGAS LUTS) */
  void SetOptions(std::string &options) { report_config.push_back(std::string("accounting_options=")+options); }
};

} // namespace ARex

#endif
