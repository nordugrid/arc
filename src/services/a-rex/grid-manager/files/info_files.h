#ifndef GRID_MANAGER_INFO_FILES_H
#define GRID_MANAGER_INFO_FILES_H

#include <string>
#include <list>

#include "../jobs/states.h"
#include "../jobs/users.h"
#include "info_types.h"

/*
  Definition of functions used to manipulate files used to stored
  information about jobs.
  Most used arguments:
   desc - description of job. Mostly used to obtain job identifier
     and directories associated with job.
   user - description of user (owner of job). Used to obtain directories
     associated with it.
   id - job identifier. Used to derive names of files.
*/
/* 
  job status files:
   files           created by    modified by   read by     root?
  lrms_mark       helper        -             gm          yes/no
  cancel_mark     job_manager   gm            gm          no
  clean_mark      job_manager   gm            gm          no
  failed_mark     gm            gm            +           yes
  state           job_manager   gm            gm          no
  description     job_manager   -             gm          no
  input           gm            downloader    downloader  yes/no
  output          gm            uploader      uploader    yes/no
  local           job_manager   gm            +           no
*/

/*
extern job_state_rec_t states_all[JOB_STATE_UNDEFINED+1];
*/
/*
  Set permissions of file 'fname' to -rw------- or if 'executable' is set
  to - -rwx--------- .
*/
bool fix_file_permissions(const std::string &fname,bool executable = false);
/*
  Set owner of file 'fname' to one specified in 'desc' or 'user'. 
  'desc' has priority if set.
*/
bool fix_file_owner(const std::string &fname,const JobUser &user);
bool fix_file_owner(const std::string &fname,const JobDescription &desc,const JobUser &user);
/*
  Check if file is owned by user as configured in 'user'. If 'user' is
  special user (equivalent to root) any file is accepted.
  Returns:
   true - belongs
   false - does not belong or error.
   If file exists 'uid', 'gid' and 't' are set to uid, gid and creation
   of that file.
*/
bool check_file_owner(const std::string &fname,const JobUser &user);
bool check_file_owner(const std::string &fname,const JobUser &user,uid_t &uid,gid_t &gid);
bool check_file_owner(const std::string &fname,const JobUser &user,uid_t &uid,gid_t &gid,time_t &t);

/*
  Check existance, remove and read content of file used to mark 
  job finish in LRMS. This file is created by external script/executable
  after it detects job exited and contains exit code of that job.
*/
bool job_lrms_mark_put(const JobDescription &desc,JobUser &user,int code);
bool job_lrms_mark_put(const JobDescription &desc,JobUser &user,LRMSResult r);
bool job_lrms_mark_check(JobId &id,JobUser &user);
bool job_lrms_mark_remove(JobId &id,JobUser &user);
LRMSResult job_lrms_mark_read(JobId &id,JobUser &user);

/*
  Create, check existance and remove file used to mark cancelation
  request for specified job. The content of file is not important.
*/
bool job_cancel_mark_put(const JobDescription &desc,JobUser &user);
bool job_cancel_mark_check(JobId &id,JobUser &user);
bool job_cancel_mark_remove(JobId &id,JobUser &user);

/*
  Create, check existance and remove file used to mark request to
  restart job. The content of file is not important.
*/
bool job_restart_mark_put(const JobDescription &desc,JobUser &user);
bool job_restart_mark_check(JobId &id,JobUser &user);
bool job_restart_mark_remove(JobId &id,JobUser &user);
/*
  Same for file, which marks job cleaning/removal request
*/
bool job_clean_mark_put(const JobDescription &desc,JobUser &user);
bool job_clean_mark_check(JobId &id,JobUser &user);
bool job_clean_mark_remove(JobId &id,JobUser &user);

/*
  Create (with given content), add to content, check for existance, delete
  and read content of file used to mark failure of the job. 
  Content describes reason of failure (usually 1-2 strings).
*/
bool job_failed_mark_put(const JobDescription &desc,JobUser &user,const std::string &content = "");
bool job_failed_mark_add(const JobDescription &desc,JobUser &user,const std::string &content);
bool job_failed_mark_check(const JobId &id,JobUser &user);
bool job_failed_mark_remove(const JobId &id,JobUser &user);
std::string job_failed_mark_read(const JobId &id,JobUser &user);

/*
  Create, add content, delete and move from session to control directory
  file holding information about resources used by job while running.
  Content is normally produced by "time" utility.
*/
bool job_controldiag_mark_put(const JobDescription &desc,JobUser &user,char const * const args[]);
bool job_diagnostics_mark_put(const JobDescription &desc,JobUser &user);
bool job_diagnostics_mark_add(const JobDescription &desc,JobUser &user,const std::string &content);
bool job_diagnostics_mark_remove(const JobDescription &desc,JobUser &user);
bool job_diagnostics_mark_move(const JobDescription &desc,JobUser &user);

/*
  Same (except add) for file containing messages from LRMS, which
  could give additional information about reason of job failure.
*/
bool job_lrmsoutput_mark_put(const JobDescription &desc,JobUser &user);
/*
  This functionality should now go into LRMS backend
bool job_lrmsoutput_mark_remove(const JobDescription &desc,JobUser &user);
bool job_lrmsoutput_mark_move(const JobDescription &desc,JobUser &user);
bool job_lrmsoutput_mark_get(JobDescription &desc,JobUser &user);
*/

