#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_SETFSUID
#include <sys/fsuid.h>
#endif

#ifdef HAVE_SSTREAM
#include <sstream>
#else
#include <strstream>
#endif

#define GRIDFTP_PLUGIN

#include <fcntl.h>
#include <dirent.h>
#include <errno.h>

#include <arc/StringConv.h>
#include <arc/FileUtils.h>
#include <arc/DateTime.h>
#include <arc/Logger.h>
#include <arc/credential/Credential.h>

#include "../conf/conf.h"
#include "../jobs/job.h"
#include "../jobs/commfifo.h"
#include "../jobs/plugins.h"
#include "../conf/environment.h"
#include "../conf/conf_pre.h"
#include "../files/info_types.h"
#include "../files/info_files.h"
#include "../jobs/job_request.h"
#include "../misc/escaped.h"
#include "../misc/proxy.h"
#include "../run/run_parallel.h"
#include "../log/job_log.h"
#include "../../../gridftpd/userspec.h"
#include "../../../gridftpd/names.h"
#include "../../../gridftpd/misc.h"
#include "../../../gridftpd/fileplugin/fileplugin.h"
#include "../../../gridftpd/auth/gacl_auth.h"
#include "../../../gridftpd/auth/permission_gacl.h"

#include "jobplugin.h"

static Arc::Logger& logger = Arc::Logger::getRootLogger();

typedef struct {
  const JobUser* user;
  const std::string* job;
  const char* reason;
} job_subst_t;

#define IS_ALLOWED_READ  1
#define IS_ALLOWED_WRITE 2
#define IS_ALLOWED_LIST  4
#define IS_ALLOWED_RW    3
#define IS_ALLOWED_ALL   7

#ifdef HAVE_SETFSUID

// Non-portable solution. Needed as long as Linux
// does not support proper setuid in threads
#define SET_USER_UID {  ::setfsuid(user->get_uid()); ::setfsgid(user->get_gid()); }
#define RESET_USER_UID { ::setfsuid(getuid()); ::setfsgid(getgid()); }

#else

// Not sure how this will affect other threads. Most probably 
// not in a best way. Anyway this option is not for linux.
#define SET_USER_UID { setegid(user->get_gid()); seteuid(user->get_uid()); }
#define RESET_USER_UID { seteuid(getuid()); setegid(getgid()); }

#endif

static void job_subst(std::string& str,void* arg) {
  job_subst_t* subs = (job_subst_t*)arg;
  if(subs->job) for(std::string::size_type p = 0;;) {
    p=str.find('%',p);
    if(p==std::string::npos) break;
    if(str[p+1]=='I') {
      str.replace(p,2,subs->job->c_str());
      p+=subs->job->length();
    } else if(str[p+1]=='S') {
      str.replace(p,2,"UNKNOWN"); // WRONG
      p+=7;
    } else if(str[p+1]=='O') {
      str.replace(p,2,subs->reason);
      p+=strlen(subs->reason);
    } else {
      p+=2;
    };
  };
  if(subs->user) subs->user->substitute(str);
}

// run external plugin to acquire non-unix local credentials
// U - user, J - job, O - reason
#define ApplyLocalCred(U,J,O) {                              \
  if(cred_plugin && (*cred_plugin)) {                        \
    job_subst_t subst_arg;                                   \
    subst_arg.user=U;                                        \
    subst_arg.job=J;                                         \
    subst_arg.reason=O;                                      \
    if(!cred_plugin->run(job_subst,&subst_arg)) {            \
      logger.msg(Arc::ERROR, "Failed to run plugin");                \
      return 1;                                              \
    };                                                       \
    if(cred_plugin->result() != 0) {                         \
      logger.msg(Arc::ERROR, "Plugin failed: %s", cred_plugin->result());  \
      return 1;                                              \
    };                                                       \
  };                                                         \
}


