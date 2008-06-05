#ifndef __ARC_GM_PARSE_RSL_H__
#define __ARC_GM_PARSE_RSL_H__
#include <string>

#include <globus_common.h>
#include <globus_rsl.h>

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

#define NG_RSL_ACTION_PARAM       const_cast<char*>("action")
#define NG_RSL_LRMS_PARAM         const_cast<char*>("lrmstype")
#define NG_RSL_QUEUE_PARAM        const_cast<char*>("queue")
#define NG_RSL_INPUT_DATA_PARAM   const_cast<char*>("inputfiles")
#define NG_RSL_OUTPUT_DATA_PARAM  const_cast<char*>("outputfiles")
#define NG_RSL_EXECUTABLE_PARAM   const_cast<char*>("executable")
#define NG_RSL_ARGUMENTS_PARAM    const_cast<char*>("arguments")
#define NG_RSL_EXECUTABLES_PARAM  const_cast<char*>("executables")
#define NG_RSL_JOB_ID_PARAM       const_cast<char*>("jobid")
#define NG_RSL_REPLICA_PARAM      const_cast<char*>("replicacollection")
#define NG_RSL_LIFETIME_PARAM     const_cast<char*>("lifetime")
#define NG_RSL_NOTIFY_PARAM       const_cast<char*>("notify")
#define NG_RSL_STARTTIME_PARAM    const_cast<char*>("starttime")
#define NG_RSL_RUNTIME_PARAM      const_cast<char*>("runtimeenvironment")
#define NG_RSL_JOBNAME_PARAM      const_cast<char*>("jobname")
#define NG_RSL_RERUN_PARAM        const_cast<char*>("rerun")
#define NG_RSL_FTPTHREADS_PARAM   const_cast<char*>("ftpthreads")
#define NG_RSL_CACHE_PARAM        const_cast<char*>("cache")
#define NG_RSL_HOSTNAME_PARAM     const_cast<char*>("hostname")
#define NG_RSL_SOFTWARE_PARAM     const_cast<char*>("clientsoftware")
#define NG_RSL_STDIN_PARAM        const_cast<char*>("stdinput")
#define NG_RSL_STDIN_PARAM2       const_cast<char*>("sstdin")
#define NG_RSL_STDLOG_PARAM       const_cast<char*>("gmlog")
#define NG_RSL_DISKSPACE_PARAM    const_cast<char*>("disk")
#define NG_RSL_COUNT_PARAM        const_cast<char*>("count")
#define NG_RSL_CPUTIME_PARAM      const_cast<char*>("cputime")
#define NG_RSL_WALLTIME_PARAM     const_cast<char*>("walltime")
#define NG_RSL_MEMORY_PARAM       const_cast<char*>("memory")
#define NG_RSL_JOBREPORT_PARAM    const_cast<char*>("jobreport")
#define NG_RSL_ACL_PARAM          const_cast<char*>("acl")
#define NG_RSL_DRY_RUN_PARAM      const_cast<char*>("dryrun")
#define NG_RSL_SESSION_TYPE_PARAM const_cast<char*>("sessiondirectorytype")
#define NG_RSL_CRED_SERVER_PARAM  const_cast<char*>("credentialserver")
#define NG_RSL_ENVIRONMENT_PARAM  const_cast<char*>("environment")
#define NG_RSL_PROJECT_PARAM      const_cast<char*>("project")
#define NG_RSL_STDIN_PARAM3       const_cast<char*>("stdin")
#define NG_RSL_STDOUT_PARAM       const_cast<char*>("stdout")
#define NG_RSL_STDERR_PARAM       const_cast<char*>("stderr")

#endif
