#include <string>

#include "../../jobs/users.h"
#include "../../files/info_files.h"
#include "../../files/info_types.h"
#include <globus_common.h>
#include <globus_rsl.h>

class value_for_shell {
 friend std::ostream& operator<<(std::ostream&,const value_for_shell&);
 private:
  const char* str;
  bool quote;
 public:
  value_for_shell(const char *str_,bool quote_):str(str_),quote(quote_) { };
  value_for_shell(const std::string &str_,bool quote_):str(str_.c_str()),quote(quote_) { };
};
std::ostream& operator<<(std::ostream &o,const value_for_shell &s);

class numvalue_for_shell {
 friend std::ostream& operator<<(std::ostream&,const numvalue_for_shell&);
 private:
  long int n;
 public:
  numvalue_for_shell(const char *str) { n=0; sscanf(str,"%li",&n); };
  numvalue_for_shell(int n_) { n=n_; };
  numvalue_for_shell operator/(int d) { return numvalue_for_shell(n/d); };
  numvalue_for_shell operator*(int d) { return numvalue_for_shell(n*d); };
};
std::ostream& operator<<(std::ostream &o,const numvalue_for_shell &s);

bool parse_rsl_for_action(const char* fname,std::string &action_s,std::string &jobid_s,std::string &lrms,std::string &queue);
bool parse_rsl(const std::string &fname,JobLocalDescription &job_desc,std::string* acl = NULL);
bool preprocess_rsl(const std::string &fname,const std::string &session_dir,const std::string &jobid);
bool process_rsl(JobUser &user,const JobDescription &desc);
bool process_rsl(JobUser &user,const JobDescription &desc,JobLocalDescription &job_desc);
bool write_grami_rsl(const JobDescription &desc,const JobUser &user,const char *opt_add = NULL);
// std::string read_grami(const JobId &job_id,const JobUser &user);
// bool set_execs(const JobDescription &desc,const JobUser &user,const std::string &session_dir);
bool set_execs(globus_rsl_t *rsl_tree,const std::string &session_dir);
globus_rsl_t* read_rsl(const std::string &fname);

#define NG_RSL_ACTION_PARAM       "action"
#define NG_RSL_LRMS_PARAM         "lrmstype"
#define NG_RSL_QUEUE_PARAM        "queue"
#define NG_RSL_INPUT_DATA_PARAM   "inputfiles"
#define NG_RSL_OUTPUT_DATA_PARAM  "outputfiles"
#define NG_RSL_EXECUTABLE_PARAM   "executable"
#define NG_RSL_ARGUMENTS_PARAM    "arguments"
#define NG_RSL_EXECUTABLES_PARAM  "executables"
#define NG_RSL_JOB_ID_PARAM       "jobid"
#define NG_RSL_REPLICA_PARAM      "replicacollection"
#define NG_RSL_LIFETIME_PARAM     "lifetime"
#define NG_RSL_NOTIFY_PARAM       "notify"
#define NG_RSL_STARTTIME_PARAM    "starttime"
#define NG_RSL_RUNTIME_PARAM      "runtimeenvironment"
#define NG_RSL_JOBNAME_PARAM      "jobname"
#define NG_RSL_RERUN_PARAM        "rerun"
#define NG_RSL_FTPTHREADS_PARAM   "ftpthreads"
#define NG_RSL_CACHE_PARAM        "cache"
#define NG_RSL_HOSTNAME_PARAM     "hostname"
#define NG_RSL_SOFTWARE_PARAM     "clientsoftware"
#define NG_RSL_STDIN_PARAM        "stdinput"
#define NG_RSL_STDIN_PARAM2       "sstdin"
#define NG_RSL_STDLOG_PARAM       "gmlog"
#define NG_RSL_DISKSPACE_PARAM    "disk"
#define NG_RSL_COUNT_PARAM        "count"
#define NG_RSL_CPUTIME_PARAM      "cputime"
#define NG_RSL_WALLTIME_PARAM     "walltime"
#define NG_RSL_MEMORY_PARAM       "memory"
#define NG_RSL_JOBREPORT_PARAM    "jobreport"
#define NG_RSL_ACL_PARAM          "acl"
#define NG_RSL_DRY_RUN_PARAM      "dryrun"
#define NG_RSL_SESSION_TYPE_PARAM "sessiondirectorytype"
#define NG_RSL_CRED_SERVER_PARAM  "credentialserver"
#define NG_RSL_ENVIRONMENT_PARAM  "environment"
#define NG_RSL_PROJECT_PARAM      "project"
#define NG_RSL_STDIN_PARAM3       "stdin"
#define NG_RSL_STDOUT_PARAM       "stdout"
#define NG_RSL_STDERR_PARAM       "stderr"
#define NG_RSL_DEFAULT_STDIN      "/dev/null"
#define NG_RSL_DEFAULT_STDOUT     "/dev/null"
#define NG_RSL_DEFAULT_STDERR     "/dev/null"