JobPlugin::JobPlugin(std::istream &cfile,userspec_t &user_s):user_a(user_s.user),job_map(user_s.user) {
  initialized=true;
  rsl_opened=false;
  job_rsl_max_size = DEFAULT_JOB_RSL_MAX_SIZE;
  user=NULL;
  direct_fs=NULL;
  proxy_fname="";
  std::string configfile("");
  cont_plugins=NULL;
  cred_plugin=NULL;
  readonly=false;
  chosenFilePlugin=NULL;
  srand(time(NULL)); 
  for(;;) {
    std::string rest;
    std::string command=config_read_line(cfile,rest);
    if(command.length() == 0) { break; } /* end of file - should not be here */
    else if(command == "configfile") {
      input_escaped_string(rest.c_str(),configfile); 
    } else if(command == "allownew") {
      std::string value("");
      input_escaped_string(rest.c_str(),value);
      if(strcasecmp(value.c_str(),"no") == 0) { readonly=true; }
      else if(strcasecmp(value.c_str(),"yes") == 0) { readonly=false; };
    } else if(command == "unixmap") {  /* map to local unix user */
      if(!job_map) job_map.mapname(rest.c_str());
    } else if(command == "unixgroup") {  /* map to local unix user */
      if(!job_map) job_map.mapgroup(rest.c_str());
    } else if(command == "unixvo") {  /* map to local unix user */
      if(!job_map) job_map.mapvo(rest.c_str());
    } else if(command == "remotegmdirs") {
      std::string remotedir = config_next_arg(rest);
      if(remotedir.length() == 0) {
        logger.msg(Arc::ERROR, "empty argument to remotedirs");
        initialized=false;
      };
      struct gm_dirs_ dirs;
      dirs.control_dir = remotedir;
      remotedir = config_next_arg(rest);
      if(remotedir.length() == 0) {
        logger.msg(Arc::ERROR, "bad arguments to remotedirs");
        initialized=false;
      };
      dirs.session_dir = remotedir;
      gm_dirs_info.push_back(dirs);
      std::string drain = config_next_arg(rest);
      if (drain.empty() || drain != "drain")
        gm_dirs_non_draining.push_back(dirs); 
    } else if(command == "maxjobdesc") {
      if(rest.empty()) {
        job_rsl_max_size = 0;
      } else if(sscanf(rest.c_str(),"%u",&job_rsl_max_size) != 1) {
        logger.msg(Arc::ERROR, "Wrong number in maxjobdesc");
        initialized=false;
      };
    } else if(command == "end") {
      break; /* end of section */
    } else {
      logger.msg(Arc::WARNING, "Unsupported configuration command: %s", command);
    };
  };
  JobLog job_log;
  JobsListConfig jobs_cfg;
  GMEnvironment env(job_log,jobs_cfg);
  if(!env)
    initialized = false;
  else {
    if(configfile.length()) env.nordugrid_config_loc(configfile);
    const char* uname = user_s.get_uname();
    if((bool)job_map) uname=job_map.unix_name();
    user=new JobUser(env, uname);
    if(!user->is_valid()) { initialized=false; }
    else {
      /* read configuration */
      if(env.nordugrid_config_loc().length() != 0) {
        std::vector<std::string> session_roots;
        std::string control_dir;
        std::string default_lrms;
        std::string default_queue;
        cont_plugins = new ContinuationPlugins;
        cred_plugin = new RunPlugin;
        std::string allowsubmit;
        bool strict_session;
        if(!configure_user_dirs(uname,
          control_dir,session_roots,
          session_dirs_non_draining,
          default_lrms,default_queue,avail_queues,*cont_plugins,*cred_plugin,
          allowsubmit,strict_session, env)) {
          logger.msg(Arc::ERROR, "Failed processing grid-manager configuration");
          initialized=false;
        } else if (gm_dirs_info.size() > 0 && session_roots.size() > 1) {
          logger.msg(Arc::ERROR, "Cannot use multiple session dirs and remotegmdirs at the same time");
          initialized=false;
        } else {                  
          if(default_queue.empty() && (avail_queues.size() == 1)) {
            default_queue=*(avail_queues.begin());
          };
          user->SetControlDir(control_dir);
          user->SetSessionRoot(session_roots);
          user->SetLRMS(default_lrms,default_queue);
          user->SetStrictSession(strict_session);
          for(;allowsubmit.length();) {
            std::string group = config_next_arg(allowsubmit);
            if(user_a.check_group(group)) { readonly=false; break; };
          };
          if(readonly) logger.msg(Arc::ERROR, "This user is denied to submit new jobs.");
          struct gm_dirs_ dirs;
          dirs.control_dir = user->ControlDir();
          dirs.session_dir = user->SessionRoot();
          gm_dirs_info.push_back(dirs);
          gm_dirs_non_draining.push_back(dirs); // can't drain main control dir
          session_dirs = user->SessionRoots();
          /* link to the class for direct file access - creating one object per set of GM dirs */
          // choose whether to use multiple session dirs or remote GM dirs
          if (session_dirs.size() > 1) {
            for (std::vector<std::string>::iterator i = session_dirs.begin(); i != session_dirs.end(); i++) {
              std::string direct_config = "";
              direct_config += "mount "+(*i)+"\n";
              direct_config+="dir / nouser read cd dirlist delete append overwrite";
              direct_config+=" create "+
                  Arc::tostring(user->get_uid())+":"+Arc::tostring(user->get_gid())+
                  " 600:600";
              direct_config+=" mkdir "+
                  Arc::tostring(user->get_uid())+":"+Arc::tostring(user->get_gid())+
                  " 700:700\n";
              direct_config+="end\n";
  #ifdef HAVE_SSTREAM
              std::stringstream fake_cfile(direct_config);
  #else
              std::strstream fake_cfile;
              fake_cfile<<direct_config;
  #endif
              DirectFilePlugin * direct_fs = new DirectFilePlugin(fake_cfile,user_s);
              file_plugins.push_back(direct_fs);
            }
          }
          else {
            for (std::vector<struct gm_dirs_>::iterator i = gm_dirs_info.begin(); i != gm_dirs_info.end(); i++) {
              std::string direct_config = "";
              direct_config += "mount "+(*i).session_dir+"\n";
              direct_config+="dir / nouser read cd dirlist delete append overwrite";
              direct_config+=" create "+
                  Arc::tostring(user->get_uid())+":"+Arc::tostring(user->get_gid())+
                  " 600:600";
              direct_config+=" mkdir "+
                  Arc::tostring(user->get_uid())+":"+Arc::tostring(user->get_gid())+
                  " 700:700\n";
              direct_config+="end\n";
  #ifdef HAVE_SSTREAM
              std::stringstream fake_cfile(direct_config);
  #else
              std::strstream fake_cfile;
              fake_cfile<<direct_config;
  #endif
              DirectFilePlugin * direct_fs = new DirectFilePlugin(fake_cfile,user_s);
              file_plugins.push_back(direct_fs);
            }
          }
          if((bool)job_map) {
            logger.msg(Arc::INFO, "Job submission user: %s (%i:%i)", uname, user->get_uid(), user->get_gid());
          };
        };
      } else {
        initialized=false;
      };
    };
  };
  if(!initialized) if(user) { delete user; user=NULL; };
  if((!user_a.is_proxy()) ||
     (user_a.proxy() == NULL) || (user_a.proxy()[0] == 0)) {
    logger.msg(Arc::WARNING, "No delegated credentials were passed");
  } else {
    proxy_fname=user_a.proxy();
  };
  subject=user_a.DN();
  port=user_s.get_port();
  memcpy(host,user_s.get_host(),sizeof(host));
  job_id="";
#if 0
  if(user) {
    if(user->StrictSession()) {
      // Changing unix user. That means control directory must be
      // writable for every serviced user.
      user->SwitchUser(true);
    };
  };
#endif
}

JobPlugin::~JobPlugin(void) {
  delete_job_id();
  if(proxy_fname.length() != 0) { remove(proxy_fname.c_str()); };
  if(cont_plugins) delete cont_plugins;
  if(cred_plugin) delete cred_plugin;
  for (unsigned int i = 0; i < file_plugins.size(); i++) {
    if (file_plugins.at(i)) delete file_plugins.at(i);
  }
};
 

int JobPlugin::makedir(std::string &dname) {
  if(!initialized) return 1;
  std::string id;
  bool spec_dir;
  if((dname == "new") || (dname == "info")) return 0;
  if(is_allowed(dname.c_str(),true,&spec_dir,&id) & IS_ALLOWED_WRITE) {
    if(spec_dir) {
      error_description="Can't create subdirectory in a special directory.";
      return 1;
    };
    ApplyLocalCred(user,&id,"write");
    DirectFilePlugin * fp = selectFilePlugin(id);
    if((getuid()==0) && (user) && (user->StrictSession())) {
      SET_USER_UID;
      int r=fp->makedir(dname);
      RESET_USER_UID;
      return r;
    };
    return fp->makedir(dname);
  };
  error_description="Not allowed for this job.";
  return 1;
}

int JobPlugin::removefile(std::string &name) {
  if(!initialized) return 1;
  if(name.find('/') == std::string::npos) { /* request to cancel the job */
    if((name == "new") || (name == "info")) {
      error_description="Special directory can't be mangled.";
      return 1;
    };
    if(is_allowed(name.c_str()) & IS_ALLOWED_WRITE) {  /* owner of the job */
      JobId id(name); JobDescription job_desc(id,"");
      std::string controldir = getControlDir(id);
      if (controldir.empty()) {
        error_description="No control information found for this job.";
        return 1;
      }    
      user->SetControlDir(controldir);
      if(job_cancel_mark_put(job_desc,*user)) return 0;
    };
    error_description="Not allowed to cancel this job.";
    return 1;
  };
  const char* logname;
  std::string id;
  bool spec_dir;
  if(is_allowed(name.c_str(),false,&spec_dir,&id,&logname) & IS_ALLOWED_WRITE) {
    if(logname) {
      if((*logname) != 0) return 0; /* pretend status file is deleted */
    };
    if(spec_dir) {
      error_description="Special directory can't be mangled.";
      return 1; /* can delete status directory */
    };
    ApplyLocalCred(user,&id,"write");
    DirectFilePlugin * fp = selectFilePlugin(id);
    if((getuid()==0) && (user) && (user->StrictSession())) {
      SET_USER_UID;
      int r=fp->removefile(name);
      RESET_USER_UID;
      return r;
    };
    return fp->removefile(name);
  };
  error_description="Not allowed for this job.";
  return 1;
}

