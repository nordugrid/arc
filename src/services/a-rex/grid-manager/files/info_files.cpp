#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

/*
 routines to work with informational files
*/

#include <string>
#include <list>
#include <iostream>
#include <fstream>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <arc/StringConv.h>
#include "../files/delete.h"
#include "../misc/escaped.h"

#include "../run/run_function.h"
#include "../run/run_redirected.h"

#include "../conf/conf.h"
//@ #include <arc/certificate.h>
#include "info_types.h"
#include "info_files.h"

#if defined __GNUC__ && __GNUC__ >= 3

#define istream_readline(__f,__s,__n) {      \
   __f.get(__s,__n,__f.widen('\n'));         \
   if(__f.fail()) __f.clear();               \
   __f.ignore(std::numeric_limits<std::streamsize>::max(), __f.widen('\n')); \
}

#else

#define istream_readline(__f,__s,__n) {      \
   __f.get(__s,__n,'\n');         \
   if(__f.fail()) __f.clear();               \
   __f.ignore(INT_MAX,'\n'); \
}

#endif


const char * const sfx_failed      = ".failed";
const char * const sfx_cancel      = ".cancel";
const char * const sfx_restart     = ".restart";
const char * const sfx_clean       = ".clean";
const char * const sfx_status      = ".status";
const char * const sfx_local       = ".local";
const char * const sfx_errors      = ".errors";
const char * const sfx_rsl         = ".description";
const char * const sfx_diag        = ".diag";
const char * const sfx_lrmsoutput  = ".comment";
const char * const sfx_diskusage   = ".disk";
const char * const sfx_acl         = ".acl";

static Arc::Logger& logger = Arc::Logger::getRootLogger();

bool fix_file_permissions(const std::string &fname,bool executable) {
  mode_t mode = S_IRUSR | S_IWUSR;
  if(executable) { mode |= S_IXUSR; };
  return (chmod(fname.c_str(),mode) == 0);
}

static bool fix_file_permissions(const std::string &fname,const JobUser &user) {
  mode_t mode = S_IRUSR | S_IWUSR;
  if(user.ShareLevel() == JobUser::jobinfo_share_group) {
    mode |= S_IRGRP;
  } else if(user.ShareLevel() == JobUser::jobinfo_share_all) {
    mode |= S_IRGRP | S_IROTH;
  };
  return (chmod(fname.c_str(),mode) == 0);
}

bool fix_file_owner(const std::string &fname,const JobUser &user) {
  if(getuid() == 0) {
    if(lchown(fname.c_str(),user.get_uid(),user.get_gid()) == -1) {
      logger.msg(Arc::ERROR,"Failed setting file owner: %s",fname.c_str());
      return false;
    };
  };
  return true;
}

bool fix_file_owner(const std::string &fname,const JobDescription &desc,const JobUser &user) {
  if(getuid() == 0) {
    uid_t uid = desc.get_uid();
    gid_t gid = desc.get_gid();
    if( uid == 0 ) {
      uid=user.get_uid(); gid=user.get_gid();
    };
    if(lchown(fname.c_str(),uid,gid) == -1) {
      logger.msg(Arc::ERROR,"Failed setting file owner: %s",fname.c_str());
      return false;
    };
  };
  return true;
}

bool check_file_owner(const std::string &fname,const JobUser &user) {
  uid_t uid;
  gid_t gid;
  time_t t;
  return check_file_owner(fname,user,uid,gid,t);
}

bool check_file_owner(const std::string &fname,const JobUser &user,uid_t &uid,gid_t &gid) {
  time_t t;
  return check_file_owner(fname,user,uid,gid,t);
}

bool check_file_owner(const std::string &fname,const JobUser &user,uid_t &uid,gid_t &gid,time_t &t) {
  struct stat st;
  if(lstat(fname.c_str(),&st) != 0) return false;
  if(!S_ISREG(st.st_mode)) return false;
  uid=st.st_uid; gid=st.st_gid; t=st.st_ctime;
  /* superuser can't run jobs */
  if(uid == 0) return false;
  /* accept any file if superuser */
  if(user.get_uid() != 0) {
    if(uid != user.get_uid()) return false;
  };
  return true;
}

bool job_lrms_mark_put(const JobDescription &desc,JobUser &user,int code) {
  LRMSResult r(code);
  return job_lrms_mark_put(desc,user,r);
}

bool job_lrms_mark_put(const JobDescription &desc,JobUser &user,LRMSResult r) {
  std::string fname = user.ControlDir()+"/job."+desc.get_id()+".lrms_done";
  std::string content = Arc::tostring(r.code());
  content+=" ";
  content+=r.description();
  return job_mark_write_s(fname,content) & fix_file_owner(fname,desc,user) & fix_file_permissions(fname);
}

