#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
/* 
  Filename: users.cc
  keeps list of users
*/

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <pwd.h>
#include <grp.h>
#include <cstdlib>
#include <arc/StringConv.h>
#include <arc/Logger.h>
#include <arc/Utils.h>

#include <string>
#include <list>

#include "../conf/conf.h"
#include "../run/run_parallel.h"
#include "../misc/escaped.h"
#include "../jobs/states.h"
#include "../conf/environment.h"
#include "users.h"

static Arc::Logger& logger = Arc::Logger::getRootLogger();
static std::string empty_string("");

JobUser::JobUser(const GMEnvironment& env):gm_env(env) {
  control_dir="";
  unix_name=""; home=""; uid=0; gid=0;
  valid=false; jobs=NULL;
  keep_finished=DEFAULT_KEEP_FINISHED;
  keep_deleted=DEFAULT_KEEP_DELETED;
  cred_plugin=NULL;
  strict_session=false;
  share_uid=0;
}

void JobUser::SetLRMS(const std::string &lrms_name,const std::string &queue_name) {
  default_lrms=lrms_name;
  default_queue=queue_name;
}

void JobUser::SetControlDir(const std::string &dir) {
  if(dir.length() == 0) {
    control_dir=home + "/.jobstatus";
  }
  else { control_dir=dir; };
}

void JobUser::SetSessionRoot(const std::string &dir) {
  session_roots.clear();
  if(dir.length() == 0 || dir == "*") { session_roots.push_back(home + "/.jobs"); }
  else { session_roots.push_back(dir); };
}

void JobUser::SetSessionRoot(const std::vector<std::string> &dirs) {
  session_roots.clear();
  if (dirs.empty()) {
    std::string dir;
    SetSessionRoot(dir);
  } else {
    for (std::vector<std::string>::const_iterator i = dirs.begin(); i != dirs.end(); i++) {
      if (*i == "*") session_roots.push_back(home + "/.jobs");
      else session_roots.push_back(*i);
    }
  }
}

const std::string & JobUser::SessionRoot(std::string job_id) const {
  if (session_roots.size() == 0) return empty_string;
  if (session_roots.size() == 1 || job_id.empty()) return session_roots[0];
  // search for this jobid's session dir
  struct stat st;
  for (std::vector<std::string>::const_iterator i = session_roots.begin(); i != session_roots.end(); i++) {
    std::string sessiondir(*i + '/' + job_id);
    if (stat(sessiondir.c_str(), &st) == 0 && S_ISDIR(st.st_mode))
      return *i;
  }
  return empty_string; // not found
}

void JobUser::SetCacheParams(CacheConfig params) {
  std::vector<std::string> cache_dirs = params.getCacheDirs();
  for (std::vector<std::string>::iterator i = cache_dirs.begin(); i != cache_dirs.end(); i++) {
    substitute(*i);
  }
  params.setCacheDirs(cache_dirs);
  std::vector<std::string> drain_cache_dirs = params.getDrainingCacheDirs();
  for (std::vector<std::string>::iterator i = drain_cache_dirs.begin(); i != drain_cache_dirs.end(); i++) {
    substitute(*i);
  }
  params.setDrainingCacheDirs(drain_cache_dirs);
  cache_params = params;
}