int JobPlugin::removedir(std::string &dname) {
  if(!initialized) return 1;
  if(dname.find('/') == std::string::npos) { /* request to clean the job */
    if((dname == "new") || (dname == "info")) {
      error_description="Special directory can't be mangled.";
      return 1;
    };
    if(is_allowed(dname.c_str()) & IS_ALLOWED_WRITE) {  /* owner of the job */
      /* check the status */
      JobId id(dname); 
      std::string controldir = getControlDir(id);
      if (controldir.empty()) {
        error_description="No control information found for this job.";
        return 1;
      }    
      user->SetControlDir(controldir);
      std::string sessiondir = getSessionDir(id);
      if (sessiondir.empty()) {
        // session dir could have already been cleaned, so set to first in list
        sessiondir = user->SessionRoots().at(0);
      }    
      user->SetSessionRoot(sessiondir);
      job_state_t status=job_state_read_file(id,*user);
      if((status == JOB_STATE_FINISHED) ||
         (status == JOB_STATE_DELETED)) { /* remove files */
        if(job_clean_final(JobDescription(id,user->SessionRoot()+"/"+id),
                           *user)) return 0;
      }
      else { /* put marks */
        JobDescription job_desc(id,"");
        bool res = job_cancel_mark_put(job_desc,*user);
        res &= job_clean_mark_put(job_desc,*user);
        if(res) return 0;
      };
      error_description="Failed to clean job.";
      return 1;
    };
    error_description="Not allowed for this job.";
    return 1;
  };
  std::string id;
  bool spec_dir;
  if(is_allowed(dname.c_str(),false,&spec_dir,&id) & IS_ALLOWED_WRITE) {
    if(spec_dir) {
      error_description="Special directory can't be mangled.";
      return 1;
    };
    ApplyLocalCred(user,&id,"write");
    DirectFilePlugin * fp = selectFilePlugin(id);
    if((getuid()==0) && (user) && (user->StrictSession())) {
      SET_USER_UID;
      int r=fp->removedir(dname);
      RESET_USER_UID;
      return r;
    };
    return fp->removedir(dname);
  };
  error_description="Not allowed for this job.";
  return 1;
}

int JobPlugin::open(const char* name,open_modes mode,unsigned long long int size) {
  if(!initialized) return 1;
  if(rsl_opened) {  /* unfinished request - cancel */
    logger.msg(Arc::ERROR, "Request to open file with storing in progress");
    rsl_opened=false;
    delete_job_id();
    error_description="Job submission is still in progress.";
    return 1; 
  };
  /* check if acl request */
  if((strncmp(name,".gacl-",6) == 0) && (strchr(name,'/') == NULL)) {
    std::string newname(name+6);
    newname="info/"+newname+"/acl";
    return open(newname.c_str(),mode,size);
  };
  if( mode == GRIDFTP_OPEN_RETRIEVE ) {  /* open for reading */
    std::string fname;
    const char* logname;
    /* check if reading status files */
    bool spec_dir;
    if(is_allowed(name,false,&spec_dir,&fname,&logname) & IS_ALLOWED_READ) {
      std::string controldir = getControlDir(fname);
      if (controldir.empty()) {
        error_description="No control information found for this job.";
        return 1;
      }    
      user->SetControlDir(controldir);
      chosenFilePlugin = selectFilePlugin(fname);
      if(logname) {
        if((*logname) != 0) {
          if(strncmp(logname,"proxy",5) == 0) {
            error_description="Not allowed for this file.";
            chosenFilePlugin = NULL;
            return 1;
          }; 
          fname=user->ControlDir()+"/job."+fname+"."+logname;
          return chosenFilePlugin->open_direct(fname.c_str(),mode);
        };
      };
      if(spec_dir) {
        error_description="Special directory can't be mangled.";
        return 1;
      };
      ApplyLocalCred(user,&fname,"read");
      if((getuid()==0) && (user) && (user->StrictSession())) {
        SET_USER_UID;
        int r=chosenFilePlugin->open(name,mode);
        RESET_USER_UID;
        return r;
      };
      return chosenFilePlugin->open(name,mode);
    };
    error_description="Not allowed for this job.";
    return 1;
  }
  else if( mode == GRIDFTP_OPEN_STORE ) {
    std::string name_f(name);
    std::string::size_type n = name_f.find('/');
    if((n != std::string::npos) && (n != 0)) {
      if(((n==3) && 
          (strncmp(name_f.c_str(),"new",n) == 0)
         ) ||
         ((n==job_id.length()) && 
          (strncmp(name_f.c_str(),job_id.c_str(),n) == 0)
         )) {
        if(name_f.find('/',n+1) != std::string::npos) { // should not contain subdirs
          error_description="Can't create subdirectory here.";
          return 1;
        };
        // new job so select control and session dirs
        std::string controldir, sessiondir;
        if (!chooseControlAndSessionDir(job_id, controldir, sessiondir)) {
          error_description="No control and/or session directory available.";
          return 1;
        };
        user->SetControlDir(controldir);
        user->SetSessionRoot(sessiondir);
        if(job_id.length() == 0) {
          if(readonly) {
            error_description="You are not allowed to submit new jobs to this service.";
            logger.msg(Arc::ERROR, error_description);
            return 1;
          };
          if(!make_job_id()) {
            error_description="Failed to allocate ID for job.";
            logger.msg(Arc::ERROR, error_description);
            return 1;
          };
        };
        logger.msg(Arc::INFO, "Accepting submission of new job: %s", job_id);
        rsl_opened=true;
        chosenFilePlugin = selectFilePlugin(job_id);
        return 0;
      };
    };
    std::string id;
    bool spec_dir;
    const char* logname;
    if(is_allowed(name,true,&spec_dir,&id,&logname) & IS_ALLOWED_WRITE) {
      std::string controldir = getControlDir(id);
      if (controldir.empty()) {
        std::string sessiondir;
        if (!chooseControlAndSessionDir(job_id, controldir, sessiondir)) {
          error_description="No control and/or session directory available.";
          return 1;
        }
        user->SetSessionRoot(sessiondir);
      }    
      user->SetControlDir(controldir);
      chosenFilePlugin = selectFilePlugin(id);
      if(spec_dir) {
        // It is allowed to modify ACL
        if(logname) {
          if(strcmp(logname,"acl") == 0) {
            std::string fname=user->ControlDir()+"/job."+id+"."+logname;
            return chosenFilePlugin->open_direct(fname.c_str(),mode);
          };
        };
        error_description="Special directory can't be mangled.";
        chosenFilePlugin = NULL;
        return 1;
      };
      ApplyLocalCred(user,&id,"write");
      if((getuid()==0) && (user) && (user->StrictSession())) {
        SET_USER_UID;
        int r=chosenFilePlugin->open(name,mode,size);
        RESET_USER_UID;
        return r;
      };
      return chosenFilePlugin->open(name,mode,size);
    };
    error_description="Not allowed for this job.";
    return 1;
  }
  else {
    logger.msg(Arc::ERROR, "Unknown open mode %i", mode);
    error_description="Unknown/unsupported request.";
    return 1;
  };
}