/*
  Common purpose functions, used by previous functions.
*/
std::string job_mark_read_s(const std::string &fname);
long int job_mark_read_i(const std::string &fname);
bool job_mark_write_s(const std::string &fname,const std::string &content);
bool job_mark_add_s(const std::string &fname,const std::string &content);
bool job_mark_put(const std::string &fname);
bool job_mark_check(const std::string &fname);
bool job_mark_remove(const std::string &fname);
time_t job_mark_time(const std::string &fname);
long int job_mark_size(const std::string &fname);

/*
  Create file to store stderr of external utilities used to stage-in/out
  data, submit/cancel job in LRMS.
*/
bool job_errors_mark_put(const JobDescription &desc,JobUser &user);

/*
  Get modification time of file used to store state of the job.
*/
time_t job_state_time(const JobId &id,JobUser &user);

/*
  Read and write file storing state of the job.
*/
job_state_t job_state_read_file(const JobId &id,const JobUser &user);
job_state_t job_state_read_file(const JobId &id,const JobUser &user,bool &pending);
job_state_t job_state_read_file(const std::string &fname,bool &pending);
bool job_state_write_file(const JobDescription &desc,JobUser &user,job_state_t state,bool pending = false);
bool job_state_write_file(const std::string &fname,job_state_t state,bool pending = false);

/*
  Read and write file used to store RSL description of job.
*/
bool job_description_read_file(JobId &id,JobUser &user,std::string &rsl);
bool job_description_read_file(const std::string &fname,std::string &rsl);
bool job_description_write_file(const std::string &fname,std::string &rsl);
bool job_description_write_file(const std::string &fname,const char* rsl);

/*
  Read and write file used to store ACL of job.
*/
bool job_acl_read_file(JobId &id,JobUser &user,std::string &acl);
bool job_acl_write_file(JobId &id,JobUser &user,std::string &acl);

/*
  Not used
*/
bool job_cache_write_file(const JobDescription &desc,JobUser &user,std::list<FileData> &files);
bool job_cache_read_file(const JobId &id,JobUser &user,std::list<FileData> &files);

/*
  Write and read file containing list of output files. Each line of file
  contains name of output file relative to session directory and optionally
  destination, to which it should be transfered.
*/
bool job_output_write_file(const JobDescription &desc,JobUser &user,std::list<FileData> &files);
bool job_output_read_file(const JobId &id,JobUser &user,std::list<FileData> &files);

/*
  Same for input files and their sources.
*/
bool job_input_write_file(const JobDescription &desc,JobUser &user,std::list<FileData> &files);
bool job_input_read_file(const JobId &id,JobUser &user,std::list<FileData> &files);

/*
  Functions used by previous functions.
*/
bool job_Xput_write_file(const std::string &fname,std::list<FileData> &files);
bool job_Xput_read_file(const std::string &fname,std::list<FileData> &files);
bool job_Xput_read_file(std::list<FileData> &files);

/*
  Write and read file, containing most important/needed job parameters.
  Information is passed to/from file through 'job_desc' object.
*/
bool job_local_write_file(const JobDescription &desc,const JobUser &user,JobLocalDescription &job_desc);
bool job_local_write_file(const std::string &fname,JobLocalDescription &job_desc);
bool job_local_read_file(const JobId &id,const JobUser &user,JobLocalDescription &job_desc);
bool job_local_read_file(const std::string &fname,JobLocalDescription &job_desc);
// bool job_local_read_string(const std::string &fname,unsigned int num,std::string &str);

/*
  Read only some attributes from previously mentioned file.
*/
bool job_local_read_notify(const JobId &id,const JobUser &user,std::string &notify);
bool job_local_read_lifetime(const JobId &id,const JobUser &user,time_t &lifetime);
bool job_local_read_cleanuptime(const JobId &id,const JobUser &user,time_t &cleanuptime);

/*
  Not used
*/
bool job_diskusage_create_file(const JobDescription &desc,JobUser &user,unsigned long long int &requested);
bool job_diskusage_read_file(const JobDescription &desc,JobUser &user,unsigned long long int &requested,unsigned long long int &used);
bool job_diskusage_change_file(const JobDescription &desc,JobUser &user,signed long long int used,bool &result);
bool job_diskusage_remove_file(const JobDescription &desc,JobUser &user);

/*
  Concatenate files containing diagnostics and stderr of external utilities
  and write to separate file in jobs directories (as requested by user).  
*/
bool job_stdlog_move(const JobDescription &desc,JobUser &user,const std::string &logname);

/*
  Remove all files, which should be removed after job's state becomes FINISHED
*/
bool job_clean_finished(const JobId &id,JobUser &user);

/*
  Remove all files, which should be removed after job's state becomes DELETED
*/
bool job_clean_deleted(const JobDescription &desc,JobUser &user);

/*
  Remove all job's files.
*/
bool job_clean_final(const JobDescription &desc,JobUser &user);

#endif