bool job_lrms_mark_check(JobId &id,JobUser &user) {
  std::string fname = user.ControlDir() + "/job." + id + ".lrms_done";
  return job_mark_check(fname);
}

bool job_lrms_mark_remove(JobId &id,JobUser &user) {
  std::string fname = user.ControlDir() + "/job." + id + ".lrms_done";
  return job_mark_remove(fname);
}

LRMSResult job_lrms_mark_read(JobId &id,JobUser &user) {
  std::string fname = user.ControlDir() + "/job." + id + ".lrms_done";
  LRMSResult r("-1 Internal error");
  std::ifstream f(fname.c_str()); if(! f.is_open() ) return r;
  f>>r;
  return r;
}

bool job_cancel_mark_put(const JobDescription &desc,JobUser &user) {
  std::string fname = user.ControlDir() + "/job." + desc.get_id() + sfx_cancel;
  return job_mark_put(fname) & fix_file_owner(fname,desc,user) & fix_file_permissions(fname);
}

bool job_cancel_mark_check(JobId &id,JobUser &user) {
  std::string fname = user.ControlDir() + "/job." + id + sfx_cancel;
  return job_mark_check(fname);
}

bool job_cancel_mark_remove(JobId &id,JobUser &user) {
  std::string fname = user.ControlDir() + "/job." + id + sfx_cancel;
  return job_mark_remove(fname);
}

bool job_restart_mark_put(const JobDescription &desc,JobUser &user) {
  std::string fname = user.ControlDir() + "/job." + desc.get_id() + sfx_restart;
  return job_mark_put(fname) & fix_file_owner(fname,desc,user) & fix_file_permissions(fname);
}

bool job_restart_mark_check(JobId &id,JobUser &user) {
  std::string fname = user.ControlDir() + "/job." + id + sfx_restart;
  return job_mark_check(fname);
}

bool job_restart_mark_remove(JobId &id,JobUser &user) {
  std::string fname = user.ControlDir() + "/job." + id + sfx_restart;
  return job_mark_remove(fname);
}

bool job_clean_mark_put(const JobDescription &desc,JobUser &user) {
  std::string fname = user.ControlDir() + "/job." + desc.get_id() + sfx_clean;
  return job_mark_put(fname) & fix_file_owner(fname,desc,user) & fix_file_permissions(fname);
}

bool job_clean_mark_check(JobId &id,JobUser &user) {
  std::string fname = user.ControlDir() + "/job." + id + sfx_clean;
  return job_mark_check(fname);
}

bool job_clean_mark_remove(JobId &id,JobUser &user) {
  std::string fname = user.ControlDir() + "/job." + id + sfx_clean;
  return job_mark_remove(fname);
}

bool job_errors_mark_put(const JobDescription &desc,JobUser &user) {
  std::string fname = user.ControlDir() + "/job." + desc.get_id() + sfx_errors;
  return job_mark_put(fname) & fix_file_owner(fname,desc,user) & fix_file_permissions(fname);
}

bool job_failed_mark_put(const JobDescription &desc,JobUser &user,const std::string &content) {
  std::string fname = user.ControlDir() + "/job." + desc.get_id() + sfx_failed;
  if(job_mark_size(fname) > 0) return true;
  return job_mark_write_s(fname,content) & fix_file_owner(fname,desc,user) & fix_file_permissions(fname,user);
}

bool job_failed_mark_add(const JobDescription &desc,JobUser &user,const std::string &content) {
  std::string fname = user.ControlDir() + "/job." + desc.get_id() + sfx_failed;
  return job_mark_add_s(fname,content) & fix_file_owner(fname,desc,user) & fix_file_permissions(fname,user);
}

bool job_failed_mark_check(const JobId &id,JobUser &user) {
  std::string fname = user.ControlDir() + "/job." + id + sfx_failed;
  return job_mark_check(fname);
}

bool job_failed_mark_remove(const JobId &id,JobUser &user) {
  std::string fname = user.ControlDir() + "/job." + id + sfx_failed;
  return job_mark_remove(fname);
}

std::string job_failed_mark_read(const JobId &id,JobUser &user) {
  std::string fname = user.ControlDir() + "/job." + id + sfx_failed;
  return job_mark_read_s(fname);
}

static int job_mark_put_callback(void* arg) {
  std::string& fname = *((std::string*)arg);
  if(job_mark_put(fname) & fix_file_permissions(fname)) return 0;
  return -1;
}

static int job_mark_remove_callback(void* arg) {
  std::string& fname = *((std::string*)arg);
  if(job_mark_remove(fname)) return 0;
  return -1;
}

typedef struct {
  const std::string* fname;
  const std::string* content;
} job_mark_add_t;

static int job_mark_add_callback(void* arg) {
  const std::string& fname = *(((job_mark_add_t*)arg)->fname);
  const std::string& content = *(((job_mark_add_t*)arg)->content);
  if(job_mark_add_s(fname,content) & fix_file_permissions(fname)) return 0;
  return -1;
}