int JobPlugin::close(bool eof) {
  if(!initialized || chosenFilePlugin == NULL) return 1;
  if(!rsl_opened) {
    if((getuid()==0) && (user) && (user->StrictSession())) {
      SET_USER_UID;
      int r=chosenFilePlugin->close(eof);
      RESET_USER_UID;
      return r;
    };
    return chosenFilePlugin->close(eof);
  };
  rsl_opened=false;
  if(job_id.length() == 0) {
    error_description="There is no job ID defined.";
    return 1;
  };
  if(!eof) { delete_job_id(); return 0; }; /* download was canceled */
  /* *************************************************
   * store RSL (description)                         *
   ************************************************* */
  std::string rsl_fname=user->ControlDir()+"/job."+job_id+".description";
  std::string acl("");
  /* analyze rsl (checking, substituting, etc)*/
  JobLocalDescription job_desc;
  if(parse_job_req(rsl_fname.c_str(),job_desc,&acl) != JobReqSuccess) {
    error_description="Failed to parse job/action description.";
    logger.msg(Arc::ERROR, error_description);
    delete_job_id();
    return 1;
  };
  if(job_desc.action.length() == 0) job_desc.action="request";
  if(job_desc.action == "cancel") {
    delete_job_id();
    if(job_desc.jobid.length() == 0) {
      error_description="Missing ID in request to cancel job.";
      logger.msg(Arc::ERROR, error_description);
      return 1;
    };
    return removefile(job_desc.jobid);
  };
  if(job_desc.action == "clean") {
    delete_job_id();
    if(job_desc.jobid.length() == 0) {
      error_description="Missing ID in request to clean job.";
      logger.msg(Arc::ERROR, error_description);
      return 1;
    };
    return removedir(job_desc.jobid);
  };
  if(job_desc.action == "renew") {
    delete_job_id();
    if(job_desc.jobid.length() == 0) {
      error_description="Missing ID in request to renew credentials.";
      logger.msg(Arc::ERROR, error_description);
      return 1;
    };
    return checkdir(job_desc.jobid);
  };
  if(job_desc.action == "restart") {
    delete_job_id();
    if(job_desc.jobid.length() == 0) {
      error_description="Missing ID in request to clean job.";
      logger.msg(Arc::ERROR, error_description);
      return 1;
    };
    const char* logname;
    std::string id;
    if(!(is_allowed(job_desc.jobid.c_str(),false,NULL,&id,&logname) & 
                                                       IS_ALLOWED_LIST)) {
      error_description="Not allowed for this job.";
      logger.msg(Arc::ERROR, error_description);
      return 1;
    };
    if(job_desc.jobid != id) {
      error_description="Wrong ID specified.";
      logger.msg(Arc::ERROR, error_description);
      return 1;
    };
    JobLocalDescription job_desc;
    if(!job_local_read_file(id,*user,job_desc)) {
      error_description="Job is probably corrupted: can't read internal information.";
      logger.msg(Arc::ERROR, error_description);
      return 1;
    };
    if(job_desc.failedstate.length() == 0) {
      error_description="Job can't be restarted.";
      logger.msg(Arc::ERROR, error_description);
      return 1;
    };
    if(job_desc.reruns <= 0) {
      error_description="Job run out number of allowed retries.";
      logger.msg(Arc::ERROR, error_description);
      return 1;
    };
    if(!job_restart_mark_put(JobDescription(id,""),*user)) {
      error_description="Failed to report restart request.";
      logger.msg(Arc::ERROR, error_description);
      return 1;
    };
    return 0;
  };
  if(job_desc.action != "request") {
    logger.msg(Arc::ERROR, "action(%s) != request", job_desc.action);
    error_description="Wrong action in job RSL description.";
    delete_job_id();
    return 1;
  };
  if((job_desc.jobid.length() != 0) && (job_desc.jobid != job_id)) {
    // Client may specify it's own job ID
    int h_old=Arc::FileOpen(rsl_fname.c_str(),O_RDONLY);
    delete_job_id(); 
    if(readonly) {
      ::close(h_old);
      remove(rsl_fname.c_str());
      error_description="New jobs are not allowed.";
      logger.msg(Arc::ERROR, error_description);
      return 1;
    };
    if(!make_job_id(job_desc.jobid)) {
      ::close(h_old);
      remove(rsl_fname.c_str());
      error_description="Failed to allocate requested job ID: "+job_desc.jobid;
      logger.msg(Arc::ERROR, error_description);
      return 1;
    };
    rsl_fname=user->ControlDir()+"/job."+job_id+".description";
    {
      int l = -1;
      if(h_old != -1) {
        int h=Arc::FileOpen(rsl_fname.c_str(),O_WRONLY,0600);
        if(h != -1) {
          for(;;) {
            char buf[256];
            l=::read(h_old,buf,sizeof(buf));
            if(l <= 0) break;
            const char* s = buf;
            for(;l;) {
              ssize_t ll=::write(h,s,l);
              if(ll <= 0) { l=-1; break; };
              l-=ll; s+=ll;
            };
            if(l < 0) break;
          };
          ::close(h);
        };
        ::close(h_old);
      };
      if(l == -1) {
        logger.msg(Arc::ERROR, "Failed writing RSL");
        remove(rsl_fname.c_str());
        error_description="Failed to store job RSL description.";
        delete_job_id(); return 1;
      };
    };
  };
  // Check for proper LRMS name in request. If there is no LRMS name 
  // in user configuration that means service is opaque frontend and
  // accepts any LRMS in request.
  if((!job_desc.lrms.empty()) && (!user->DefaultLRMS().empty())) {
    if(job_desc.lrms != user->DefaultLRMS()) {
      error_description="Request for LRMS "+job_desc.lrms+" is not allowed.";
      logger.msg(Arc::ERROR, error_description);
      delete_job_id(); 
      return 1;
    };
  };
  if(job_desc.lrms.empty()) job_desc.lrms=user->DefaultLRMS();
  // Check for proper queue in request. 
  if(job_desc.queue.empty()) job_desc.queue=user->DefaultQueue();
  if(job_desc.queue.empty()) {
    error_description="Request has no queue defined.";
    logger.msg(Arc::ERROR, error_description);
    delete_job_id(); 
    return 1;
  };
  if(avail_queues.size() > 0) { // If no queues configured - service takes any
    for(std::list<std::string>::iterator q = avail_queues.begin();;++q) {
      if(q == avail_queues.end()) {
        error_description="Requested queue "+job_desc.queue+" does not match any of available queues.";
        logger.msg(Arc::ERROR, error_description);
        delete_job_id(); 
        return 1;
      };
      if(*q == job_desc.queue) break; 
    };
  };
  JobDescription job(job_id,"",JOB_STATE_ACCEPTED);
  if(!process_job_req(*user, job, job_desc)) {
    error_description="Failed to preprocess job description.";
    logger.msg(Arc::ERROR, error_description);
    delete_job_id(); 
    return 1;
  };
  /* ****************************************
   * Start local file                       *
   **************************************** */
  /* !!!!! some parameters are unchecked here - rerun,diskspace !!!!! */
  job_desc.jobid=job_id;
  job_desc.starttime=time(NULL);
  job_desc.DN=subject;
  if(port != 0) {
    job_desc.clientname=
       Arc::tostring(host[0])+"."+Arc::tostring(host[1])+"."+
       Arc::tostring(host[2])+"."+Arc::tostring(host[3])+":"+
       Arc::tostring(port);
  };
  /* ***********************************************
   * Try to create proxy                           *
   *********************************************** */
  if(proxy_fname.length() != 0) {
    std::string fname=user->ControlDir()+"/job."+job_id+".proxy";
    int h=Arc::FileOpen(fname,O_WRONLY | O_CREAT | O_EXCL,0600);
    if(h == -1) {
      error_description="Failed to store credentials.";
      return 1;
    };
    int hh=Arc::FileOpen(proxy_fname,O_RDONLY);
    if(hh == -1) {
      ::close(h);
      ::remove(fname.c_str());
      error_description="Failed to read credentials.";
      return 1;
    };
    fix_file_owner(fname,*user);
    int l,ll;
    const char* s;
    char buf[256]; 
    for(;;) {
      ll=::read(hh,buf,sizeof(buf));
      if((ll==0) || (ll==-1)) break;
      for(l=0,s=buf;(ll>0) && (l!=-1);s+=l,ll-=l) l=::write(h,s,ll);
      if(l==-1) break;
    };
    ::close(h);
    ::close(hh);
    try {
      JobLog job_log;
      JobsListConfig jobs_cfg;
      GMEnvironment env(job_log,jobs_cfg);
      Arc::Credential ci(proxy_fname, proxy_fname, env.cert_dir_loc(), "");
      job_desc.expiretime = ci.GetEndTime();
    } catch (std::exception) {
      job_desc.expiretime = time(NULL);
    };
  };
  /* ******************************************
   * Write local file                         *
   ****************************************** */
  if(!job_local_write_file(job,*user,job_desc)) {
    logger.msg(Arc::ERROR, "Failed writing local description");
    delete_job_id(); 
    error_description="Failed to create job description.";
    return 1;
  };
  if(acl.length() != 0) {
    if(!job_acl_write_file(job_id,*user,acl)) {
      logger.msg(Arc::ERROR, "Failed writing ACL");
      delete_job_id(); 
      error_description="Failed to process/store job ACL.";
      return 1;
    };
  };
  /* ***********************************************
   * Call authentication/authorization plugin/exec *
   *********************************************** */
  /* talk to external plugin to ask if we can proceed */
  std::list<ContinuationPlugins::result_t> results;
  cont_plugins->run(job,*user,results);
  // analyze results
  std::list<ContinuationPlugins::result_t>::iterator result = results.begin();
  for(;result != results.end();++result) {
    if(result->action == ContinuationPlugins::act_fail) {
      logger.msg(Arc::ERROR, "Failed to run external plugin: %s", result->response);
      delete_job_id();
      error_description="Job is not allowed by external plugin: "+
                        result->response;
      return 1;
    } else if(result->action == ContinuationPlugins::act_log) {
      // Scream but go ahead
      logger.msg(Arc::ERROR, "Failed to run external plugin: %s", result->response);
    } else if(result->action == ContinuationPlugins::act_pass) {
      // Just continue
      if(result->response.length())
        logger.msg(Arc::INFO, "Plugin response: %s", result->response);
    } else {
      logger.msg(Arc::ERROR, "Failed to run external plugin");
      delete_job_id();
      error_description="Failed to pass external plugin.";
      return 1;
    };
  };
  /* ************************************************************
   * From here code accesses filesystem on behalf of local user *
   ************************************************************ */
  if(cred_plugin && (*cred_plugin)) {
    job_subst_t subst_arg;
    subst_arg.user=user;
    subst_arg.job=&job_id;
    subst_arg.reason="new";
    // run external plugin to acquire non-unix local credentials
    if(!cred_plugin->run(job_subst,&subst_arg)) {
      logger.msg(Arc::ERROR, "Failed to run plugin");
      delete_job_id();
      error_description="Failed to obtain external credentials.";
      return 1;
    };
    if(cred_plugin->result() != 0) {
      logger.msg(Arc::ERROR, "Plugin failed: %s", cred_plugin->result());
      delete_job_id();
      error_description="Failed to obtain external credentials.";
      return 1;
    };
  };
  /* *******************************************
   * Create session directory                  *
   ******************************************* */
  std::string dir=user->SessionRoot()+"/"+job_id;
  if((getuid()==0) && (user) && (user->StrictSession())) {
    SET_USER_UID;
  };
  if(mkdir(dir.c_str(),0700) != 0) {
    if((getuid()==0) && (user) && (user->StrictSession())) {
      RESET_USER_UID;
    };
    logger.msg(Arc::ERROR, "Failed to create session directory");
    delete_job_id();
    error_description="Failed to create session directory.";
    return 1;
  };
  if((getuid()==0) && (user) && (user->StrictSession())) {
    RESET_USER_UID;
  };
  fix_file_owner(dir,*user);
  /* **********************************************************
   * Create status file (do it last so GM picks job up here)  *
   ********************************************************** */
  if(!job_state_write_file(job,*user,JOB_STATE_ACCEPTED)) {
    logger.msg(Arc::ERROR, "Failed writing status");
    delete_job_id(); 
    error_description="Failed registering job in grid-manager.";
    return 1;
  };
  SignalFIFO(*user);
  job_id.resize(0);
  chosenFilePlugin = NULL;
  return 0;
}

