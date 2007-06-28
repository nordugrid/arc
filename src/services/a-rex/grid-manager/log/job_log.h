/* write essential inforamtion about job started/finished */
#ifndef __GM_JOB_LOG_H__
#define __GM_JOB_LOG_H__

#include <string>
#include <list>
#include <fstream>
#include "../misc/escaped.h"
#include "../jobs/job.h"
#include "../jobs/users.h"
#include "../run/run_parallel.h"

///  Put short information into log when every job starts/finishes.
///  And store more detailed information for Reporter.
class JobLog {
 private:
  std::string filename;
  std::list<std::string> urls;
  RunElement *proc;
  time_t last_run;
  time_t ex_period;
  bool open_stream(std::ofstream &o);
 public:
  JobLog(void);
  JobLog(const char* fname);
  ~JobLog(void);
  /* chose name of log file */
  void SetOutput(const char* fname);
  /* log job start information */
  bool start_info(JobDescription &job,const JobUser &user);
  /* log job finish iformation */
  bool finish_info(JobDescription &job,const JobUser &user);
  /* read information stored by start_info and finish_info */
  static bool read_info(std::fstream &i,bool &processed,bool &jobstart,struct tm &t,JobId &jobid,JobLocalDescription &job_desc,std::string &failure);
  bool is_reporting(void) { return (urls.size() > 0); };
  /* Run external utility to report gathered information to logger service */
  bool RunReporter(JobUsers& users);
  /* Set url of service and local name to use */
  // bool SetReporter(const char* destination,const char* name = NULL);
  bool SetReporter(const char* destination);
  /* Set after which too old logger information is removed */
  void SetExpiration(time_t period = 0);
  /* Create data file for Reporter */
  bool make_file(JobDescription &job,JobUser &user);
};

#endif
