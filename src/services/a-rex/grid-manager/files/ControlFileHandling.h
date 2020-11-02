#ifndef GRID_MANAGER_INFO_FILES_H
#define GRID_MANAGER_INFO_FILES_H

#include <string>
#include <list>

#include "../jobs/GMJob.h"

#include "ControlFileContent.h"

namespace ARex {

class GMConfig;
class GMJob;

/*
  Definition of functions used to manipulate files used to stored
  information about jobs.
  Most used arguments:
   job - description of job. Mostly used to obtain job identifier
     and directories associated with job.
   config - GM configuration. Used to get control dir information.
   id - job identifier. Used to derive names of files.
*/

extern const char * const sfx_failed;
extern const char * const sfx_cancel;
extern const char * const sfx_restart;
extern const char * const sfx_clean;
extern const char * const sfx_status;
extern const char * const sfx_local;
extern const char * const sfx_errors;
extern const char * const sfx_desc;
extern const char * const sfx_diag;
extern const char * const sfx_lrmsoutput;
extern const char * const sfx_acl;
extern const char * const sfx_proxy;
extern const char * const sfx_xml;
extern const char * const sfx_input;
extern const char * const sfx_output;
extern const char * const sfx_inputstatus;
extern const char * const sfx_outputstatus;
extern const char * const sfx_statistics;
extern const char * const sfx_lrms_done;
extern const char * const sfx_proxy_tmp;
extern const char * const sfx_grami;

extern const char * const subdir_new;
extern const char * const subdir_cur;
extern const char * const subdir_old;
extern const char * const subdir_rew;

enum job_output_mode {
  job_output_all,
  job_output_success,
  job_output_cancel,
  job_output_failure
};

// Set permissions of file 'fname' to -rw------- or if 'executable' is set
// to - -rwx--------- .
bool fix_file_permissions(const std::string &fname,bool executable = false);
// Set permissions taking into account share uid/gid in GMConfig
bool fix_file_permissions(const std::string &fname,const GMJob &job,const GMConfig &config);
bool fix_file_permissions_in_session(const std::string &fname,const GMJob &job,const GMConfig &config,bool executable);

// Set owner of file 'fname' to one specified in 'job'
bool fix_file_owner(const std::string &fname,const GMJob &job);
// Set owner to user
bool fix_file_owner(const std::string &fname,const Arc::User& user);

// Check if file is owned by current user. If user is equivalent to root any
// file is accepted.
// Returns:
//   true - belongs
//   false - does not belong or error.
//   If file exists 'uid', 'gid' and 't' are set to uid, gid and creation
//   of that file.
bool check_file_owner(const std::string &fname);
bool check_file_owner(const std::string &fname,uid_t &uid,gid_t &gid);
bool check_file_owner(const std::string &fname,uid_t &uid,gid_t &gid,time_t &t);

// Make path to file in control dir
std::string job_control_path(std::string const& control_dir, std::string const& id, char const* sfx);

// Check existence, remove and read content of file used to mark
// job finish in LRMS. This file is created by external script/executable
// after it detects job exited and contains exit code of that job.
bool job_lrms_mark_check(const JobId &id,const GMConfig &config);
bool job_lrms_mark_remove(const JobId &id,const GMConfig &config);
LRMSResult job_lrms_mark_read(const JobId &id,const GMConfig &config);

// Create, check existence and remove file used to mark cancellation
// request for specified job. The content of file is not important.
bool job_cancel_mark_put(const GMJob &job,const GMConfig &config);
bool job_cancel_mark_check(const JobId &id,const GMConfig &config);
bool job_cancel_mark_remove(const JobId &id,const GMConfig &config);

// Create, check existence and remove file used to mark request to
// restart job. The content of file is not important.
bool job_restart_mark_put(const GMJob &job,const GMConfig &config);
bool job_restart_mark_check(const JobId &id,const GMConfig &config);
bool job_restart_mark_remove(const JobId &id,const GMConfig &config);

// Same for file, which marks job cleaning/removal request
bool job_clean_mark_put(const GMJob &job,const GMConfig &config);
bool job_clean_mark_check(const JobId &id,const GMConfig &config);
bool job_clean_mark_remove(const JobId &id,const GMConfig &config);

// Create (with given content), add to content, check for existence, delete
// and read content of file used to mark failure of the job.
// Content describes reason of failure (usually 1-2 strings).
bool job_failed_mark_put(const GMJob &job,const GMConfig &config,const std::string &content = "");
bool job_failed_mark_add(const GMJob &job,const GMConfig &config,const std::string &content);
bool job_failed_mark_check(const JobId &id,const GMConfig &config);
bool job_failed_mark_remove(const JobId &id,const GMConfig &config);
std::string job_failed_mark_read(const JobId &id,const GMConfig &config);


// Create, add content, delete and move from session to control directory
// file holding information about resources used by job while running.
// Content is normally produced by "time" utility.
bool job_controldiag_mark_put(const GMJob &job,const GMConfig &config,char const * const args[]);
bool job_diagnostics_mark_put(const GMJob &job,const GMConfig &config);
bool job_diagnostics_mark_remove(const GMJob &job,const GMConfig &config);
bool job_diagnostics_mark_move(const GMJob &job,const GMConfig &config);

// Same for file containing messages from LRMS, which could give additional
// information about reason of job failure.
bool job_lrmsoutput_mark_put(const GMJob &job,const GMConfig &config);

// Common purpose functions, used by previous functions.
std::string job_mark_read(const std::string &fname);
bool job_mark_write(const std::string &fname,const std::string &content);
bool job_mark_add(const std::string &fname,const std::string &content);
bool job_mark_put(const std::string &fname);
bool job_mark_check(const std::string &fname);
bool job_mark_remove(const std::string &fname);
time_t job_mark_time(const std::string &fname);
long int job_mark_size(const std::string &fname);

// Create file to store stderr of external utilities used to stage-in/out
// data, submit/cancel job in LRMS.
bool job_errors_mark_put(const GMJob &job,const GMConfig &config);
bool job_errors_mark_add(const GMJob &job,const GMConfig &config,const std::string &msg);
std::string job_errors_filename(const JobId &id, const GMConfig &config);

// Get modification time of file used to store state of the job.
time_t job_state_time(const JobId &id,const GMConfig &config);

// Read and write file storing state of the job.
job_state_t job_state_read_file(const JobId &id,const GMConfig &config);
job_state_t job_state_read_file(const JobId &id,const GMConfig &config,bool &pending);
bool job_state_write_file(const GMJob &job,const GMConfig &config,job_state_t state,bool pending);

// Get modification time of file used to store description of the job.
time_t job_description_time(const JobId &id,const GMConfig &config);

// Read and write file used to store description of job.
bool job_description_read_file(const JobId &id,const GMConfig &config,std::string &desc);
bool job_description_read_file(const std::string &fname,std::string &desc);
bool job_description_write_file(const GMJob &job,const GMConfig &config,const std::string &desc);

// Read and write file used to store ACL of job.
bool job_acl_read_file(const JobId &id,const GMConfig &config,std::string &acl);
bool job_acl_write_file(const JobId &id,const GMConfig &config,const std::string &acl);

// Read and write xml file containing job description.
bool job_xml_read_file(const JobId &id,const GMConfig &config,std::string &xml);
bool job_xml_write_file(const JobId &id,const GMConfig &config,const std::string &xml);
bool job_xml_check_file(const JobId &id,const GMConfig &config);

// Write and read file, containing most important/needed job parameters.
// Information is passed to/from file through 'job' object.
bool job_local_write_file(const GMJob &job,const GMConfig &config,const JobLocalDescription &job_desc);
bool job_local_write_file(const std::string &fname,const JobLocalDescription &job_desc);
bool job_local_read_file(const JobId &id,const GMConfig &config,JobLocalDescription &job_desc);
bool job_local_read_file(const std::string &fname,JobLocalDescription &job_desc);

// Read only some attributes from previously mentioned file.
bool job_local_read_cleanuptime(const JobId &id,const GMConfig &config,time_t &cleanuptime);
bool job_local_read_failed(const JobId &id,const GMConfig &config,std::string &state,std::string &cause);
bool job_local_read_delegationid(const JobId &id,const GMConfig &config,std::string &delegationid);

// Write and read file containing list of input files. Each line of file
// contains name of input file relative to session directory and optionally
// source, from which it should be transferred.
bool job_input_write_file(const GMJob &job,const GMConfig &config,std::list<FileData> &files);
bool job_input_read_file(const JobId &id,const GMConfig &config,std::list<FileData> &files);

bool job_input_status_add_file(const GMJob &job,const GMConfig &config,const std::string& file = "");
bool job_input_status_read_file(const JobId &id,const GMConfig &config,std::list<std::string>& files);

// Write and read file containing list of output files. Each line of file
// contains name of output file relative to session directory and optionally
// destination, to which it should be transferred.
bool job_output_write_file(const GMJob &job,const GMConfig &config,std::list<FileData> &files,job_output_mode mode = job_output_all);
bool job_output_read_file(const JobId &id,const GMConfig &config,std::list<FileData> &files);

bool job_output_status_add_file(const GMJob &job,const GMConfig &config,const FileData& file);
bool job_output_status_write_file(const GMJob &job,const GMConfig &config,std::list<FileData>& files);
bool job_output_status_read_file(const JobId &id,const GMConfig &config,std::list<FileData>& files);

// Common functions for input/output files.
bool job_Xput_read_file(const std::string &fname,std::list<FileData> &files, uid_t uid = 0, gid_t gid = 0);
bool job_Xput_write_file(const std::string &fname,std::list<FileData> &files,
                         job_output_mode mode = job_output_all, uid_t uid = 0, gid_t gid = 0);

// Return filename storing job's proxy.
std::string job_proxy_filename(const JobId &id, const GMConfig &config);
bool job_proxy_write_file(const GMJob &job,const GMConfig &config,const std::string &cred);
bool job_proxy_read_file(const JobId &id,const GMConfig &config,std::string &cred);

// Remove all files, which should be removed after job's state becomes FINISHED
bool job_clean_finished(const JobId &id,const GMConfig &config);

// Remove all files, which should be removed after job's state becomes DELETED
bool job_clean_deleted(const GMJob &job,const GMConfig &config, std::list<std::string> cache_per_job_dirs=std::list<std::string>());

// Remove all job's files.
bool job_clean_final(const GMJob &job,const GMConfig &config);

} // namespace ARex

#endif