int JobPlugin::read(unsigned char *buf,unsigned long long int offset,unsigned long long int *size) {
  if(!initialized || chosenFilePlugin == NULL) {
    error_description="Transfer is not initialised.";
    return 1;
  };
  error_description="Failed to read from disc.";
  if((getuid()==0) && (user) && (user->StrictSession())) {
    SET_USER_UID;
    int r=chosenFilePlugin->read(buf,offset,size);
    RESET_USER_UID;
    return r;
  };
  return chosenFilePlugin->read(buf,offset,size);
}

int JobPlugin::write(unsigned char *buf,unsigned long long int offset,unsigned long long int size) {
  if(!initialized || chosenFilePlugin == NULL) {
    error_description="Transfer is not initialised.";
    return 1;
  };
  error_description="Failed to write to disc.";
  if(!rsl_opened) {
    if((getuid()==0) && (user) && (user->StrictSession())) {
      SET_USER_UID;
      int r=chosenFilePlugin->write(buf,offset,size);
      RESET_USER_UID;
      return r;
    };
    return chosenFilePlugin->write(buf,offset,size);
  };
  /* write to rsl */
  if(job_id.length() == 0) {
    error_description="No job ID defined.";
    return 1;
  };
  if((job_rsl_max_size > 0) && ((offset+size) >= job_rsl_max_size)) {
    error_description="Job description is too big.";
    return 1;
  };
  std::string rsl_fname=user->ControlDir()+"/job."+job_id+".description";
  int h=Arc::FileOpen(rsl_fname.c_str(),O_WRONLY|O_CREAT,0600);
  if(h == -1) {
    error_description="Failed to open job description file.";
    return 1;
  };
  if(::lseek(h,offset,SEEK_SET) != offset) {
    ::close(h);
    error_description="Failed to seek in job description file.";
    return 1;
  };
  for(;size;) {
    ssize_t l = ::write(h,buf,size);
    if(l <= 0) {
      ::close(h);
      error_description="Failed to write job description file.";
      return 1;
    };
    size-=l; buf+=l;
  };
  fix_file_owner(rsl_fname,*user);
  ::close(h);
  // remove desc file used to claim job id, if different from this one
  if (user->ControlDir() != gm_dirs_info.at(gm_dirs_info.size()-1).control_dir) {
    rsl_fname=gm_dirs_info.at(gm_dirs_info.size()-1).control_dir+"/job."+job_id+".description";
    remove(rsl_fname.c_str());
  }
  return 0;
}