bool JobUser::CreateDirectories(void) {
  bool res = true;
  if(control_dir.length() != 0) {
    if(mkdir(control_dir.c_str(),S_IRWXU) != 0) {
      if(errno != EEXIST) res=false;
    } else {
      (chown(control_dir.c_str(),uid,gid) != 0);
      if(uid == 0) {
        chmod(control_dir.c_str(),S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
      } else {
        chmod(control_dir.c_str(),S_IRUSR | S_IWUSR | S_IXUSR);
      };
    };
    if(mkdir((control_dir+"/logs").c_str(),S_IRWXU) != 0) {
      if(errno != EEXIST) res=false;
    } else {
      (chown((control_dir+"/logs").c_str(),uid,gid) != 0);
    };
    if(mkdir((control_dir+"/accepting").c_str(),S_IRWXU) != 0) {
      if(errno != EEXIST) res=false;
    } else {
      (chown((control_dir+"/accepting").c_str(),uid,gid) != 0);
    };
    if(mkdir((control_dir+"/processing").c_str(),S_IRWXU) != 0) {
      if(errno != EEXIST) res=false;
    } else {
      (chown((control_dir+"/processing").c_str(),uid,gid) != 0);
    };
    if(mkdir((control_dir+"/finished").c_str(),S_IRWXU) != 0) {
      if(errno != EEXIST) res=false;
    } else {
      (chown((control_dir+"/finished").c_str(),uid,gid) != 0);
    };
  };
  if(session_roots.size() != 0) {
    for(std::vector<std::string>::iterator i = session_roots.begin(); i != session_roots.end(); i++) {
      if(mkdir(i->c_str(),S_IRWXU) != 0) {
        if(errno != EEXIST) res=false;
      } else {
        (chown(i->c_str(),uid,gid) != 0);
        if(uid == 0) {
          chmod(i->c_str(),S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
        } else {
          chmod(i->c_str(),S_IRUSR | S_IWUSR | S_IXUSR);
        };
      };
    };
  };
  return res;
}

/*
  %R - session root
  %r - list of session roots
  %C - control dir
  %c - list of control dirs
  %U - username
  %u - userid
  %g - groupid
  %H - home dir
  %Q - default queue
  %L - default lrms
  %W - installation path
  %G - globus path
  %F - configuration file
*/

bool JobUser::substitute(std::string& param) const {
  std::string::size_type curpos=0;
  for(;;) {
    if(curpos >= param.length()) break;
    std::string::size_type pos = param.find('%',curpos);
    if(pos == std::string::npos) break;
    pos++; if(pos>=param.length()) break;
    if(param[pos] == '%') { curpos=pos+1; continue; };
    std::string to_put;
    switch(param[pos]) {
      case 'R': to_put=SessionRoot(); break;
      case 'C': to_put=ControlDir(); break;
      case 'U': to_put=UnixName(); break;
      case 'H': to_put=Home(); break;
      case 'Q': to_put=DefaultQueue(); break;
      case 'L': to_put=DefaultLRMS(); break;
      case 'u': to_put=Arc::tostring(get_uid()); break;
      case 'g': to_put=Arc::tostring(get_gid()); break;
      case 'W': to_put=gm_env.nordugrid_loc(); break;
      case 'F': to_put=gm_env.nordugrid_config_loc(); break;
      case 'G': 
        logger.msg(Arc::ERROR,"Globus location variable substitution is not supported anymore. Please specify path directly.");
        break;
      default: to_put=param.substr(pos-1,2);
    };
    curpos=pos+1+(to_put.length() - 2);
    param.replace(pos-1,2,to_put);
  };
  return true;
}

bool JobUsers::substitute(std::string& param) const {
  std::string session_roots = "";
  std::string control_dirs = "";
  for(JobUsers::const_iterator i = begin();i!=end();++i) {
    std::string tmp_s;
    tmp_s = i->SessionRoot();
    make_escaped_string(tmp_s);
    tmp_s=tmp_s+" ";
    if(session_roots.find(tmp_s) == std::string::npos) session_roots+=tmp_s;
    tmp_s = i->ControlDir();
    make_escaped_string(tmp_s);
    tmp_s=tmp_s+" ";
    if(control_dirs.find(tmp_s) == std::string::npos) control_dirs+=tmp_s;
  };
  std::string::size_type curpos=0;
  for(;;) {
    if(curpos >= param.length()) break;
    std::string::size_type pos = param.find('%',curpos);
    if(pos == std::string::npos) break;
    pos++; if(pos>=param.length()) break;
    if(param[pos] == '%') { curpos=pos+1; continue; };
    std::string to_put;
    switch(param[pos]) {
      case 'r': to_put=session_roots; break;
      case 'c': to_put=control_dirs; break;
      default: to_put=param.substr(pos-1,2);
    };
    curpos=pos+1+(to_put.length() - 2);
    param.replace(pos-1,2,to_put);
  };
  return true;
}

JobUser::JobUser(const GMEnvironment& env,uid_t uid_,RunPlugin* cred):gm_env(env) {
  struct passwd pw_;
  struct passwd *pw;
  char buf[BUFSIZ];
  uid=uid_;
  valid=false;
  cred_plugin=cred;
  /* resolve name */
  if(uid_ == 0) {
    unix_name="";  
    gid=0;
    home="/tmp";
    valid=true;
  }
  else {
    getpwuid_r(uid_,&pw_,buf,BUFSIZ,&pw);
    if(pw != NULL) {
      unix_name=pw->pw_name;  
      gid=pw->pw_gid;
      home=pw->pw_dir;
      valid=true;
    };
  };
  jobs=NULL;
  SetControlDir("");
  SetSessionRoot("");
  SetLRMS("","");
  keep_finished=DEFAULT_KEEP_FINISHED;
  keep_deleted=DEFAULT_KEEP_DELETED;
  strict_session=false;
  share_uid=0;
}

JobUser::JobUser(const GMEnvironment& env,const std::string &u_name,RunPlugin* cred):gm_env(env) {
  struct passwd pw_;
  struct passwd *pw;
  char buf[BUFSIZ];
  unix_name=u_name;  
  cred_plugin=cred;
  valid=false;
  /* resolve name */
  if(u_name.length() == 0) {
    uid=0;  
    gid=0;
    home="/tmp";
    valid=true;
  }
  else {
    getpwnam_r(u_name.c_str(),&pw_,buf,BUFSIZ,&pw);
    if(pw != NULL) {
      uid=pw->pw_uid;
      gid=pw->pw_gid;
      home=pw->pw_dir;
      valid=true;
    };
  };
  SetControlDir("");
  SetSessionRoot("");
  SetLRMS("","");
  jobs=NULL;
  keep_finished=DEFAULT_KEEP_FINISHED;
  keep_deleted=DEFAULT_KEEP_DELETED;
  strict_session=false;
  share_uid=0;
}

JobUser::JobUser(const JobUser &user):gm_env(user.gm_env) {
  uid=user.uid; gid=user.gid;
  unix_name=user.unix_name;  
  control_dir=user.control_dir;
  home=user.home;
  jobs=user.jobs;
  session_roots=user.session_roots;
  default_lrms=user.default_lrms;
  default_queue=user.default_queue;
  valid=user.valid;
  keep_finished=user.keep_finished;
  keep_deleted=user.keep_deleted;
  cache_params=user.cache_params;
  cred_plugin=user.cred_plugin;
  strict_session=user.strict_session;
  share_uid=user.share_uid;
  share_gids=user.share_gids;
}

JobUser::~JobUser(void) { 
}

void JobUser::SetShareID(uid_t suid) {
  share_uid=suid;
  share_gids.clear();
  if(suid <= 0) return;
  struct passwd pwd_buf;
  struct passwd* pwd = NULL;
#ifdef _SC_GETPW_R_SIZE_MAX
  int buflen = sysconf(_SC_GETPW_R_SIZE_MAX);
  if (buflen <= 0) buflen = 16384;
#else
  int buflen = 16384;
#endif
  char* buf = (char*)malloc(buflen);
  if(!buf) return;
  if(getpwuid_r(suid, &pwd_buf, buf, buflen, &pwd) == 0) {
    if(pwd) {
#ifdef HAVE_GETGROUPLIST
#ifdef _MACOSX
      int groups[100];
#else
      gid_t groups[100];
#endif
      int ngroups = 100;
      if(getgrouplist(pwd->pw_name, pwd->pw_gid, groups, &ngroups) >= 0) {
        for(int n = 0; n<ngroups;++n) {
          share_gids.push_back((gid_t)(groups[n]));
        };
      };
#endif
      share_gids.push_back(pwd->pw_gid);
    };
  };
  free(buf);
}

bool JobUser::match_share_gid(gid_t sgid) const {
  for(std::list<gid_t>::const_iterator i = share_gids.begin();
                       i != share_gids.end();++i) {
    if(sgid == *i) return true;
  };
  return false;
}

JobUsers::JobUsers(GMEnvironment& env):gm_env(env) {
}

JobUsers::~JobUsers(void) {
}

JobUsers::iterator JobUsers::AddUser(const std::string &unix_name,RunPlugin* cred_plugin,const std::string &control_dir, const std::vector<std::string> *session_roots) {
  JobUser user(gm_env,unix_name,cred_plugin);
  user.SetControlDir(control_dir);
  if(session_roots) user.SetSessionRoot(*session_roots);
  if(user.is_valid()) { return users.insert(users.end(),user); };
  return users.end();
}

std::string JobUsers::ControlDir(iterator user) {
  if(user == users.end()) return std::string("");
  return (*user).ControlDir();
}

JobUsers::iterator JobUsers::find(const std::string user) {
  iterator i;
  for(i=users.begin();i!=users.end();++i) {
    if((*i) == user) break;
  };
  return i;
}

std::string JobUsers::ControlDir(const std::string user) {
  for(iterator i=users.begin();i!=users.end();++i) {
    if((*i) == user) return (*i).ControlDir();
  };
  return std::string("");
}

#ifndef NO_GLOBUS_CODE

/* Change effective user - real switch is done only if running as root.
   This method is caled in single-threaded environment and in 
   post-multi-threaded right after fork. So it must avoid 
   using anything that could internally inlcude calls to thread 
   functions. */
bool JobUser::SwitchUser(bool su) const {
  //std::string uid_s = Arc::tostring(uid);
  //if(!Arc::SetEnv("USER_ID",uid_s)) if(!su) return false;
  //if(!Arc::SetEnv("USER_NAME",unix_name)) if(!su) return false;

  static char uid_s[64];
  snprintf(uid_s,63,"%llu",(long long unsigned int)uid);
  uid_s[63]=0;
#ifdef HAVE_SETENV
  if(setenv("USER_ID",uid_s,1) != 0) if(!su) return false;
  if(setenv("USER_NAME",unix_name.c_str(),1) != 0) if(!su) return false;
#else
  static char user_id_s[64];
  static char user_name_s[64];
  strncpy(user_id_s,"USER_ID",63); user_id_s[63]=0;
  strncat(user_id_s,uid_s,63-strlen(user_id_s)); user_id_s[63]=0;
  strncpy(user_name_s,"USER_ID",63); user_id_s[63]=0;
  strncat(user_name_s,unix_name.c_str(),63-strlen(user_name_s)); user_name_s[63]=0;
  if(putenv(user_id_s) != 0) if(!su) return false;
  if(putenv(user_name_s) != 0) if(!su) return false;
#endif

  /* set proper umask */
  umask(0177);
  if(!su) return true;
  uid_t cuid;
  if(((cuid=getuid()) != 0) && (uid != 0)) {
    if(cuid != uid) return false;
  };
  if(uid != 0) {
    setgid(gid); /* this is not an error if group failed, not a big deal */
    if(setuid(uid) != 0) return false;
  };
  return true;
}

bool JobUser::run_helpers(void) {
//  if(unix_name.length() == 0) { /* special users can not run helpers */
//    return true;  
//  };
  bool started = true;
  for(std::list<JobUserHelper>::iterator i=helpers.begin();i!=helpers.end();++i) {
    started &= i->run(*this);
  };
  return started;
}

bool JobUsers::run_helpers(void) {
  for(iterator i=users.begin();i!=users.end();++i) {
    i->run_helpers();
  };
  return true;
}
#endif // NO_GLOBUS_CODE

JobUserHelper::JobUserHelper(const std::string &cmd) {
  command=cmd;
  proc=NULL;
}
  
JobUserHelper::~JobUserHelper(void) {
#ifndef NO_GLOBUS_CODE
    if(proc != NULL) {
      delete proc;
      proc=NULL;
    };
#endif
}

#ifndef NO_GLOBUS_CODE
bool JobUserHelper::run(JobUser &user) {
    if(proc != NULL) {
      if(proc->Running()) {
        return true; /* it is already/still running */
      };
      delete proc;
      proc=NULL;
    };
    /* start/restart */
    if(command.length() == 0) return true;  /* has anything to run ? */
    char* args[100]; /* up to 98 arguments should be enough */
    std::string args_s = command;
    std::string arg_s;
    int n;
    for(n=0;n<99;n++) {
      arg_s=config_next_arg(args_s);
      if(arg_s.length() == 0) break;
      args[n]=strdup(arg_s.c_str());
    };
    args[n]=NULL;
    logger.msg(Arc::VERBOSE,"Starting helper process (%s): %s",
               user.UnixName().c_str(),command.c_str());
    std::string helper_id="helper."+user.UnixName();
    bool started=RunParallel::run(user,helper_id.c_str(),args,&proc);
    for(n=0;n<99;n++) {
      if(args[n] == NULL) break;
      free(args[n]);
    };
    if(started) return true;
    if(proc && (*proc)) return true;
    if(proc) { delete proc; proc=NULL; };
    logger.msg(Arc::ERROR,"Helper process start failed (%s): %s",
               user.UnixName().c_str(),command.c_str());
    /* start failed */
    /* doing nothing - maybe in the future */
    return false;
}
#endif