typedef struct {
  const std::string* dname;
  const std::list<FileData>* flist;
} job_dir_remove_t;

static int job_dir_remove_callback(void* arg) {
  const std::string& dname = *(((job_dir_remove_t*)arg)->dname);
  const std::list<FileData>& flist = *(((job_dir_remove_t*)arg)->flist);
  delete_all_files(dname,flist,true);
  remove(dname.c_str());
  return 0;
}

bool job_diagnostics_mark_put(const JobDescription &desc,JobUser &user) {
  std::string fname = desc.SessionDir() + sfx_diag;
  if(user.StrictSession()) {
    JobUser tmp_user(user.get_uid()==0?desc.get_uid():user.get_uid());
    return (RunFunction::run(tmp_user,"job_diagnostics_mark_put",&job_mark_put_callback,&fname,10) == 0);
  };
  return job_mark_put(fname) & fix_file_owner(fname,desc,user) & fix_file_permissions(fname);
}

bool job_controldiag_mark_put(const JobDescription &desc,JobUser &user,char const * const args[]) {
  std::string fname = user.ControlDir() + "/job." + desc.get_id() + sfx_diag;
  if(!job_mark_put(fname)) return false;
  if(!fix_file_owner(fname,desc,user)) return false;
  if(!fix_file_permissions(fname)) return false;
  if(args == NULL) return true;
  int h = open(fname.c_str(),O_WRONLY);
  if(h == -1) return false;
  int r;
  int t = 10;
  JobUser tmp_user((uid_t)0);
  r=RunRedirected::run(tmp_user,"job_controldiag_mark_put",-1,h,-1,(char**)args,t);
  close(h);
  if(r != 0) return false;
  return true;
}

bool job_lrmsoutput_mark_put(const JobDescription &desc,JobUser &user) {
  std::string fname = desc.SessionDir() + sfx_lrmsoutput;
  if(user.StrictSession()) {
    JobUser tmp_user(user.get_uid()==0?desc.get_uid():user.get_uid());
    return (RunFunction::run(tmp_user,"job_lrmsoutput_mark_put",&job_mark_put_callback,&fname,10) == 0);
  };
  return job_mark_put(fname) & fix_file_owner(fname,desc,user) & fix_file_permissions(fname);
}

bool job_diagnostics_mark_add(const JobDescription &desc,JobUser &user,const std::string &content) {
  std::string fname = desc.SessionDir() + sfx_diag;
  if(user.StrictSession()) {
    JobUser tmp_user(user.get_uid()==0?desc.get_uid():user.get_uid());
    job_mark_add_t arg; arg.fname=&fname; arg.content=&content;
    return (RunFunction::run(tmp_user,"job_diagnostics_mark_add",&job_mark_add_callback,&arg,10) == 0);
  };
  return job_mark_add_s(fname,content) & fix_file_owner(fname,desc,user) & fix_file_permissions(fname);
}

bool job_diagnostics_mark_remove(const JobDescription &desc,JobUser &user) {
  std::string fname = user.ControlDir() + "/job." + desc.get_id() + sfx_diag;
  bool res1 = job_mark_remove(fname);
  fname = desc.SessionDir() + sfx_diag;
  if(user.StrictSession()) {
    JobUser tmp_user(user.get_uid()==0?desc.get_uid():user.get_uid());
    return (res1 | (RunFunction::run(tmp_user,"job_diagnostics_mark_remove",&job_mark_remove_callback,&fname,10) == 0));
  };
  return (res1 | job_mark_remove(fname));
}

bool job_lrmsoutput_mark_remove(const JobDescription &desc,JobUser &user) {
  std::string fname = desc.SessionDir() + sfx_lrmsoutput;
  if(user.StrictSession()) {
    JobUser tmp_user(user.get_uid()==0?desc.get_uid():user.get_uid());
    return (RunFunction::run(tmp_user,"job_lrmsoutpur_mark_remove",&job_mark_remove_callback,&fname,10) == 0);
  };
  return job_mark_remove(fname);
}

typedef struct {
  int h;
  const std::string* fname;
} job_file_read_t;

static int job_file_read_callback(void* arg) {
  int h = ((job_file_read_t*)arg)->h;
  const std::string& fname = *(((job_file_read_t*)arg)->fname);
  int h1=open(fname.c_str(),O_RDONLY);
  if(h1==-1) return -1;
  char buf[256];
  int l;
  for(;;) {
    l=read(h1,buf,256);
    if((l==0) || (l==-1)) break;
    write(h,buf,l);
  };
  close(h1); close(h);
  unlink(fname.c_str());
  return 0;
}

