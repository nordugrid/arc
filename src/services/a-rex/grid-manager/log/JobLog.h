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
  std::list<std::string> report_config; // additional configuration for usage reporter
  std::string certificate_path;
  std::string ca_certificates_dir;A
  // reporter vars
  std::string reporter_tool;
  std::string reporter_logfile;
  Arc::Run *reporter_proc;
  time_t reporter_last_run;
  int reporter_period;
  // archive manager vars
  std::string archive_mgmt_tool;
  std::string archive_mgmt_logfile;
  Arc::Run *archive_mgmt_proc;
  time_t archive_mgmt_last_run;
  int archive_mgmt_period;

  bool open_stream(std::ofstream &o);
  static void initializer(void* arg);
 public:
  JobLog(void);
  //JobLog(const char* fname);
  ~JobLog(void);
  /* chose name of log file */
  void SetOutput(const char* fname);
  /* log job start information */
  bool WriteStartInfo(GMJob &job,const GMConfig &config);
  /* log job finish iformation */
  bool WriteFinishInfo(GMJob &job,const GMConfig& config);
  /* Check accounting records reporting is enabled */
  bool ReporterEnabled(void){ return !reporter_tool.empty();}
  /* Run external utility to report gathered information to accounting services */
  bool RunReporter(const GMConfig& config);
  /* Set period of running reporter */
  bool SetReporterPeriod(int period);
  /* Set name of the accounting reporter */
  bool SetReporter(const char* fname);
  /* Set name of the log file for accounting reporter */
  bool SetReporterLogFile(const char* fname);
  /* Check accounting archive management is enabled */
  bool ArchiveManagerEnabled(void){ return !archive_mgmt_tool.empty();}
  /* Run external utility to manage archived accounting records */
  bool RunArchiveManager(const GMConfig& config);
  /* Set name of the archive management tool */
  bool SetArchiveManager(const char* fname);
  /* Set period of running archive manager */
  bool SetArchiveManagerPeriod(int period);
  /* Set name of the log file for accounting archive management */
  bool SetArchiveManagerLogFile(const char* fname);
  /* Create data file for Reporter */
  bool WriteJobRecord(GMJob &job,const GMConfig &config);
  /* Set credential file names for accessing logging service */
  void SetCredentials(std::string const &key_path,std::string const &certificate_path,std::string const &ca_certificates_dir);
  /* Set accounting options (e.g. batch size for SGAS LUTS) */
  void SetOptions(std::string const &options) { report_config.push_back(std::string("accounting_options=")+options); }
};

} // namespace ARex

#endif