int JobPlugin::readdir(const char* name,std::list<DirEntry> &dir_list,DirEntry::object_info_level mode) {
  if(!initialized) {
    error_description="Plugin is not initialised.";
    return 1;
  };
  if((name[0] == 0) || (strcmp("info",name) == 0)) { /* root jobs directory or jobs' info */
    if(name[0] == 0) {
      DirEntry dent_new(false,"new");
      DirEntry dent_info(false,"info");
      dent_new.may_dirlist=true;
      dent_info.may_dirlist=true;
      dir_list.push_back(dent_new);
      dir_list.push_back(dent_info);
    };
    // loop through all control dirs
    for (std::vector<struct gm_dirs_>::iterator i = gm_dirs_info.begin(); i != gm_dirs_info.end(); i++) {
      std::string cdir=(*i).control_dir;
      Glib::Dir *dir=Arc::DirOpen(cdir);
      if(dir != NULL) {
        std::string file_name;
        while ((file_name = dir->read_name()) != "") {
          std::vector<std::string> tokens;
          Arc::tokenize(file_name, tokens, "."); // look for job.id.local
          if (tokens.size() == 3 && tokens[0] == "job" && tokens[2] == "local") {
            JobLocalDescription job_desc;
            std::string fname=cdir+'/'+file_name;
            if(job_local_read_file(fname,job_desc)) {
              if(job_desc.DN == subject) {
                JobId id(tokens[1]);
                dir_list.push_back(DirEntry(false,id.c_str()));
              };
            };
          };
        };
      };
      dir->close();
    };
    return 0;
  };
  if(strcmp(name,"new") == 0) { /* directory /new is always empty */
    return 0;
  };
  /* check for allowed job directory */
  const char* logname;
  std::string id;
  std::string log;
  if(is_allowed(name,false,NULL,&id,&logname,&log) & IS_ALLOWED_LIST) {
    if(logname) {
      std::string controldir = getControlDir(id);
      if (controldir.empty()) {
        error_description="No control information found for this job.";
        return 1;
      }    
      user->SetControlDir(controldir);
      if((*logname) != 0) {
        if(strchr(logname,'/') != NULL) return 1; /* no subdirs */
        if(strncmp(logname,"proxy",5) == 0) return 1;
        id=user->ControlDir()+"/job."+id+"."+logname;
        struct stat st;
        if(::stat(id.c_str(),&st) != 0) return 1;
        if(!S_ISREG(st.st_mode)) return 1;
        DirEntry dent(true,logname);
        if(strncmp(logname,"proxy",5) != 0) dent.may_read=true;
        dir_list.push_back(dent);
        return -1;
      };
      Glib::Dir* d=Arc::DirOpen(user->ControlDir());
      if(d == NULL) { return 1; }; /* maybe return ? */
      id="job."+id+".";
      std::string file_name;
      while ((file_name = d->read_name()) != "") {
        if(file_name.substr(0, id.length()) != id) continue;
        if(file_name.substr(file_name.length() - 5) == "proxy") continue;
        DirEntry dent(true, file_name.substr(id.length()));
        dir_list.push_back(dent);
      };
      d->close();
      return 0;
    };
    if(log.length() > 0) {
      const char* s = strchr(name,'/');
      if((s == NULL) || (s[1] == 0)) {
        DirEntry dent(false,log.c_str());
        dent.may_dirlist=true;
        dir_list.push_back(dent);
      };
    };
    /* allowed - pass to file system */
    ApplyLocalCred(user,&id,"read");
    chosenFilePlugin = selectFilePlugin(id);
    if((getuid()==0) && (user) && (user->StrictSession())) {
      SET_USER_UID;
      int r=chosenFilePlugin->readdir(name,dir_list,mode);
      RESET_USER_UID;
      return r;
    };
    return chosenFilePlugin->readdir(name,dir_list,mode);
  };
  error_description="Not allowed for this job.";
  return 1;
}

int JobPlugin::checkdir(std::string &dirname) {
  if(!initialized) return 1;
  /* chdir to /new will create new job */
  if(dirname.length() == 0) return 0; /* root */
  if(dirname == "new") { /* new job */
    if(readonly) {
      error_description="New jobs are not allowed.";
      logger.msg(Arc::ERROR, error_description);
      return 1;
    };
    if(!make_job_id()) {
      error_description="Failed to allocate ID for job.";
      logger.msg(Arc::ERROR, error_description);
      return 1;
    };
    dirname=job_id;
    return 0;
  };
  if(dirname == "info") { /* always allowed */
    return 0;
  };
  const char* logname;
  std::string id;
  if(is_allowed(dirname.c_str(),false,NULL,&id,&logname) & IS_ALLOWED_LIST) {
    std::string controldir = getControlDir(id);
    if (controldir.empty()) {
      error_description="No control information found for this job.";
      return 1;
    }    
    user->SetControlDir(controldir);
    if(logname) {
      if((*logname) != 0) {
        error_description="There is no such special subdirectory.";
        return 1; /* log directory has no subdirs */
      };
      return 0;
    };
    if((dirname == id) && (proxy_fname.length())) {  /* cd to session directory - renew proxy request */
      JobLocalDescription job_desc;
      if(!job_local_read_file(id,*user,job_desc)) {
        error_description="Job is probably corrupted: can't read internal information.";
        logger.msg(Arc::ERROR, error_description);
        return 1;
      };
      /* check if new proxy is better than old one */
      std::string old_proxy_fname=user->ControlDir()+"/job."+id+".proxy";
      time_t new_proxy_expires = time(NULL);
      time_t old_proxy_expires = time(NULL);
      JobLog job_log;
      JobsListConfig jobs_cfg;
      GMEnvironment env(job_log,jobs_cfg);
      try {
        Arc::Credential new_ci(proxy_fname, proxy_fname, env.cert_dir_loc(), "");
        job_desc.expiretime = new_ci.GetEndTime();
      } catch (std::exception) { };
      try {
        Arc::Credential old_ci(old_proxy_fname, old_proxy_fname, env.cert_dir_loc(), "");
        job_desc.expiretime = old_ci.GetEndTime();
      } catch (std::exception) { };
      if(((int)(new_proxy_expires-old_proxy_expires)) > 0) {
        /* try to renew proxy */
        if(renew_proxy(old_proxy_fname.c_str(),proxy_fname.c_str()) == 0) {
          fix_file_owner(old_proxy_fname,*user);
          logger.msg(Arc::INFO, "New proxy expires at %s", Arc::TimeStamp(Arc::Time(new_proxy_expires), Arc::MDSTime));
          JobDescription job(id,"",JOB_STATE_ACCEPTED);
          job_desc.expiretime=new_proxy_expires;
          if(!job_local_write_file(job,*user,job_desc)) {
            logger.msg(Arc::ERROR, "Failed to write 'local' information");
          };
          error_description="Applying external credentials locally failed.";
          ApplyLocalCred(user,&id,"renew");
          error_description="";
          /* Cause restart of job if it potentially failed 
             because of expired proxy */
          if((((int)(old_proxy_expires-time(NULL))) <= 0) && (
              (job_desc.failedstate == 
                    JobDescription::get_state_name(JOB_STATE_PREPARING)) ||
              (job_desc.failedstate == 
                    JobDescription::get_state_name(JOB_STATE_FINISHING))
             )
            ) {
            logger.msg(Arc::INFO, "Job could have died due to expired proxy: restarting");
            if(!job_restart_mark_put(JobDescription(id,""),*user)) {
              logger.msg(Arc::ERROR, "Failed to report renewed proxy to job");
            };
          };
        } else {
          logger.msg(Arc::ERROR, "Failed to renew proxy");
        };
      };
    };
    ApplyLocalCred(user,&id,"read");
    chosenFilePlugin = selectFilePlugin(id);
    if((getuid()==0) && (user) && (user->StrictSession())) {
      SET_USER_UID;
      int r=chosenFilePlugin->checkdir(dirname);
      RESET_USER_UID;
      return r;
    };
    return chosenFilePlugin->checkdir(dirname);
  };
  error_description="Not allowed for this job.";
  return 1;
}

