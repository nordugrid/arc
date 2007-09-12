#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
/* 
  Filename: users.cc
  keeps list of users
*/

//@ #include "../std.h"
//@ 
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <pwd.h>
#define olog std::cerr
#define inttostring(N) Arc::tostring(N)
#include <arc/StringConv.h>
//@ 

#include <string>
#include <list>

#include "../conf/conf.h"
#include "../run/run_parallel.h"
//@ #include "../misc/inttostring.h"
//@ #include "../misc/log_time.h"
#include "../misc/escaped.h"
#include "../jobs/states.h"
#include "../conf/environment.h"
#include "users.h"


JobUser::JobUser(void) {
  control_dir="";
  unix_name=""; home=""; uid=0; gid=0;
  cache_dir=""; cache_data_dir=""; cache_link_dir=""; cache_max=0; cache_min=0;
  valid=false; jobs=NULL;
  session_root="";
  keep_finished=DEFAULT_KEEP_FINISHED;
  keep_deleted=DEFAULT_KEEP_DELETED;
  cred_plugin=NULL;
  strict_session=false;
  sharelevel=jobinfo_share_private;
};

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
  if(dir.length() == 0) { session_root=home + "/.jobs"; }
  else { session_root=dir; };
}

void JobUser::SetCacheDir(const std::string &dir,const std::string &data_dir,bool priv) {
  SetCacheDir(dir,data_dir,"",priv);
}

void JobUser::SetCacheSize(long long int cache_max_,long long int cache_min_) {
  if((cache_min_==0) || (cache_max_==0)) cache_min_=cache_max_;
  cache_max=cache_max_;
  cache_min=cache_min_;
}

void JobUser::SetCacheDir(const std::string &dir,const std::string &data_dir,const std::string &link_dir,bool priv) {
  cache_dir=dir;
  if(data_dir == "") { cache_data_dir=dir; } else { cache_data_dir=data_dir; };
  cache_link_dir=link_dir;
  private_cache=priv;
}

bool JobUser::CreateDirectories(void) {
  bool res = true;
  if(control_dir.length() != 0) {
    if(mkdir(control_dir.c_str(),S_IRWXU) != 0) {
      if(errno != EEXIST) res=false;
    } else {
      (void)chown(control_dir.c_str(),uid,gid);
    };
    if(mkdir((control_dir+"/logs").c_str(),S_IRWXU) != 0) {
      if(errno != EEXIST) res=false;
    } else {
      (void)chown((control_dir+"/logs").c_str(),uid,gid);
    };
  };
  if(session_root.length() != 0) {
    if(mkdir(session_root.c_str(),S_IRWXU) != 0) {
      if(errno != EEXIST) res=false;
    } else {
      (void)chown(session_root.c_str(),uid,gid);
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
      case 'u': to_put=inttostring(get_uid()); break;
      case 'g': to_put=inttostring(get_gid()); break;
      case 'W': to_put=nordugrid_loc; break;
      case 'G': to_put=globus_loc; break;
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

JobUser::JobUser(uid_t uid_,RunPlugin* cred) {
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
  SetCacheDir("","");
  SetCacheSize(0);
  keep_finished=DEFAULT_KEEP_FINISHED;
  keep_deleted=DEFAULT_KEEP_DELETED;
  strict_session=false;
  sharelevel=jobinfo_share_private;
}

JobUser::JobUser(const std::string &u_name,RunPlugin* cred) {
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
  SetCacheDir("","");
  SetCacheSize(0);
  jobs=NULL;
  keep_finished=DEFAULT_KEEP_FINISHED;
  keep_deleted=DEFAULT_KEEP_DELETED;
  strict_session=false;
  sharelevel=jobinfo_share_private;
};

JobUser::JobUser(const JobUser &user) {
  uid=user.uid; gid=user.gid;
  unix_name=user.unix_name;  
  control_dir=user.control_dir;
  home=user.home;
  jobs=user.jobs;
  session_root=user.session_root;
  default_lrms=user.default_lrms;
  default_queue=user.default_queue;
  valid=user.valid;
  keep_finished=user.keep_finished;
  keep_deleted=user.keep_deleted;
  cache_dir=user.cache_dir;
  cache_data_dir=user.cache_data_dir;
  cache_link_dir=user.cache_link_dir;
  private_cache=user.private_cache;
  cache_max=user.cache_max;
  cache_min=user.cache_min;
  cred_plugin=user.cred_plugin;
  strict_session=user.strict_session;
  sharelevel=user.sharelevel;
};

JobUser::~JobUser(void) { 
};

JobUsers::JobUsers(void) {
};

JobUsers::~JobUsers(void) {
};

JobUsers::iterator JobUsers::AddUser(const std::string &unix_name,RunPlugin* cred_plugin,const std::string &control_dir, const std::string &session_root) {
  JobUser user(unix_name,cred_plugin);
  user.SetControlDir(control_dir);
  user.SetSessionRoot(session_root);
  if(user.is_valid()) { return users.insert(users.end(),user); };
  return users.end();
};

std::string JobUsers::ControlDir(iterator user) {
  if(user == users.end()) return std::string("");
  return (*user).ControlDir();
};

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

/* change effective user - real switch is done only if running as root */
bool JobUser::SwitchUser(bool su) const {
  std::string uid_s = inttostring(uid);
  if(setenv("USER_ID",uid_s.c_str(),1) != 0) if(!su) return false;
  if(setenv("USER_NAME",unix_name.c_str(),1) != 0) if(!su) return false;
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
      if(proc->get_exit_code() == -1) proc->kill();
      RunParallel::release(proc);
      proc=NULL;
    };
#endif
}

#ifndef NO_GLOBUS_CODE
bool JobUserHelper::run(JobUser &user) {
    if(proc != NULL) {
      if(proc->get_exit_code() == -1) {
        return true; /* it is already/still running */
      };
      RunParallel::release(proc);
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
    //olog<<"Starting helper process ("<<user.UnixName()<<"): "<<
    //                                                  args[0]<<std::endl;
    std::string helper_id="helper."+user.UnixName();
    bool started=RunParallel::run(user,helper_id.c_str(),args,&proc);
    for(n=0;n<99;n++) {
      if(args[n] == NULL) break;
      free(args[n]);
    };
    if(started) return true;
    olog<<"Helper process start failed ("<<user.UnixName()<<"): "<<
                                                       command<<std::endl;
    /* start failed */
    /* doing nothing - maybe in the future */
    return false;
}
#endif