bool job_diagnostics_mark_move(const JobDescription &desc,JobUser &user) {
  std::string fname2 = user.ControlDir() + "/job." + desc.get_id() + sfx_diag;
  int h2=open(fname2.c_str(),O_WRONLY | O_APPEND,S_IRUSR | S_IWUSR);
  if(h2==-1) return false;
  fix_file_owner(fname2,desc,user);
  fix_file_permissions(fname2,user);
  std::string fname1 = user.SessionRoot() + "/" + desc.get_id() + sfx_diag;
  if(user.StrictSession()) {
    JobUser tmp_user(user.get_uid()==0?desc.get_uid():user.get_uid());
    job_file_read_t arg; arg.h=h2; arg.fname=&fname1;
    RunFunction::run(tmp_user,"job_diagnostics_mark_move",&job_file_read_callback,&arg,10);
    close(h2);
    return true;
  };
  int h1=open(fname1.c_str(),O_RDONLY);
  if(h1==-1) { close(h2); return false; };
  char buf[256];
  int l;
  for(;;) {
    l=read(h1,buf,256);
    if((l==0) || (l==-1)) break;
    write(h2,buf,l);
  };
  close(h1); close(h2);
  unlink(fname1.c_str());
  return true;
}

bool job_stdlog_move(const JobDescription& /*desc*/,JobUser& /*user*/,const std::string& /*logname*/) {
/*

 Status data is now available during runtime.

*/
  return true;
}