int JobPlugin::checkfile(std::string &name,DirEntry &info,DirEntry::object_info_level mode) {
  if(!initialized) return 1;
  if(name.length() == 0) {
    info.name=""; info.is_file=false;
    return 0; 
  };
  if((name == "new") || (name == "info")) {
    info.name=""; info.is_file=false;
    return 0; 
  };
  const char* logname;
  std::string id;
  if(is_allowed(name.c_str(),false,NULL,&id,&logname) & IS_ALLOWED_LIST) {
    std::string controldir = getControlDir(id);
    if (controldir.empty()) {
      error_description="No control information found for this job.";
      return 1;
    }    
    user->SetControlDir(controldir);
    if(logname) {
      if((*logname) == 0) { /* directory itself */
        info.is_file=false; info.name=""; info.may_dirlist=true;
      }
      else {
        if(strncmp(logname,"proxy",5) == 0) {
          error_description="There is no such special file.";
          return 1;
        };
        id=user->ControlDir()+"/job."+id+"."+logname;
        struct stat st;
        if(::stat(id.c_str(),&st) != 0) {
          error_description="There is no such special file.";
          return 1;
        };
        if(!S_ISREG(st.st_mode)) {
          error_description="There is no such special file.";
          return 1;
        };
        info.is_file=true; info.name="";
        info.may_read=true; info.size=st.st_size;
      };
      return 0;
    };
    ApplyLocalCred(user,&id,"read");
    chosenFilePlugin = selectFilePlugin(id);
    if((getuid()==0) && (user) && (user->StrictSession())) {
      SET_USER_UID;
      int r=chosenFilePlugin->checkfile(name,info,mode);
      RESET_USER_UID;
      return r;
    };
    return chosenFilePlugin->checkfile(name,info,mode);
  };
  error_description="Not allowed for this job.";
  return 1;
}

bool JobPlugin::delete_job_id(void) {
  if(job_id.length() != 0) {
    std::string controldir = getControlDir(job_id);
    if (controldir.empty()) {
      error_description="No control information found for this job.";
      return false;
    }    
    user->SetControlDir(controldir);
    std::string sessiondir = getSessionDir(job_id);
    if (sessiondir.empty()) {
      // session dir could have already been cleaned, so set to first in list
      sessiondir = user->SessionRoots().at(0);
    }    
    user->SetSessionRoot(sessiondir);
    job_clean_final(JobDescription(job_id,user->SessionRoot()+"/"+job_id),*user);
    job_id="";
  };
  return true;
}

bool JobPlugin::make_job_id(const std::string &id) {
  if((id.find('/') != std::string::npos) || (id.find('\n') != std::string::npos)) {
    logger.msg(Arc::ERROR, "ID contains forbidden characters");
    return false;
  };
  if((id == "new") || (id == "info")) return false;
  // claim id by creating empty description file
  std::string fname=user->ControlDir()+"/job."+id+".description";
  struct stat st;
  if(stat(fname.c_str(),&st) == 0) return false;
  int h = Arc::FileOpen(fname.c_str(),O_RDWR | O_CREAT | O_EXCL,S_IRWXU);
  // So far assume control directory is on local fs.
  // TODO: add locks or links for NFS
  if(h == -1) return false;
  // check the new ID is not used in other control dirs
  for (std::vector<gm_dirs_>::iterator i = gm_dirs_info.begin(); i != gm_dirs_info.end(); i++) {
    if (i->control_dir == user->ControlDir())
      continue;
    std::string desc_fname=i->control_dir+"/job."+job_id+".description";
    struct stat st;
    if(stat(desc_fname.c_str(),&st) == 0) {
      close(h);
      remove(fname.c_str());
      return false;
    }
  }
  fix_file_owner(fname,*user);
  close(h);
  delete_job_id();
  job_id=id;
  return true;
}

bool JobPlugin::make_job_id(void) {
  int i;
  bool found = false;
  delete_job_id();
  for(i=0;i<100;i++) {
    job_id=Arc::tostring((unsigned int)getpid())+
           Arc::tostring((unsigned int)time(NULL))+
           Arc::tostring(rand(),1);
    // create job.id.description file then loop through all control dirs to find if it already exists
    std::string fname=user->ControlDir()+"/job."+job_id+".description";
    int h = Arc::FileOpen(fname.c_str(),O_RDWR | O_CREAT | O_EXCL,0600);
    // So far assume control directory is on local fs.
    // TODO: add locks or links for NFS
    if(h == -1) {
      if(errno == EEXIST) continue;
      logger.msg(Arc::ERROR, "Failed to create file in %s", user->ControlDir());
      return false;
    };
    for (std::vector<gm_dirs_>::iterator i = gm_dirs_info.begin(); i != gm_dirs_info.end(); i++) {
      if (i->control_dir == user->ControlDir())
        continue;
      std::string desc_fname=i->control_dir+"/job."+job_id+".description";
      struct stat st;
      if(stat(desc_fname.c_str(),&st) == 0) {
        found = true;
        break;
      }
    }
    if (found) {
      found = false;
      close(h);
      remove(fname.c_str());
      continue;
    }
    // safe to use this id
    fix_file_owner(fname,*user);
    close(h);
    break;
  };
  if(i>=100) {
    logger.msg(Arc::ERROR, "Out of tries while allocating new job id");
    job_id=""; return false;
  };
  return true;
}

