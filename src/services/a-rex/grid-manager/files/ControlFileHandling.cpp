#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sstream>
#include <fcntl.h>
#include <errno.h>

#include <arc/FileAccess.h>
#include <arc/FileUtils.h>
#include <arc/FileLock.h>

#include "../run/RunRedirected.h"
#include "../conf/GMConfig.h"
#include "../jobs/GMJob.h"

#include "ControlFileHandling.h"

namespace ARex {

// Files in control dir, job.id.sfx
const char * const sfx_failed      = "failed";        // Description of failure
const char * const sfx_cancel      = "cancel";        // Mark to tell A-REX to cancel job
const char * const sfx_restart     = "restart";       // Mark to tell A-REX to restart job
const char * const sfx_clean       = "clean";         // Mark to tell A-REX to clean job
const char * const sfx_status      = ".status";        // Current job status
const char * const sfx_local       = "local";         // Local information about job
const char * const sfx_errors      = "errors";        // Log of data staging and job submission
const char * const sfx_desc        = "description";   // Job description sent by user
const char * const sfx_diag        = "diag";          // Diagnostic info about finished job
const char * const sfx_lrmsoutput  = "comment";       // Additional information from LRMS
const char * const sfx_acl         = "acl";           // ACL information for job
const char * const sfx_proxy       = "proxy";         // Delegated proxy
const char * const sfx_xml         = "xml";           // XML description of job
const char * const sfx_input       = "input";         // Input files required by job
const char * const sfx_output      = "output";        // Output files written by job
const char * const sfx_inputstatus = "input_status";  // Input files staged by client
const char * const sfx_outputstatus = "output_status";// Output files already staged out
const char * const sfx_statistics  = "statistics";    // Statistical information on data staging
const char * const sfx_lrms_done   = "lrms_done";
const char * const sfx_proxy_tmp   = "proxy_tmp";
const char * const sfx_grami       = "grami";

// Sub-directories for different jobs states
const char * const subdir_new      = "accepting";      // Submitted but not yet picked up by A-REX
const char * const subdir_cur      = "processing";     // Being processed by A-REX
const char * const subdir_old      = "finished";       // Finished or deleted jobs
const char * const subdir_rew      = "restarting";     // Jobs waiting to restart

static Arc::Logger& logger = Arc::Logger::getRootLogger();

static job_state_t job_state_read_file(const std::string &fname,bool &pending);
static bool job_state_write_file(const std::string &fname,job_state_t state,bool pending = false);
static bool job_mark_put(Arc::FileAccess& fa, const std::string &fname);
static bool job_mark_remove(Arc::FileAccess& fa,const std::string &fname);


bool fix_file_permissions(const std::string &fname,bool executable) {
  mode_t mode = S_IRUSR | S_IWUSR;
  if(executable) { mode |= S_IXUSR; };
  return (chmod(fname.c_str(),mode) == 0);
}

static bool fix_file_permissions(Arc::FileAccess& fa,const std::string &fname,bool executable = false) {
  mode_t mode = S_IRUSR | S_IWUSR;
  if(executable) { mode |= S_IXUSR; };
  return fa.fa_chmod(fname.c_str(),mode);
}

bool fix_file_permissions(const std::string &fname,const GMJob &job,const GMConfig& config) {
  mode_t mode = S_IRUSR | S_IWUSR;
  uid_t uid = job.get_user().get_uid();
  gid_t gid = job.get_user().get_gid();
  if(!config.MatchShareUid(uid)) {
    mode |= S_IRGRP;
    if(!config.MatchShareGid(gid)) {
      mode |= S_IROTH;
    };
  };
  return (chmod(fname.c_str(),mode) == 0);
}

bool fix_file_permissions_in_session(const std::string &fname,const GMJob &job,const GMConfig &config,bool executable) {
  mode_t mode = S_IRUSR | S_IWUSR;
  if(executable) { mode |= S_IXUSR; };
  if(config.StrictSession()) {
    uid_t uid = getuid()==0?job.get_user().get_uid():getuid();
    uid_t gid = getgid()==0?job.get_user().get_gid():getgid();
    Arc::FileAccess fa;
    if(!fa.fa_setuid(uid,gid)) return false;
    return fa.fa_chmod(fname,mode);
  };
  return (chmod(fname.c_str(),mode) == 0);
}

bool fix_file_owner(const std::string &fname,const GMJob& job) {
  return fix_file_owner(fname, job.get_user());
}

bool fix_file_owner(const std::string &fname,const Arc::User& user) {
  if(getuid() == 0) {
    if(lchown(fname.c_str(),user.get_uid(),user.get_gid()) == -1) {
      logger.msg(Arc::ERROR,"Failed setting file owner: %s",fname);
      return false;
    };
  };
  return true;
}

bool check_file_owner(const std::string &fname) {
  uid_t uid;
  gid_t gid;
  time_t t;
  return check_file_owner(fname,uid,gid,t);
}

bool check_file_owner(const std::string &fname,uid_t &uid,gid_t &gid) {
  time_t t;
  return check_file_owner(fname,uid,gid,t);
}

bool check_file_owner(const std::string &fname,uid_t &uid,gid_t &gid,time_t &t) {
  struct stat st;
  if(lstat(fname.c_str(),&st) != 0) return false;
  if(!S_ISREG(st.st_mode)) return false;
  uid=st.st_uid; gid=st.st_gid; t=st.st_ctime;
  /* superuser can't run jobs */
  if(uid == 0) return false;
  /* accept any file if superuser */
  if(getuid() != 0) {
    if(uid != getuid()) return false;
  };
  return true;
}

static const std::string::size_type id_split_chunk = 3;
static const std::string::size_type id_split_num = 4;

std::string job_control_path(std::string const& control_dir, std::string const& id, char const* sfx) {
  std::string path(control_dir);
  path += "/jobs/";
  int num = id_split_num;
  for(std::string::size_type pos = 0; pos < id.length(); pos+=id_split_chunk) {
    if (--num == 0) {
      path.append(id,pos,std::string::npos);
      path += "/";
      break;
    };
    path.append(id,pos,id_split_chunk);
    path += "/";
  };
  if(sfx) path += sfx;
  return path;
}

//static std::string job_control_path(std::string const& control_dir, std::string const& id, char const* sfx) {
//  return control_dir + id + sfx;
//}

bool job_lrms_mark_check(const JobId &id,const GMConfig &config) {
  std::string fname = job_control_path(config.ControlDir(), id, sfx_lrms_done);
  return job_mark_check(fname);
}

bool job_lrms_mark_remove(const JobId &id,const GMConfig &config) {
  std::string fname = job_control_path(config.ControlDir(), id, sfx_lrms_done);
  return job_mark_remove(fname);
}

LRMSResult job_lrms_mark_read(const JobId &id,const GMConfig &config) {
  std::string fname = job_control_path(config.ControlDir(), id, sfx_lrms_done);
  LRMSResult r("-1 Internal error");
  std::ifstream f(fname.c_str()); if(! f.is_open() ) return r;
  f>>r;
  return r;
}

bool job_cancel_mark_put(const GMJob &job,const GMConfig &config) {
  std::string fname = config.ControlDir() + "/" + subdir_new + "/" + job.get_id() + sfx_cancel;
  return job_mark_put(fname) && fix_file_owner(fname,job) && fix_file_permissions(fname);
}

bool job_cancel_mark_check(const JobId &id,const GMConfig &config) {
  std::string fname = config.ControlDir() + "/" + subdir_new + "/" + id + sfx_cancel;
  return job_mark_check(fname);
}

bool job_cancel_mark_remove(const JobId &id,const GMConfig &config) {
  std::string fname = config.ControlDir() + "/" + subdir_new + "/" + id + sfx_cancel;
  return job_mark_remove(fname);
}

bool job_restart_mark_put(const GMJob &job,const GMConfig &config) {
  std::string fname = config.ControlDir() + "/" + subdir_new + "/" + job.get_id() + sfx_restart;
  return job_mark_put(fname) && fix_file_owner(fname,job) && fix_file_permissions(fname);
}

bool job_restart_mark_check(const JobId &id,const GMConfig &config) {
  std::string fname = config.ControlDir() + "/" + subdir_new + "/" + id + sfx_restart;
  return job_mark_check(fname);
}

bool job_restart_mark_remove(const JobId &id,const GMConfig &config) {
  std::string fname = config.ControlDir() + "/" + subdir_new + "/" + id + sfx_restart;
  return job_mark_remove(fname);
}

bool job_clean_mark_put(const GMJob &job,const GMConfig &config) {
  std::string fname = config.ControlDir() + "/" + subdir_new + "/" + job.get_id() + sfx_clean;
  return job_mark_put(fname) && fix_file_owner(fname,job) && fix_file_permissions(fname);
}

bool job_clean_mark_check(const JobId &id,const GMConfig &config) {
  std::string fname = config.ControlDir() + "/" + subdir_new + "/" + id + sfx_clean;
  return job_mark_check(fname);
}

bool job_clean_mark_remove(const JobId &id,const GMConfig &config) {
  std::string fname = config.ControlDir() + "/" + subdir_new + "/" + id + sfx_clean;
  return job_mark_remove(fname);
}

bool job_failed_mark_put(const GMJob &job,const GMConfig &config,const std::string &content) {
  std::string fname = job_control_path(config.ControlDir(), job.get_id(), sfx_failed);
  if(job_mark_size(fname) > 0) return true;
  return job_mark_write(fname,content) && fix_file_owner(fname,job) && fix_file_permissions(fname,job,config);
}

bool job_failed_mark_add(const GMJob &job,const GMConfig &config,const std::string &content) {
  std::string fname = job_control_path(config.ControlDir(), job.get_id(), sfx_failed);
  return job_mark_add(fname,content) && fix_file_owner(fname,job) && fix_file_permissions(fname,job,config);
}

bool job_failed_mark_check(const JobId &id,const GMConfig &config) {
  std::string fname = job_control_path(config.ControlDir(), id, sfx_failed);
  return job_mark_check(fname);
}

bool job_failed_mark_remove(const JobId &id,const GMConfig &config) {
  std::string fname = job_control_path(config.ControlDir(), id, sfx_failed);
  return job_mark_remove(fname);
}

std::string job_failed_mark_read(const JobId &id,const GMConfig &config) {
  std::string fname = job_control_path(config.ControlDir(), id, sfx_failed);
  return job_mark_read(fname);
}

bool job_controldiag_mark_put(const GMJob &job,const GMConfig &config,char const * const args[]) {
  std::string fname = job_control_path(config.ControlDir(), job.get_id(), sfx_diag);
  if(!job_mark_put(fname)) return false;
  if(!fix_file_owner(fname,job)) return false;
  if(!fix_file_permissions(fname)) return false;
  if(args == NULL) return true;
  struct stat st;
  if(args[0] && stat(args[0], &st) != 0) return true;
  int h = open(fname.c_str(),O_WRONLY);
  if(h == -1) return false;
  int r;
  int t = 10;
  r=RunRedirected::run(job.get_user(),"job_controldiag_mark_put",-1,h,-1,(char**)args,t);
  close(h);
  if(r != 0) return false;
  return true;
}

bool job_diagnostics_mark_put(const GMJob &job,const GMConfig &config) {
  std::string fname = job.SessionDir() + "." + sfx_diag;
  if(config.StrictSession()) {
    Arc::FileAccess fa;
    if(!fa.fa_setuid(job.get_user().get_uid(),job.get_user().get_gid())) return false;
    return job_mark_put(fa,fname) && fix_file_permissions(fa,fname);
  };
  return job_mark_put(fname) && fix_file_owner(fname,job) && fix_file_permissions(fname);
}

bool job_diagnostics_mark_remove(const GMJob &job,const GMConfig &config) {
  std::string fname = job_control_path(config.ControlDir(), job.get_id(), sfx_diag);
  bool res1 = job_mark_remove(fname);
  fname = job.SessionDir() + "." + sfx_diag;
  if(config.StrictSession()) {
    Arc::FileAccess fa;
    if(!fa.fa_setuid(job.get_user().get_uid(),job.get_user().get_gid())) return res1;
    return (res1 | job_mark_remove(fa,fname));
  };
  return (res1 | job_mark_remove(fname));
}

bool job_diagnostics_mark_move(const GMJob &job,const GMConfig &config) {
  std::string fname1;
  if (job.GetLocalDescription() && !job.GetLocalDescription()->sessiondir.empty())
    fname1 = job.GetLocalDescription()->sessiondir + "." + sfx_diag;
  else
    fname1 = job.SessionDir() + "." + sfx_diag;
  std::string fname2 = job_control_path(config.ControlDir(), job.get_id(), sfx_diag);

  std::string data;
  if(config.StrictSession()) {
    Arc::FileRead(fname1, data, job.get_user().get_uid(), job.get_user().get_gid());
    Arc::FileDelete(fname1, job.get_user().get_uid(), job.get_user().get_gid());
  }
  else {
    Arc::FileRead(fname1, data);
    Arc::FileDelete(fname1);
  }
  // behaviour is to create file in control dir even if reading fails
  return Arc::FileCreate(fname2, data) && fix_file_owner(fname2,job) && fix_file_permissions(fname2,job,config);
}

bool job_lrmsoutput_mark_put(const GMJob &job,const GMConfig &config) {
  std::string fname = job.SessionDir() + "." + sfx_lrmsoutput;
  if(config.StrictSession()) {
    Arc::FileAccess fa;
    if(!fa.fa_setuid(job.get_user().get_uid(),job.get_user().get_gid())) return false;
    return job_mark_put(fa,fname) && fix_file_permissions(fa,fname);
  };
  return job_mark_put(fname) && fix_file_owner(fname,job) && fix_file_permissions(fname);
}

bool job_lrmsoutput_mark_remove(const GMJob &job,const GMConfig &config) {
  std::string fname = job.SessionDir() + "." + sfx_lrmsoutput;
  if(config.StrictSession()) {
    Arc::FileAccess fa;
    if(!fa.fa_setuid(job.get_user().get_uid(),job.get_user().get_gid())) return false;
    return job_mark_remove(fa,fname);
  };
  return job_mark_remove(fname);
}

std::string job_mark_read(const std::string &fname) {
  std::string s("");
  Arc::FileRead(fname, s);
  return s;
}

bool job_mark_write(const std::string &fname,const std::string &content) {
  return Arc::FileCreate(fname, content);
}

bool job_mark_add(const std::string &fname,const std::string &content) {
  int h=open(fname.c_str(),O_WRONLY | O_CREAT | O_APPEND,S_IRUSR | S_IWUSR);
  if(h==-1) return false;
  write(h,(const void *)content.c_str(),content.length());
  close(h); return true;
}

bool job_mark_put(const std::string &fname) {
  int h=open(fname.c_str(),O_WRONLY | O_CREAT,S_IRUSR | S_IWUSR);
  if(h==-1) return false;
  close(h); return true;
}

static bool job_mark_put(Arc::FileAccess& fa, const std::string &fname) {
  if(!fa.fa_open(fname,O_WRONLY | O_CREAT,S_IRUSR | S_IWUSR)) return false;
  fa.fa_close(); return true;
}

bool job_mark_check(const std::string &fname) {
  struct stat st;
  if(lstat(fname.c_str(),&st) != 0) return false;
  if(!S_ISREG(st.st_mode)) return false;
  return true;
}

bool job_mark_remove(const std::string &fname) {
  if(unlink(fname.c_str()) != 0) { if(errno != ENOENT) return false; };
  return true;
}

static bool job_mark_remove(Arc::FileAccess& fa,const std::string &fname) {
  if(!fa.fa_unlink(fname)) {
    if(fa.geterrno() != ENOENT) return false;
  };
  return true;
}

time_t job_mark_time(const std::string &fname) {
  struct stat st;
  if(lstat(fname.c_str(),&st) != 0) return 0;
  if(st.st_mtime == 0) st.st_mtime = 1; // doomsday protection
  return st.st_mtime;
}

long int job_mark_size(const std::string &fname) {
  struct stat st;
  if(lstat(fname.c_str(),&st) != 0) return 0;
  if(!S_ISREG(st.st_mode)) return 0;
  return st.st_size;
}

bool job_errors_mark_put(const GMJob &job,const GMConfig &config) {
  std::string fname = job_control_path(config.ControlDir(), job.get_id(), sfx_errors);
  return job_mark_put(fname) && fix_file_owner(fname,job) && fix_file_permissions(fname);
}

bool job_errors_mark_add(const GMJob &job,const GMConfig &config,const std::string &msg) {
  std::string fname = job_control_path(config.ControlDir(), job.get_id(), sfx_errors);
  return job_mark_add(fname,msg) && fix_file_owner(fname,job) && fix_file_permissions(fname);
}

std::string job_errors_filename(const JobId &id, const GMConfig &config) {
  return job_control_path(config.ControlDir(), id, sfx_errors);
}


time_t job_state_time(const JobId &id,const GMConfig &config) {
  std::string fname;
  time_t t;
  fname = config.ControlDir() + "/" + subdir_cur + "/" + id + sfx_status;
  t = job_mark_time(fname);
  if(t != 0) return t;
  fname = config.ControlDir() + "/" + subdir_new + "/" + id + sfx_status;
  t = job_mark_time(fname);
  if(t != 0) return t;
  fname = config.ControlDir() + "/" + subdir_rew + "/" + id + sfx_status;
  t = job_mark_time(fname);
  if(t != 0) return t;
  fname = config.ControlDir() + "/" + subdir_old + "/" + id + sfx_status;
  return job_mark_time(fname);
}

job_state_t job_state_read_file(const JobId &id,const GMConfig &config) {
  bool pending;
  return job_state_read_file(id, config, pending);
}

job_state_t job_state_read_file(const JobId &id,const GMConfig &config,bool& pending) {
  std::string fname;
  job_state_t st;
  fname = config.ControlDir() + "/" + subdir_cur + "/" + id + sfx_status;
  st = job_state_read_file(fname,pending);
  if(st != JOB_STATE_DELETED) return st;
  fname = config.ControlDir() + "/" + subdir_new + "/" + id + sfx_status;
  st = job_state_read_file(fname,pending);
  if(st != JOB_STATE_DELETED) return st;
  fname = config.ControlDir() + "/" + subdir_rew + "/" + id + sfx_status;
  st = job_state_read_file(fname,pending);
  if(st != JOB_STATE_DELETED) return st;
  fname = config.ControlDir() + "/" + subdir_old + "/" + id + sfx_status;
  return job_state_read_file(fname,pending);
}

bool job_state_write_file(const GMJob &job,const GMConfig &config,job_state_t state,bool pending) {
  std::string fname;
  if(state == JOB_STATE_ACCEPTED) { 
    fname = config.ControlDir() + "/" + subdir_old + "/" + job.get_id() + sfx_status; remove(fname.c_str());
    fname = config.ControlDir() + "/" + subdir_cur + "/" + job.get_id() + sfx_status; remove(fname.c_str());
    fname = config.ControlDir() + "/" + subdir_rew + "/" + job.get_id() + sfx_status; remove(fname.c_str());
    fname = config.ControlDir() + "/" + subdir_new + "/" + job.get_id() + sfx_status;
  } else if((state == JOB_STATE_FINISHED) || (state == JOB_STATE_DELETED)) {
    fname = config.ControlDir() + "/" + subdir_new + "/" + job.get_id() + sfx_status; remove(fname.c_str());
    fname = config.ControlDir() + "/" + subdir_cur + "/" + job.get_id() + sfx_status; remove(fname.c_str());
    fname = config.ControlDir() + "/" + subdir_rew + "/" + job.get_id() + sfx_status; remove(fname.c_str());
    fname = config.ControlDir() + "/" + subdir_old + "/" + job.get_id() + sfx_status;
  } else {
    fname = config.ControlDir() + "/" + subdir_new + "/" + job.get_id() + sfx_status; remove(fname.c_str());
    fname = config.ControlDir() + "/" + subdir_old + "/" + job.get_id() + sfx_status; remove(fname.c_str());
    fname = config.ControlDir() + "/" + subdir_rew + "/" + job.get_id() + sfx_status; remove(fname.c_str());
    fname = config.ControlDir() + "/" + subdir_cur + "/" + job.get_id() + sfx_status;
  };
  return job_state_write_file(fname,state,pending) && fix_file_owner(fname,job) && fix_file_permissions(fname,job,config);
}

static job_state_t job_state_read_file(const std::string &fname,bool &pending) {

  std::string data;
  if(!Arc::FileRead(fname, data)) {
    if(!job_mark_check(fname)) return JOB_STATE_DELETED; /* job does not exist */
    return JOB_STATE_UNDEFINED; /* can't open file */
  };
  data = data.substr(0, data.find('\n'));
  /* interpret information */
  if(data.substr(0, 8) == "PENDING:") {
    data = data.substr(8); pending=true;
  } else {
    pending=false;
  };
  return GMJob::get_state(data.c_str());
}

static bool job_state_write_file(const std::string &fname,job_state_t state,bool pending) {
  std::string data;
  if (pending) data += "PENDING:";
  data += GMJob::get_state_name(state);
  return Arc::FileCreate(fname, data);
}

time_t job_description_time(const JobId &id,const GMConfig &config) {
  std::string fname = job_control_path(config.ControlDir(), id, sfx_desc);
  return job_mark_time(fname);
}

bool job_description_read_file(const JobId &id,const GMConfig &config,std::string &desc) {
  std::string fname = job_control_path(config.ControlDir(), id, sfx_desc);
  return job_description_read_file(fname,desc);
}

bool job_description_read_file(const std::string &fname,std::string &desc) {
  if (!Arc::FileRead(fname, desc)) return false;
  while (desc.find('\n') != std::string::npos) desc.erase(desc.find('\n'), 1);
  return true;
}

bool job_description_write_file(const GMJob &job,const GMConfig &config,const std::string &desc) {
  std::string fname = job_control_path(config.ControlDir(), job.get_id(), sfx_desc);
  return Arc::FileCreate(fname, desc) && fix_file_owner(fname,job) && fix_file_permissions(fname,job,config);
}

bool job_acl_read_file(const JobId &id,const GMConfig &config,std::string &acl) {
  std::string fname = job_control_path(config.ControlDir(), id, sfx_acl);
  return job_description_read_file(fname,acl);
}

bool job_acl_write_file(const JobId &id,const GMConfig &config,const std::string &acl) {
  std::string fname = job_control_path(config.ControlDir(), id, sfx_acl);
  return Arc::FileCreate(fname, acl);
}

bool job_xml_read_file(const JobId &id,const GMConfig &config,std::string &xml) {
  std::string fname = job_control_path(config.ControlDir(), id, sfx_xml);
  return job_description_read_file(fname,xml);
}

bool job_xml_write_file(const JobId &id,const GMConfig &config,const std::string &xml) {
  std::string fname = job_control_path(config.ControlDir(), id, sfx_xml);
  return Arc::FileCreate(fname, xml);
}

bool job_local_write_file(const GMJob &job,const GMConfig &config,const JobLocalDescription &job_desc) {
  std::string fname = job_control_path(config.ControlDir(), job.get_id(), sfx_local);
  return job_local_write_file(fname,job_desc) && fix_file_owner(fname,job) && fix_file_permissions(fname,job,config);
}

bool job_local_write_file(const std::string &fname,const JobLocalDescription &job_desc) {
  return job_desc.write(fname);
}

bool job_local_read_file(const JobId &id,const GMConfig &config,JobLocalDescription &job_desc) {
  std::string fname = job_control_path(config.ControlDir(), id, sfx_local);
  return job_local_read_file(fname,job_desc);
}

bool job_local_read_file(const std::string &fname,JobLocalDescription &job_desc) {
  return job_desc.read(fname);
}

bool job_local_read_var(const std::string &fname,const std::string &vnam,std::string &value) {
  return JobLocalDescription::read_var(fname,vnam,value);
}

bool job_local_read_cleanuptime(const JobId &id,const GMConfig &config,time_t &cleanuptime) {
  std::string fname = job_control_path(config.ControlDir(), id, sfx_local);
  std::string str;
  if(!job_local_read_var(fname,"cleanuptime",str)) return false;
  cleanuptime=Arc::Time(str).GetTime();
  return true;
}

bool job_local_read_failed(const JobId &id,const GMConfig &config,std::string &state,std::string &cause) {
  state = "";
  cause = "";
  std::string fname = job_control_path(config.ControlDir(), id, sfx_local);
  job_local_read_var(fname,"failedstate",state);
  job_local_read_var(fname,"failedcause",cause);
  return true;
}

bool job_local_read_delegationid(const JobId &id,const GMConfig &config,std::string &delegationid) {
  std::string fname = job_control_path(config.ControlDir(), id, sfx_local);
  if(!job_local_read_var(fname,"cleanuptime",delegationid)) return false;
  return true;
}

/* job.ID.input functions */

bool job_input_write_file(const GMJob &job,const GMConfig &config,std::list<FileData> &files) {
  std::string fname = job_control_path(config.ControlDir(), job.get_id(), sfx_input);
  return job_Xput_write_file(fname,files) && fix_file_owner(fname,job) && fix_file_permissions(fname);
}

bool job_input_read_file(const JobId &id,const GMConfig &config,std::list<FileData> &files) {
  std::string fname = job_control_path(config.ControlDir(), id, sfx_input);
  return job_Xput_read_file(fname,files);
}

bool job_input_status_add_file(const GMJob &job,const GMConfig &config,const std::string& file) {
  // 1. lock
  // 2. add
  // 3. unlock
  std::string fname = job_control_path(config.ControlDir(), job.get_id(), sfx_inputstatus);
  Arc::FileLock lock(fname);
  for (int i = 10; !lock.acquire() && i >= 0; --i)  {
    if (i == 0) return false;
    sleep(1);
  }
  std::string data;
  if (!Arc::FileRead(fname, data) && errno != ENOENT) {
    lock.release();
    return false;
  }
  std::ostringstream line;
  line<<file<<"\n";
  data += line.str();
  bool r = Arc::FileCreate(fname, data);
  lock.release();
  return r && fix_file_owner(fname,job) && fix_file_permissions(fname);
}

bool job_input_status_read_file(const JobId &id,const GMConfig &config,std::list<std::string>& files) {
  std::string fname = job_control_path(config.ControlDir(), id, sfx_inputstatus);
  Arc::FileLock lock(fname);
  for (int i = 10; !lock.acquire() && i >= 0; --i) {
    if (i == 0) return false;
    sleep(1);
  }
  bool r = Arc::FileRead(fname, files);
  lock.release();
  return r;
}

/* job.ID.output functions */
bool job_output_write_file(const GMJob &job,const GMConfig &config,std::list<FileData> &files,job_output_mode mode) {
  std::string fname = job_control_path(config.ControlDir(), job.get_id(), sfx_output);
  return job_Xput_write_file(fname,files,mode) && fix_file_owner(fname,job) && fix_file_permissions(fname);
}

bool job_output_read_file(const JobId &id,const GMConfig &config,std::list<FileData> &files) {
  std::string fname = job_control_path(config.ControlDir(), id, sfx_output);
  return job_Xput_read_file(fname,files);
}

bool job_output_status_add_file(const GMJob &job,const GMConfig &config,const FileData& file) {
  // Not using lock here because concurrent read/write is not expected
  std::string fname = job_control_path(config.ControlDir(), job.get_id(), sfx_outputstatus);
  std::string data;
  if (!Arc::FileRead(fname, data) && errno != ENOENT) return false;
  std::ostringstream line;
  line<<file<<"\n";
  data += line.str();
  return Arc::FileCreate(fname, data) && fix_file_owner(fname,job) && fix_file_permissions(fname);
}

bool job_output_status_write_file(const GMJob &job,const GMConfig &config,std::list<FileData> &files) {
  std::string fname = job_control_path(config.ControlDir(), job.get_id(), sfx_outputstatus);
  return job_Xput_write_file(fname,files) && fix_file_owner(fname,job) && fix_file_permissions(fname);
}

bool job_output_status_read_file(const JobId &id,const GMConfig &config,std::list<FileData> &files) {
  std::string fname = job_control_path(config.ControlDir(), id, sfx_outputstatus);
  return job_Xput_read_file(fname,files);
}

/* common functions */

bool job_Xput_write_file(const std::string &fname,std::list<FileData> &files,job_output_mode mode, uid_t uid, gid_t gid) {
  std::ostringstream s;
  for(FileData::iterator i=files.begin();i!=files.end(); ++i) { 
    if(mode == job_output_all) {
      s << (*i) << std::endl;
    } else if(mode == job_output_success) {
      if(i->ifsuccess) {
        s << (*i) << std::endl;
      } else {
        // This case is handled at higher level
      };
    } else if(mode == job_output_cancel) {
      if(i->ifcancel) {
        s << (*i) << std::endl;
      } else {
        // This case is handled at higher level
      };
    } else if(mode == job_output_failure) {
      if(i->iffailure) {
        s << (*i) << std::endl;
      } else {
        // This case is handled at higher level
      };
    };
  };
  if (!Arc::FileCreate(fname, s.str(), uid, gid)) return false;
  return true;
}

bool job_Xput_read_file(const std::string &fname,std::list<FileData> &files, uid_t uid, gid_t gid) {
  std::list<std::string> file_content;
  if (!Arc::FileRead(fname, file_content, uid, gid)) return false;
  for(std::list<std::string>::iterator i = file_content.begin(); i != file_content.end(); ++i) {
    FileData fd;
    std::istringstream s(*i);
    s >> fd;
    if(!fd.pfn.empty()) files.push_back(fd);
  };
  return true;
}

std::string job_proxy_filename(const JobId &id, const GMConfig &config){
  return job_control_path(config.ControlDir(), id, sfx_proxy);
}

bool job_proxy_write_file(const GMJob &job,const GMConfig &config,const std::string &cred) {
  std::string fname = job_control_path(config.ControlDir(), job.get_id(), sfx_proxy);
  return Arc::FileCreate(fname, cred, 0, 0, S_IRUSR | S_IWUSR) && fix_file_owner(fname,job);
}

bool job_proxy_read_file(const JobId &id,const GMConfig &config,std::string &cred) {
  std::string fname = job_control_path(config.ControlDir(), id, sfx_proxy);
  return Arc::FileRead(fname, cred, 0, 0);
}

bool job_clean_finished(const JobId &id,const GMConfig &config) {
  std::string fname;
  fname = job_control_path(config.ControlDir(), id, sfx_proxy_tmp); remove(fname.c_str());
  fname = job_control_path(config.ControlDir(), id, sfx_lrms_done); remove(fname.c_str());
  return true;
}

bool job_clean_deleted(const GMJob &job,const GMConfig &config,std::list<std::string> cache_per_job_dirs) {
  std::string id = job.get_id();
  job_clean_finished(id,config);
  std::string session;
  if(job.GetLocalDescription() && !job.GetLocalDescription()->sessiondir.empty())
    session = job.GetLocalDescription()->sessiondir;
  else
    session = job.SessionDir();
  std::string fname;
  fname = job_control_path(config.ControlDir(),id,sfx_proxy); remove(fname.c_str());
  fname = config.ControlDir()+"/"+subdir_new+"/"+id+sfx_restart; remove(fname.c_str());
  fname = job_control_path(config.ControlDir(),id,sfx_errors); remove(fname.c_str());
  fname = config.ControlDir()+"/"+subdir_new+"/"+id+sfx_cancel; remove(fname.c_str());
  fname = config.ControlDir()+"/"+subdir_new+"/"+id+sfx_clean;  remove(fname.c_str());
  fname = job_control_path(config.ControlDir(),id,sfx_output); remove(fname.c_str());
  fname = job_control_path(config.ControlDir(),id,sfx_input); remove(fname.c_str());
  fname = job_control_path(config.ControlDir(),id,"grami_log"); remove(fname.c_str());
  fname = session+"."+sfx_lrmsoutput; remove(fname.c_str());
  fname = job_control_path(config.ControlDir(),id,sfx_outputstatus); remove(fname.c_str());
  fname = job_control_path(config.ControlDir(),id,sfx_inputstatus); remove(fname.c_str());
  fname = job_control_path(config.ControlDir(),id,sfx_statistics); remove(fname.c_str());
  /* remove session directory */
  if(config.StrictSession()) {
    Arc::DirDelete(session, true, job.get_user().get_uid(), job.get_user().get_gid());
  } else {
    Arc::DirDelete(session);
  }
  // remove cache per-job links, in case this failed earlier
  for (std::list<std::string>::iterator i = cache_per_job_dirs.begin(); i != cache_per_job_dirs.end(); i++) {
    Arc::DirDelete((*i) + "/" + id);
  }
  return true;
}

bool job_clean_final(const GMJob &job,const GMConfig &config) {
  std::string id = job.get_id();
  job_clean_finished(id,config);
  job_clean_deleted(job,config);
  std::string fname;
  fname = job_control_path(config.ControlDir(),id,sfx_local);  remove(fname.c_str());
  fname = job_control_path(config.ControlDir(),id,sfx_grami); remove(fname.c_str());
  fname = job_control_path(config.ControlDir(),id,sfx_failed); remove(fname.c_str());
  job_diagnostics_mark_remove(job,config);
  job_lrmsoutput_mark_remove(job,config);
  fname = config.ControlDir()+"/"+subdir_new+"/"+id+sfx_status; remove(fname.c_str());
  fname = config.ControlDir()+"/"+subdir_cur+"/"+id+sfx_status; remove(fname.c_str());
  fname = config.ControlDir()+"/"+subdir_old+"/"+id+sfx_status; remove(fname.c_str());
  fname = config.ControlDir()+"/"+subdir_rew+"/"+id+sfx_status; remove(fname.c_str());
  fname = job_control_path(config.ControlDir(),id,sfx_desc); remove(fname.c_str());
  fname = job_control_path(config.ControlDir(),id,sfx_xml); remove(fname.c_str());
  return true;
}

} // namespace ARex
