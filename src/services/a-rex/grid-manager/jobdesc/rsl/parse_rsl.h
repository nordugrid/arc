#ifndef __ARC_GM_PARSE_RSL_H__
#define __ARC_GM_PARSE_RSL_H__
#include <string>

#include "../../jobs/users.h"
#include "../../files/info_files.h"
#include "../../files/info_types.h"

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

#endif