/*
  name - name of file to access
  locked - true if job already running
  jobid - returns id extracted from name
  logname - name of log file (errors, status, etc.)
  log - stdlog of job
  spec_dir - if file belogs to virtual directory 
  returns access rights. For special files superset of all  rights is
  returned. Distinction between files is processed at higher levels.
*/
int JobPlugin::is_allowed(const char* name,bool locked,bool* spec_dir,std::string* jobid,char const ** logname,std::string* log) {
  if(logname) (*logname) = NULL;
  if(log) (*log)="";
  if(spec_dir) (*spec_dir)=false;
  JobId id(name);
  if(id == "info") { // directory which contains list of jobs-directories
    if(spec_dir) (*spec_dir)=false;
    return (IS_ALLOWED_READ | IS_ALLOWED_LIST);
  };
  if(strncmp(id.c_str(),"info/",5) == 0) {
    if(spec_dir) (*spec_dir)=true;
    name+=5; id=name;
    std::string::size_type n=id.find('/'); if(n != std::string::npos) id.erase(n);
    if(jobid) (*jobid)=id;
    if(id.length() == 0) return 0;
    const char* l_name = name+id.length();
    if(l_name[0] == '/') l_name++;
    if(logname) { (*logname)=l_name; };
    JobLocalDescription job_desc;
    std::string controldir = getControlDir(id);
    if (controldir.empty()) {
      error_description="No control information found for this job.";
      return 1;
    }    
    user->SetControlDir(controldir);
    if(!job_local_read_file(id,*user,job_desc)) return false;
    if(job_desc.DN != subject) {
      // Not an owner. Check acl.
      std::string acl_file = user->ControlDir()+"/job."+id+".acl";
      struct stat st;
      if(stat(acl_file.c_str(),&st) == 0) {
        if(S_ISREG(st.st_mode)) {
          GACLacl* acl = GACLloadAcl((char*)(acl_file.c_str()));
          if(acl) {
            GACLperm perm = AuthUserGACLTest(acl,user_a);
            int res = 0;
            if(GACLhasList(perm))
              res|=IS_ALLOWED_LIST;
            if(GACLhasRead(perm) | GACLhasWrite(perm))
              res|=(IS_ALLOWED_READ | IS_ALLOWED_LIST);
            if(GACLhasAdmin(perm))
              res|=(IS_ALLOWED_READ | IS_ALLOWED_WRITE | IS_ALLOWED_LIST);
            //if(strncmp(l_name,"proxy",5) == 0) res&=IS_ALLOWED_LIST;
            //if(strncmp(l_name,"acl",3) != 0) res&=~IS_ALLOWED_WRITE;
            return res;
          };
        };
      };
      return 0;
    };
    //if(strncmp(l_name,"proxy",5) == 0) return (IS_ALLOWED_LIST);
    //if(strncmp(l_name,"acl",3) != 0) return (IS_ALLOWED_READ | IS_ALLOWED_LIST);;
    return (IS_ALLOWED_READ | IS_ALLOWED_WRITE | IS_ALLOWED_LIST);
  };
  std::string::size_type n=id.find('/'); if(n != std::string::npos) id.erase(n);
  if(jobid) (*jobid)=id;
  JobLocalDescription job_desc;
  std::string controldir = getControlDir(id);
  if (controldir.empty()) {
    error_description="No control information found for this job.";
    return 1;
  }    
  user->SetControlDir(controldir);
  if(job_local_read_file(id,*user,job_desc)) {
    int res = 0;
    bool spec = false;
    //bool proxy = false;
    //bool acl = false;
    if(log) (*log)=job_desc.stdlog;
    if(n != std::string::npos) {
      int l = job_desc.stdlog.length();
      if(l != 0) {
        if(strncmp(name+n+1,job_desc.stdlog.c_str(),l) == 0) {
          if(name[n+1+l] == 0) {
            if(spec_dir) (*spec_dir)=true;
            if(logname) (*logname)=name+n+1+l;
            spec=true;
          } else if(name[n+1+l] == '/') {
            if(spec_dir) (*spec_dir)=true;
            if(logname) (*logname)=name+n+1+l+1;
            spec=true;
            //if(strncmp(name+n+1+l+1,"proxy",5) == 0) proxy=true;
            //if(strncmp(name+n+1+l+1,"acl",3) == 0) acl=true;
          };
        };
      };
    };
    if(job_desc.DN == subject) {
      res|=(IS_ALLOWED_READ | IS_ALLOWED_WRITE | IS_ALLOWED_LIST);
    } else {
      // Not an owner. Check acl.
      std::string acl_file = user->ControlDir()+"/job."+id+".acl";
      struct stat st;
      if(stat(acl_file.c_str(),&st) == 0) {
        if(S_ISREG(st.st_mode)) {
          GACLacl* acl = GACLloadAcl((char*)(acl_file.c_str()));
          if(acl) {
            GACLperm perm = AuthUserGACLTest(acl,user_a);
            if(spec) {
              if(GACLhasList(perm))
                res|=IS_ALLOWED_LIST;
              if(GACLhasRead(perm) | GACLhasWrite(perm))
                res|=(IS_ALLOWED_READ | IS_ALLOWED_LIST);
              if(GACLhasAdmin(perm))
                res|=(IS_ALLOWED_READ | IS_ALLOWED_WRITE | IS_ALLOWED_LIST);
            } else {
              if(GACLhasList(perm)) res|=IS_ALLOWED_LIST;
              if(GACLhasRead(perm)) res|=IS_ALLOWED_READ;
              if(GACLhasWrite(perm)) res|=IS_ALLOWED_WRITE;
              if(GACLhasAdmin(perm))
                res|=(IS_ALLOWED_READ | IS_ALLOWED_WRITE | IS_ALLOWED_LIST);
            };
          } else {
            logger.msg(Arc::ERROR, "Failed to read job's ACL for job %s from %s", id, user->ControlDir());
          };
        };
      };
    };
    return res; 
  } else {
    logger.msg(Arc::ERROR, "Failed to read job's local description for job %s from %s", id, user->ControlDir());
  };
  return 0;
}

/*
 * Methods to deal with multple control and session dirs
 */
DirectFilePlugin * JobPlugin::selectFilePlugin(std::string id) {
  if (file_plugins.size() == 1)
    return file_plugins.at(0);
  
  // get session dir
  std::string sd = getSessionDir(id);
  if (sd.empty())
    return file_plugins.at(0);
    
  // match to our list
  if (session_dirs.size() > 1) {
    // find this id's session dir from the list of session dirs
    for (unsigned int i = 0; i < session_dirs.size(); i++) {
      if (session_dirs.at(i) == sd) {
        return file_plugins.at(i);
      }
    }
  }
  else {
    // find this id's session dir from the gm_dirs info
    for (unsigned int i = 0; i < gm_dirs_info.size(); i++) {
      if (gm_dirs_info.at(i).session_dir == sd) {
        return file_plugins.at(i);
      }
    }
  }
  // error - shouldn't be possible but return first in list
  return file_plugins.at(0);
}

bool JobPlugin::chooseControlAndSessionDir(std::string job_id, std::string& controldir, std::string& sessiondir) {

  if (gm_dirs_non_draining.size() == 0 || session_dirs_non_draining.size() == 0) {
    // no active control or session dirs available
    logger.msg(Arc::ERROR, "No non-draining control or session dirs available");
    return false;
  }
  // if multiple session dirs are defined, don't use remote dirs
  if (session_dirs.size() > 1) {
    // the 'main' control dir is last in the list
    controldir = gm_dirs_info.at(gm_dirs_info.size()-1).control_dir;
    // choose randomly from non-draining session dirs
    sessiondir = session_dirs_non_draining.at(rand() % session_dirs_non_draining.size());
  }
  else {
    // choose randomly from non-draining gm_dirs_info
    unsigned int i = rand() % gm_dirs_non_draining.size();
    controldir = gm_dirs_non_draining.at(i).control_dir; 
    sessiondir = gm_dirs_non_draining.at(i).session_dir;
  } 
  logger.msg(Arc::INFO, "Using control dir %s", controldir);
  logger.msg(Arc::INFO, "Using session dir %s", sessiondir);
  return true;
}  
  

std::string JobPlugin::getControlDir(std::string id) {
  
  // if multiple session dirs are defined we only have one control dir
  if (session_dirs.size() > 1 || gm_dirs_info.size() == 1) {
    // the 'main' control dir is last in the list
    return gm_dirs_info.at(gm_dirs_info.size()-1).control_dir;
  }
  // check for existence of job.id.description file (the first created)
  for (unsigned int i = 0; i < gm_dirs_info.size(); i++) {
    JobUser u(*user);
    u.SetControlDir(gm_dirs_info.at(i).control_dir);
    JobId jobid(id);
    std::string rsl;
    if (job_description_read_file(jobid, u, rsl))
      return gm_dirs_info.at(i).control_dir;
  }
  // no control info found
  std::string empty("");
  return empty;
}

std::string JobPlugin::getSessionDir(std::string id) {

  // if multiple session dirs are defined, don't use remote dirs
  if (session_dirs.size() > 1) {
    // look for this id's session dir
    struct stat st;
    for (unsigned int i = 0; i < session_dirs.size(); i++) {
      std::string sessiondir(session_dirs.at(i) + '/' + id);
      if (stat(sessiondir.c_str(), &st) == 0 && S_ISDIR(st.st_mode)) {
        return session_dirs.at(i);
      }
    }
  }
  else {
    struct stat st;
    for (unsigned int i = 0; i < gm_dirs_info.size(); i++) {
      std::string sessiondir(gm_dirs_info.at(i).session_dir + '/' + id);
      if (stat(sessiondir.c_str(), &st) == 0 && S_ISDIR(st.st_mode)) {
        return gm_dirs_info.at(i).session_dir;
      }
    }
  }
  // no session dir found
  std::string empty("");
  return empty;
}