bool job_mark_put(const std::string &fname) {
  int h=open(fname.c_str(),O_WRONLY | O_CREAT,S_IRUSR | S_IWUSR);
  if(h==-1) return false; close(h); return true;
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

std::string job_mark_read_s(const std::string &fname) {
  std::string s("");
  std::ifstream f(fname.c_str()); if(! f.is_open() ) return s;
  char buf[256]; f.getline(buf,254); s=buf;
  return s;
}

long int job_mark_read_i(const std::string &fname) {
  std::ifstream f(fname.c_str()); if(! f.is_open() ) return -1;
  char buf[32]; f.getline(buf,30); f.close();
  char* e; long int i;
  i=strtol(buf,&e,10); if((*e) == 0) return i;
  return -1;
}

bool job_mark_write_s(const std::string &fname,const std::string &content) {
  int h=open(fname.c_str(),O_WRONLY | O_CREAT | O_TRUNC,S_IRUSR | S_IWUSR);
  if(h==-1) return false;
  write(h,(const void *)content.c_str(),content.length());
  close(h); return true;
}

bool job_mark_add_s(const std::string &fname,const std::string &content) {
  int h=open(fname.c_str(),O_WRONLY | O_CREAT | O_APPEND,S_IRUSR | S_IWUSR);
  if(h==-1) return false;
  write(h,(const void *)content.c_str(),content.length());
  close(h); return true;
}

time_t job_mark_time(const std::string &fname) {
  struct stat st;
  if(lstat(fname.c_str(),&st) != 0) return 0;
  return st.st_mtime;
}

long int job_mark_size(const std::string &fname) {
  struct stat st;
  if(lstat(fname.c_str(),&st) != 0) return 0;
  if(!S_ISREG(st.st_mode)) return 0;
  return st.st_size;
}

bool job_diskusage_create_file(const JobDescription &desc,JobUser& /*user*/,unsigned long long int &requested) {
  std::string fname = desc.SessionDir() + sfx_diskusage;
  int h=open(fname.c_str(),O_WRONLY | O_CREAT,S_IRUSR | S_IWUSR);
  if(h==-1) return false;
  char content[200];
  sprintf(content,"%llu 0\n",requested);
  write(h,content,strlen(content));
  close(h); return true;
}

bool job_diskusage_read_file(const JobDescription &desc,JobUser& /*user*/,unsigned long long int &requested,unsigned long long int &used) {
  std::string fname = desc.SessionDir() + sfx_diskusage;
  int h=open(fname.c_str(),O_RDONLY);
  if(h==-1) return false;
  char content[200];
  ssize_t l=read(h,content,199);
  if(l==-1) { close(h); return false; };
  content[l]=0;
  unsigned long long int req_,use_;
  if(sscanf(content,"%llu %llu",&req_,&use_) != 2) { close(h); return false; };
  requested=req_; used=use_;  
  close(h); return true;
}

bool job_diskusage_change_file(const JobDescription &desc,JobUser& /*user*/,signed long long int used,bool &result) {
  // lock file, read, write, unlock
  std::string fname = desc.SessionDir() + sfx_diskusage;
  int h=open(fname.c_str(),O_RDWR);
  if(h==-1) return false;
  struct flock lock;
  lock.l_type=F_WRLCK;
  lock.l_whence=SEEK_SET;
  lock.l_start=0;
  lock.l_len=0;
  for(;;) {
    if(fcntl(h,F_SETLKW,&lock) != -1) break;
    if(errno == EINTR) continue;
    close(h); return false;
  };
  char content[200];
  ssize_t l=read(h,content,199);
  if(l==-1) {
    lock.l_whence=SEEK_SET; lock.l_start=0; lock.l_len=0;
    lock.l_type=F_UNLCK; fcntl(h,F_SETLK,&lock);
    close(h); return false;
  };
  content[l]=0;
  unsigned long long int req_,use_;
  if(sscanf(content,"%llu %llu",&req_,&use_) != 2) {
    lock.l_whence=SEEK_SET; lock.l_start=0; lock.l_len=0;
    lock.l_type=F_UNLCK; fcntl(h,F_SETLK,&lock);
    close(h); return false;
  };
  if((-used) > use_) {
    result=true; use_=0; // ??????
  }
  else {
    use_+=used; result=true;
    if(use_>req_) result=false;
  };
  lseek(h,0,SEEK_SET);  
  sprintf(content,"%llu %llu\n",req_,use_);
  write(h,content,strlen(content));
  lock.l_whence=SEEK_SET; lock.l_start=0; lock.l_len=0;
  lock.l_type=F_UNLCK; fcntl(h,F_SETLK,&lock);
  close(h); return true;
}

bool job_diskusage_remove_file(const JobDescription &desc,JobUser& /*user*/) {
  std::string fname = desc.SessionDir() + sfx_diskusage;
  return job_mark_remove(fname);
}

time_t job_state_time(const JobId &id,JobUser &user) {
  std::string fname = user.ControlDir() + "/job." + id + sfx_status;
  return job_mark_time(fname);
}

job_state_t job_state_read_file(const JobId &id,const JobUser &user) {
  std::string fname = user.ControlDir() + "/job." + id + sfx_status;
  bool pending;
  return job_state_read_file(fname,pending);
}

job_state_t job_state_read_file(const JobId &id,const JobUser &user,bool& pending) {
  std::string fname = user.ControlDir() + "/job." + id + sfx_status;
  return job_state_read_file(fname,pending);
}

bool job_state_write_file(const JobDescription &desc,JobUser &user,job_state_t state,bool pending) {
  std::string fname = user.ControlDir() + "/job." + desc.get_id() + sfx_status;
  return job_state_write_file(fname,state,pending) & fix_file_owner(fname,desc,user) & fix_file_permissions(fname,user);
}

job_state_t job_state_read_file(const std::string &fname,bool &pending) {
  std::ifstream f(fname.c_str());
  if(!f.is_open() ) return JOB_STATE_UNDEFINED; /* can't open file */
  char buf[32];
  f.getline(buf,30);
  /* interpret information */
  char* p = buf;
  if(!strncmp("PENDING:",p,8)) { p+=8; pending=true; } else { pending=false; };
  for(int i = 0;states_all[i].name != NULL;i++) {
    if(!strcmp(states_all[i].name,p)) {
      f.close();
      return states_all[i].id;
    };
  };
  f.close();
  return JOB_STATE_UNDEFINED; /* broken file */
}

bool job_state_write_file(const std::string &fname,job_state_t state,bool pending) {
  std::ofstream f(fname.c_str(),std::ios::out | std::ios::trunc);
  if(! f.is_open() ) return false; /* can't open file */
  if(pending) f<<"PENDING:";
  f<<states_all[state].name<<std::endl;
  f.close();
  return true;
}

bool job_description_read_file(JobId &id,JobUser &user,std::string &rsl) {
  std::string fname = user.ControlDir() + "/job." + id + sfx_rsl;
  return job_description_read_file(fname,rsl);
}

bool job_description_read_file(const std::string &fname,std::string &rsl) {
  char buf[256];
  std::string::size_type n=0;
  std::ifstream f(fname.c_str());
  if(! f.is_open() ) return false; /* can't open file */
  rsl.erase();
  while(!f.eof()) {
    memset(buf,0,256);
    f.read(buf,255); rsl+=buf;
    for(;(n=rsl.find('\n',n)) != std::string::npos;) rsl.erase(n,1);
    n=rsl.length();
  };
  f.close();
  return true;
}

bool job_description_write_file(const std::string &fname,std::string &rsl) {
  return job_description_write_file(fname,rsl.c_str());
}

bool job_description_write_file(const std::string &fname,const char* rsl) {
  std::ofstream f(fname.c_str(),std::ios::out | std::ios::trunc);
  if(! f.is_open() ) return false; /* can't open file */
  f.write(rsl,strlen(rsl));
  f.close();
  return true;
}

bool job_acl_read_file(JobId &id,JobUser &user,std::string &acl) {
  std::string fname = user.ControlDir() + "/job." + id + sfx_acl;
  return job_description_read_file(fname,acl);
}

bool job_acl_write_file(JobId &id,JobUser &user,std::string &acl) {
  std::string fname = user.ControlDir() + "/job." + id + sfx_acl;
  return job_description_write_file(fname,acl.c_str());
}

bool job_local_write_file(const JobDescription &desc,const JobUser &user,JobLocalDescription &job_desc) {
  std::string fname = user.ControlDir() + "/job." + desc.get_id() + sfx_local;
  return job_local_write_file(fname,job_desc) & fix_file_owner(fname,desc,user) & fix_file_permissions(fname,user);
}

inline void write_pair(std::ofstream &f,const std::string &name,const std::string &value) {
  if(value.length()) f << name << '=' << value << std::endl;
}

inline void write_pair(std::ofstream &f,const std::string &name,const mds_time &value) {
  if(value.defined()) f << name << '=' << value << std::endl;
}

inline void write_pair(std::ofstream &f,const std::string &name,bool value) {
  f << name << '=' << (value?"yes":"no") << std::endl;
}

bool job_local_write_file(const std::string &fname,JobLocalDescription &job_desc) {
  std::ofstream f(fname.c_str(),std::ios::out | std::ios::trunc);
  if(! f.is_open() ) return false; /* can't open file */
  write_pair(f,"jobreport",job_desc.jobreport);
  write_pair(f,"lrms",job_desc.lrms);
  write_pair(f,"queue",job_desc.queue);
  write_pair(f,"localid",job_desc.localid);
  f << "args=";
  if(job_desc.arguments.size()) {
    for(std::list<std::string>::iterator i=job_desc.arguments.begin(); \
        i!=job_desc.arguments.end(); ++i) {
      output_escaped_string(f,*i);
      f << " ";
    };
  };
  f << std::endl;
  write_pair(f,"subject",job_desc.DN);
  write_pair(f,"starttime",job_desc.starttime);
  write_pair(f,"lifetime",job_desc.lifetime);
  write_pair(f,"notify",job_desc.notify);
  write_pair(f,"processtime",job_desc.processtime);
  write_pair(f,"exectime",job_desc.exectime);
  write_pair(f,"rerun",Arc::tostring(job_desc.reruns));
  if(job_desc.downloads>=0) write_pair(f,"downloads",Arc::tostring(job_desc.downloads));
  if(job_desc.uploads>=0) write_pair(f,"uploads",Arc::tostring(job_desc.uploads));
  write_pair(f,"jobname",job_desc.jobname);
  write_pair(f,"gmlog",job_desc.stdlog);
  write_pair(f,"cleanuptime",job_desc.cleanuptime);
  write_pair(f,"delegexpiretime",job_desc.expiretime);
  write_pair(f,"clientname",job_desc.clientname);
  write_pair(f,"clientsoftware",job_desc.clientsoftware);
  write_pair(f,"sessiondir",job_desc.sessiondir);
  write_pair(f,"diskspace",Arc::tostring(job_desc.diskspace));
  write_pair(f,"failedstate",job_desc.failedstate);
  write_pair(f,"fullaccess",job_desc.fullaccess);
  write_pair(f,"credentialserver",job_desc.credentialserver);
  f.close();
  return true;
}

bool job_local_read_file(const JobId &id,const JobUser &user,JobLocalDescription &job_desc) {
  std::string fname = user.ControlDir() + "/job." + id + sfx_local;
  return job_local_read_file(fname,job_desc);
}

bool job_local_read_file(const std::string &fname,JobLocalDescription &job_desc) {
  char buf[4096];
  std::ifstream f(fname.c_str());
  if(! f.is_open() ) return false; /* can't open file */
  std::string name;
  for(;!f.eof();) {
    istream_readline(f,buf,sizeof(buf));
    name.erase();
    int p=input_escaped_string(buf,name,'=');
    if(name.length() == 0) continue;
    if(buf[p] == 0) continue;
    if(name == "lrms") { job_desc.lrms = buf+p; }
    else if(name == "queue") { job_desc.queue = buf+p; }
    else if(name == "localid") { job_desc.localid = buf+p; }
    else if(name == "subject") { job_desc.DN = buf+p; }
    else if(name == "starttime") { job_desc.starttime = buf+p; }
//    else if(name == "UI") { job_desc.UI = buf+p; }
    else if(name == "lifetime") { job_desc.lifetime = buf+p; }
    else if(name == "notify") { job_desc.notify = buf+p; }
    else if(name == "processtime") { job_desc.processtime = buf+p; }
    else if(name == "exectime") { job_desc.exectime = buf+p; }
    else if(name == "jobreport") { job_desc.jobreport = buf+p; }
    else if(name == "jobname") { job_desc.jobname = buf+p; }
    else if(name == "gmlog") { job_desc.stdlog = buf+p; }
    else if(name == "rerun") {
      std::string temp_s(buf+p); int n;
      if(!Arc::stringto(temp_s,n)) { f.close(); return false; };
      job_desc.reruns = n;
    }
    else if(name == "downloads") {
      std::string temp_s(buf+p); int n;
      if(!Arc::stringto(temp_s,n)) { f.close(); return false; };
      job_desc.downloads = n;
    }
    else if(name == "uploads") {
      std::string temp_s(buf+p); int n;
      if(!Arc::stringto(temp_s,n)) { f.close(); return false; };
      job_desc.uploads = n;
    }
    else if(name == "args") {
      job_desc.arguments.clear();
      for(int n=p;buf[n] != 0;) {
        std::string arg;
        n+=input_escaped_string(buf+n,arg);
        job_desc.arguments.push_back(arg);
      };
    }
    else if(name == "cleanuptime") { job_desc.cleanuptime = buf+p; }
    else if(name == "delegexpiretime") { job_desc.expiretime = buf+p; }
    else if(name == "clientname") { job_desc.clientname = buf+p; }
    else if(name == "clientsoftware") { job_desc.clientsoftware = buf+p; }
    else if(name == "sessiondir") { job_desc.sessiondir = buf+p; }
    else if(name == "failedstate") { job_desc.failedstate = buf+p; }
    else if(name == "credentialserver") { job_desc.credentialserver = buf+p; }
    else if(name == "fullaccess") { 
      if(strcasecmp("yes",buf+p) == 0) { job_desc.fullaccess = true; }
      else if(strcasecmp("true",buf+p) == 0) { job_desc.fullaccess = true; }
      else job_desc.fullaccess = false;
    }
    else if(name == "diskspace") {
      std::string temp_s(buf+p);
      unsigned long long int n;
      if(!Arc::stringto(temp_s,n)) { f.close(); return false; };
      job_desc.diskspace = n;
    }
  };
  f.close();
  return true;
}

bool job_local_read_var(const std::string &fname,const std::string &vnam,std::string &value) {
  char buf[1024];
  std::ifstream f(fname.c_str());
  if(! f.is_open() ) return false; /* can't open file */
  std::string name;
  bool found = false;
  for(;!f.eof();) {
    istream_readline(f,buf,sizeof(buf));
    name.erase();
    int p=input_escaped_string(buf,name,'=');
    if(name.length() == 0) continue;
    if(buf[p] == 0) continue;
    if(name == vnam) { value = buf+p; found=true; break; };
  };
  f.close();
  return found;
}

bool job_local_read_lifetime(const JobId &id,const JobUser &user,time_t &lifetime) {
  std::string fname = user.ControlDir() + "/job." + id + sfx_local;
  std::string str;
  if(!job_local_read_var(fname,"lifetime",str)) return false;
  char* str_e;
  unsigned long int t = strtoul(str.c_str(),&str_e,10);
  if((*str_e) != 0) return false;
  lifetime=t;
  return true;
}

bool job_local_read_cleanuptime(const JobId &id,const JobUser &user,time_t &cleanuptime) {
  std::string fname = user.ControlDir() + "/job." + id + sfx_local;
  std::string str;
  if(!job_local_read_var(fname,"cleanuptime",str)) return false;
  mds_time cupt;
  cupt = str;
  cleanuptime=(time_t)cupt;
  return true;
}

bool job_local_read_notify(const JobId &id,const JobUser &user,std::string &notify) {
  std::string fname = user.ControlDir() + "/job." + id + sfx_local;
  if(!job_local_read_var(fname,"notify",notify)) return false;
  return true;
}

bool job_local_read_string(const std::string &fname,unsigned int num,std::string &str) {
  std::ifstream f(fname.c_str());
  if(! f.is_open() ) return false; /* can't open file */
  for(;num;num--) {
    f.ignore(INT_MAX,'\n'); 
  };
  if(f.eof()) { f.close(); return false; };
  char buf[256];
  f.get(buf,255,'\n'); if(!buf[0]) { f.close(); return false; };
  str=buf; 
  f.close();
  return true;
}

/* job.ID.input functions */

bool job_input_write_file(const JobDescription &desc,JobUser &user,std::list<FileData> &files) {
  std::string fname = user.ControlDir() + "/job." + desc.get_id() + ".input";
  return job_Xput_write_file(fname,files) & fix_file_owner(fname,desc,user) & fix_file_permissions(fname);
}

bool job_input_read_file(const JobId &id,JobUser &user,std::list<FileData> &files) {
  std::string fname = user.ControlDir() + "/job." + id + ".input";
  return job_Xput_read_file(fname,files);
}

/* job.ID.output functions */
/*
bool job_cache_write_file(const JobDescription &desc,JobUser &user,std::list<FileData> &files) {
  std::string fname = user.ControlDir() + "/job." + desc.get_id() + ".cache";
  return job_Xput_write_file(fname,files) & fix_file_owner(fname,desc,user) & fix_file_permissions(fname);
}
*/

bool job_output_write_file(const JobDescription &desc,JobUser &user,std::list<FileData> &files) {
  std::string fname = user.ControlDir() + "/job." + desc.get_id() + ".output";
  return job_Xput_write_file(fname,files) & fix_file_owner(fname,desc,user) & fix_file_permissions(fname);
}

/*
bool job_cache_read_file(const JobId &id,JobUser &user,std::list<FileData> &files) {
  std::string fname = user.ControlDir() + "/job." + id + ".cache";
  return job_Xput_read_file(fname,files);
}
*/

bool job_output_read_file(const JobId &id,JobUser &user,std::list<FileData> &files) {
  std::string fname = user.ControlDir() + "/job." + id + ".output";
  return job_Xput_read_file(fname,files);
}

/* common finctions */

bool job_Xput_write_file(const std::string &fname,std::list<FileData> &files) {
  std::ofstream f(fname.c_str(),std::ios::out | std::ios::trunc);
  if(! f.is_open() ) return false; /* can't open file */
  for(FileData::iterator i=files.begin();i!=files.end(); ++i) { 
    f << (*i) << std::endl;
  };
  f.close();
  return true;
}

bool job_Xput_read_file(std::list<FileData> &files) {
  for(;!std::cin.eof();) {
    FileData fd; std::cin >> fd;
//    if((fd.pfn.length() != 0) && (fd.pfn != "/")) {
    if(fd.pfn.length() != 0) {  /* returns zero length only if empty std::string */
      files.push_back(fd);
    };
  };
  return true;
}

bool job_Xput_read_file(const std::string &fname,std::list<FileData> &files) {
  std::ifstream f(fname.c_str());
  if(! f.is_open() ) return false; /* can't open file */
  for(;!f.eof();) {
    FileData fd; f >> fd;
//    if((fd.pfn.length() != 0) && (fd.pfn != "/")) {
    if(fd.pfn.length() != 0) {  /* returns zero length only if empty std::string */
      files.push_back(fd);
    };
  };
  f.close();
  return true;
}

bool job_clean_finished(const JobId &id,JobUser &user) {
  std::string fname;
  fname = user.ControlDir()+"/job."+id+".proxy.tmp"; remove(fname.c_str());
  fname = user.ControlDir()+"/job."+id+".lrms_done"; remove(fname.c_str());
  fname = user.ControlDir()+"/job."+id+".grami"; remove(fname.c_str());
  fname = user.ControlDir()+"/job."+id+".grami_log"; remove(fname.c_str());
  return true;
}

bool job_clean_deleted(const JobDescription &desc,JobUser &user) {
  std::string id = desc.get_id();
  job_clean_finished(id,user);
  std::string fname;
  fname = user.ControlDir()+"/job."+id+".proxy"; remove(fname.c_str());
  fname = user.ControlDir()+"/job."+id+sfx_restart; remove(fname.c_str());
  fname = user.ControlDir()+"/job."+id+sfx_errors; remove(fname.c_str());
  fname = user.ControlDir()+"/job."+id+sfx_cancel; remove(fname.c_str());
  fname = user.ControlDir()+"/job."+id+sfx_clean;  remove(fname.c_str());
  fname = user.ControlDir()+"/job."+id+".output"; remove(fname.c_str());
  fname = user.ControlDir()+"/job."+id+".input"; remove(fname.c_str());
  fname = user.SessionRoot()+"/"+id+sfx_lrmsoutput; remove(fname.c_str());
  /* remove session directory */
  std::list<FileData> flist;
  std::string dname = user.SessionRoot()+"/"+id;
  if(user.StrictSession()) {
    JobUser tmp_user(user.get_uid()==0?desc.get_uid():user.get_uid());
    job_dir_remove_t arg; arg.dname=&dname; arg.flist=&flist;
    return (RunFunction::run(tmp_user,"job_clean_deleted",&job_dir_remove_callback,&arg,10) == 0);
  } else {
    delete_all_files(dname,flist,true);
    remove(dname.c_str());
  };
  return true;
}

bool job_clean_final(const JobDescription &desc,JobUser &user) {
  std::string id = desc.get_id();
  job_clean_finished(id,user);
  job_clean_deleted(desc,user);
  std::string fname;
  fname = user.ControlDir()+"/job."+id+sfx_local;  remove(fname.c_str());
  fname = user.ControlDir()+"/job."+id+sfx_failed; remove(fname.c_str());
  job_diagnostics_mark_remove(desc,user);
  job_lrmsoutput_mark_remove(desc,user);
  fname = user.ControlDir()+"/job."+id+sfx_status; remove(fname.c_str());
  fname = user.ControlDir()+"/job."+id+sfx_rsl;    remove(fname.c_str());
  return true;
}


