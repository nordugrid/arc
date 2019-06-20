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
#include <dlfcn.h>

#include <arc/StringConv.h>
#include <arc/FileUtils.h>
#include <arc/DateTime.h>
#include <arc/Logger.h>
#include <arc/Utils.h>
#include <arc/GUID.h>
#include <arc/credential/Credential.h>
#include <arc/ArcConfigFile.h>
#include <arc/ArcConfigIni.h>

#include "../jobs/GMJob.h"
#include "../jobs/CommFIFO.h"
#include "../jobs/ContinuationPlugins.h"
#include "../files/ControlFileContent.h"
#include "../files/ControlFileHandling.h"
#include "../jobs/JobDescriptionHandler.h"
#include "../misc/proxy.h"
#include "../run/RunParallel.h"
#include "../../../gridftpd/userspec.h"
#include "../../../gridftpd/names.h"
#include "../../../gridftpd/misc.h"
#include "../../../gridftpd/fileplugin/fileplugin.h"

#include "jobplugin.h"

using namespace ARex;

static Arc::Logger logger(Arc::Logger::getRootLogger(),"JobPlugin");

typedef struct {
  const GMConfig* config;
  const Arc::User* user;
  const std::string* job;
  const char* reason;
} job_subst_t;

#ifdef HAVE_SETFSUID

// Non-portable solution. Needed as long as Linux
// does not support proper setuid in threads
#define SET_USER_UID(FP) {  ::setfsuid(FP->get_uid()); ::setfsgid(FP->get_gid()); }
#define RESET_USER_UID { ::setfsuid(getuid()); ::setfsgid(getgid()); }

#else

// Not sure how this will affect other threads. Most probably 
// not in a best way. Anyway this option is not for linux.
#define SET_USER_UID(FP) { setegid(FP->get_gid()); seteuid(FP->get_uid()); }
#define RESET_USER_UID { seteuid(getuid()); setegid(getgid()); }

#endif


class DirectUserFilePlugin: public DirectFilePlugin {
 public:
  DirectUserFilePlugin(std::string const& sd, uid_t suid, gid_t sgid, userspec_t& user):
     DirectFilePlugin(*std::unique_ptr<std::istream>(make_config(sd, suid, sgid)), user),
     uid(suid), gid(sgid) {
  }

  uid_t get_uid() const { return uid; }

  gid_t get_gid() const { return gid; }

 private:
  uid_t uid;
  gid_t gid;

  static std::istream* make_config(std::string const& sd, uid_t suid, gid_t sgid) {
    std::string direct_config = "";
    direct_config+="mount "+sd+"\n";
    direct_config+="dir / nouser read cd dirlist delete append overwrite";
    direct_config+=" create "+Arc::tostring(suid)+":"+Arc::tostring(sgid)+" 600:600";
    direct_config+=" mkdir "+Arc::tostring(suid)+":"+Arc::tostring(sgid)+" 700:700\n";
    direct_config+="end\n";
#ifdef HAVE_SSTREAM
    std::stringstream* fake_cfile = new std::stringstream(direct_config);
#else
    std::strstream* fake_cfile = new std::strstream;
    (*fake_cfile)<<direct_config;
#endif
    return fake_cfile;
  }

  static userspec_t* make_user(AuthUser& user_a) {
    userspec_t* user_s = new userspec_t;
    user_s->fill(user_a);
    return user_s;
  }

};


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
  if(subs->user && subs->config) subs->config->Substitute(str, *(subs->user));
}

JobPlugin::JobPlugin(std::istream &cfile,userspec_t &user_s,FileNode& node):
    cont_plugins(new ContinuationPlugins),
    //user_a(user_s.user),
    user_s(user_s),
    matched_vo(NULL),
    matched_voms(NULL) {

  // Because this plugin can load own plugins it's symbols need to 
  // become available in order to avoid duplicate resolution.
  phandle = dlopen(node.get_plugin_path().c_str(),RTLD_NOW | RTLD_GLOBAL);
  initialized=true;
  rsl_opened=false;
  job_rsl_max_size = DEFAULT_JOB_RSL_MAX_SIZE;
  proxy_fname="";
  proxy_is_deleg=false;
  std::string configfile = user_s.get_config_file();
  readonly=false;
  std::list<std::string> readonly_override;
  //matched_vo=user_a.default_group_vo();
  matched_vo=user_s.user.default_group_vo();
  //matched_voms=user_a.default_group_voms();
  matched_voms=user_s.user.default_group_voms();
  srand(time(NULL) + rand()); 
  for(;;) {
    std::string rest = Arc::ConfigFile::read_line(cfile);
    std::string command = Arc::ConfigIni::NextArg(rest);
    if(command.length() == 0) { break; } /* end of file - should not be here */
    if(command == "allownew") {
      std::string value = rest;
      if(strcasecmp(value.c_str(),"no") == 0) { readonly=true; }
      else if(strcasecmp(value.c_str(),"yes") == 0) { readonly=false; }
      else { logger.msg(Arc::WARNING, "Unsupported value for allownew: %s", value); };
    } else if(command == "allownew_override") {
      for(;;) {
        std::string value = Arc::ConfigIni::NextArg(rest);
        if(value.empty()) break;
        readonly_override.push_back(value);
      };
    } else if(command == "maxjobdesc") {
      if(rest.empty()) {
        job_rsl_max_size = 0;
      } else if(sscanf(rest.c_str(),"%u",&job_rsl_max_size) != 1) {
        logger.msg(Arc::ERROR, "Wrong number in maxjobdesc");
        initialized=false;
      };
    } else if(command == "endpoint") {
      endpoint = rest;
    } else if(command == "end") {
      break; /* end of section */
    } else {
      logger.msg(Arc::WARNING, "Unsupported configuration command: %s", command);
    };
  };
  if(configfile.length()) config.SetConfigFile(configfile);
  config.SetContPlugins(cont_plugins);
  std::string uname = user_s.get_uname();
  std::string ugroup = user_s.get_gname();
  user = Arc::User(uname, ugroup);
  if (!user) {
    logger.msg(Arc::ERROR, "Mapped user:group (%s:%s) not found", uname, ugroup);
    initialized = false;
  } else if((user.get_uid() == 0) && (getuid() == 0)) {
    logger.msg(Arc::INFO, "Job submission user can't be root");
    initialized = false;
  } else if(!config.Load()) { // read configuration
    logger.msg(Arc::ERROR, "Failed processing A-REX configuration");
    initialized=false;
  } else {
    avail_queues = config.Queues();
    // set default queue if not given explicitly
    if(config.DefaultQueue().empty() && (avail_queues.size() == 1)) {
      config.SetDefaultQueue(*(avail_queues.begin()));
    }
    // do substitutions in session dirs based on mapped user
    session_dirs = config.SessionRoots();
    for (std::vector<std::string>::iterator session = session_dirs.begin(); session != session_dirs.end(); ++session) {
      config.Substitute(*session, user);
    }
    session_dirs_non_draining = config.SessionRootsNonDraining();
    for (std::vector<std::string>::iterator session = session_dirs_non_draining.begin();
         session != session_dirs_non_draining.end(); ++session) {
      config.Substitute(*session, user);
    }
    if(readonly) {
      for(std::list<std::string>::iterator group = readonly_override.begin(); group != readonly_override.end(); ++group) {
        //if(user_a.check_group(*group)) { readonly=false; break; };
        if(user_s.user.check_group(*group)) { readonly=false; break; };
      };
    };
    if(readonly) logger.msg(Arc::WARNING, "This user is denied to submit new jobs.");
    if (!config.ControlDir().empty() && !session_dirs.empty()) {
      control_dir = config.ControlDir();
    }
    if (control_dir.empty()) {
      logger.msg(Arc::ERROR, "No control or session directories defined in configuration");
      initialized = false;
    }

    logger.msg(Arc::INFO, "Job submission user: %s (%i:%i)", uname, user.get_uid(), user.get_gid());
  }
  if(!initialized) {
    logger.msg(Arc::ERROR, "Job plugin was not initialised");
  }
  deleg_db_type = DelegationStore::DbSQLite;
  switch(config.DelegationDBType()) {
   case GMConfig::deleg_db_bdb:
    deleg_db_type = DelegationStore::DbBerkeley;
    break;
   case GMConfig::deleg_db_sqlite:
    deleg_db_type = DelegationStore::DbSQLite;
    break;
  };
  if(user_s.user.proxy() != NULL) {
  //if(user_a.proxy() != NULL) {
    proxy_fname=user_s.user.proxy();
   // proxy_fname=user_a.proxy();
    if((!proxy_fname.empty()) && user_s.user.is_proxy()) {
    //if((!proxy_fname.empty()) && user_a.is_proxy()) {
      proxy_is_deleg = true;
    }
  }
  if(!proxy_is_deleg) {
    logger.msg(Arc::WARNING, "No delegated credentials were passed");
  };
  //subject=user_a.DN();
  subject=user_s.user.DN();
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
  if(!proxy_fname.empty()) { remove(proxy_fname.c_str()); };
  if(cont_plugins) delete cont_plugins;
  if(phandle) dlclose(phandle);
}
 
std::string JobPlugin::get_error_description() const {
  if (!error_description.empty()) return error_description;
  if (!chosenFilePlugin) return std::string("");
  return chosenFilePlugin->error_description;
}

int JobPlugin::makedir(std::string &dname) {
  if(!initialized) return 1;
  std::string id;
  bool spec_dir;
  if((dname == "new") || (dname == "info")) return 0;
  if(!is_allowed(dname.c_str(),IS_ALLOWED_WRITE,&spec_dir,&id)) return 1;
  if(spec_dir) {
    error_description="Can't create subdirectory in a special directory.";
    return 1;
  };
  std::unique_ptr<DirectUserFilePlugin> fp(makeFilePlugin(id));
  int r;
  if((getuid()==0) && config.StrictSession()) {
    SET_USER_UID(fp);
    r=fp->makedir(dname);
    RESET_USER_UID;
  } else {
    r=fp->makedir(dname);
  }
  if (r != 0) error_description = fp->get_error_description();
  return r;
}

int JobPlugin::removefile(std::string &name) {
  if(!initialized) return 1;
  if(name.find('/') == std::string::npos) { /* request to cancel the job */
    if((name == "new") || (name == "info")) {
      error_description="Special directory can't be mangled.";
      return 1;
    };
    if(!is_allowed(name.c_str(),IS_ALLOWED_WRITE)) return 1;  /* owner of the job */
    JobId id(name); GMJob job(id, user);
    std::string controldir = getControlDir(id);
    if (controldir.empty()) {
      error_description="No control information found for this job.";
      return 1;
    }
    config.SetControlDir(controldir);
    logger.msg(Arc::INFO, "Cancelling job %s", id);
    if(job_cancel_mark_put(job,config)) {
      CommFIFO::Signal(config.ControlDir(), id);
      return 0;
    };
    error_description="Failed to cancel job.";
    return 1;
  };
  const char* logname;
  std::string id;
  bool spec_dir;
  if(!is_allowed(name.c_str(),IS_ALLOWED_WRITE,&spec_dir,&id,&logname)) return 1;
  if(logname) {
    if((*logname) != 0) return 0; /* pretend status file is deleted */
  };
  if(spec_dir) {
    error_description="Special directory can't be mangled.";
    return 1; /* can delete status directory */
  };
  std::unique_ptr<DirectUserFilePlugin> fp(makeFilePlugin(id));
  int r;
  if((getuid()==0) && config.StrictSession()) {
    SET_USER_UID(fp);
    r=fp->removefile(name);
    RESET_USER_UID;
  } else {
    r=fp->removefile(name);
  }
  if (r != 0) error_description = fp->get_error_description();
  return r;
}

int JobPlugin::removedir(std::string &dname) {
  if(!initialized) return 1;
  if(dname.find('/') == std::string::npos) { /* request to clean the job */
    if((dname == "new") || (dname == "info")) {
      error_description="Special directory can't be mangled.";
      return 1;
    };
    if(!is_allowed(dname.c_str(), IS_ALLOWED_WRITE)) return 1; /* owner of the job */
    /* check the status */
    JobId id(dname);
    std::string controldir = getControlDir(id);
    if (controldir.empty()) {
      error_description="No control information found for this job.";
      return 1;
    }
    config.SetControlDir(controldir);
    std::string sessiondir = getSessionDir(id);
    if (sessiondir.empty()) {
      // session dir could have already been cleaned, so set to first in list
      sessiondir = config.SessionRoots().at(0);
    }
    config.SetSessionRoot(sessiondir);
    job_state_t status=job_state_read_file(id,config);
    logger.msg(Arc::INFO, "Cleaning job %s", id);
    /* put marks because cleaning job may also involve removing locks */
    {
      GMJob job(id,user);
      bool res = job_cancel_mark_put(job,config);
      if(res) CommFIFO::Signal(config.ControlDir(), id);
      res &= job_clean_mark_put(job,config);
      if(res) return 0;
    };
    error_description="Failed to clean job.";
    return 1;
  };
  std::string id;
  bool spec_dir;
  if(!is_allowed(dname.c_str(),IS_ALLOWED_WRITE,&spec_dir,&id)) return 1;
  if(spec_dir) {
    error_description="Special directory can't be mangled.";
    return 1;
  };
  std::unique_ptr<DirectUserFilePlugin> fp(makeFilePlugin(id));
  int r;
  if((getuid()==0) && config.StrictSession()) {
    SET_USER_UID(fp);
    r=fp->removedir(dname);
    RESET_USER_UID;
  } else {
    r=fp->removedir(dname);
  }
  if (r != 0) error_description = fp->get_error_description();
  return r;

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
  store_job_id = "";
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
    if(!is_allowed(name,IS_ALLOWED_READ,&spec_dir,&fname,&logname)) return 1;
    std::string controldir = getControlDir(fname);
    if (controldir.empty()) {
      error_description="No control information found for this job.";
      return 1;
    }
    config.SetControlDir(controldir);
    chosenFilePlugin.reset(makeFilePlugin(fname));
    if(logname) {
      if((*logname) != 0) {
        if(strncmp(logname,"proxy",5) == 0) {
          error_description="Not allowed for this file.";
          chosenFilePlugin = NULL;
          return 1;
        };
        fname=config.ControlDir()+"/job."+fname+"."+logname;
        logger.msg(Arc::INFO, "Retrieving file %s", fname);
        return chosenFilePlugin->open_direct(fname.c_str(),mode);
      };
    };
    if(spec_dir) {
      error_description="Special directory can't be mangled.";
      return 1;
    };
    if((getuid()==0) && config.StrictSession()) {
      SET_USER_UID(chosenFilePlugin);
      int r=chosenFilePlugin->open(name,mode);
      RESET_USER_UID;
      return r;
    };
    return chosenFilePlugin->open(name,mode);
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
        config.SetControlDir(controldir);
        config.SetSessionRoot(sessiondir);
        if(job_id.length() == 0) {
          //if(readonly) {
          //  error_description="You are not allowed to submit new jobs to this service.";
          //  logger.msg(Arc::ERROR, "%s", error_description);
          //  return 1;
          //};
          if(!make_job_id()) {
            error_description="Failed to allocate ID for job.";
            logger.msg(Arc::ERROR, "%s", error_description);
            return 1;
          };
        };
        logger.msg(Arc::INFO, "Accepting submission of new job or modification request: %s", job_id);
        rsl_opened=true;
        chosenFilePlugin.reset(makeFilePlugin(job_id));
        return 0;
      };
    };
    std::string id;
    bool spec_dir;
    const char* logname;
    if(!is_allowed(name,IS_ALLOWED_WRITE,&spec_dir,&id,&logname)) return 1;
    std::string controldir = getControlDir(id);
    if (controldir.empty()) {
      std::string sessiondir;
      if (!chooseControlAndSessionDir(job_id, controldir, sessiondir)) {
        error_description="No control and/or session directory available.";
        return 1;
      }
      config.SetSessionRoot(sessiondir);
    }
    config.SetControlDir(controldir);
    chosenFilePlugin.reset(makeFilePlugin(id));
    store_job_id = id;
    logger.msg(Arc::INFO, "Storing file %s", name);
    if(spec_dir) {
      // It is allowed to modify ACL
      if(logname) {
        if(strcmp(logname,"acl") == 0) {
          std::string fname=config.ControlDir()+"/job."+id+"."+logname;
          return chosenFilePlugin->open_direct(fname.c_str(),mode);
        };
      };
      error_description="Special directory can't be mangled.";
      chosenFilePlugin = NULL;
      return 1;
    };
    if((getuid()==0) && config.StrictSession()) {
      SET_USER_UID(chosenFilePlugin);
      int r=chosenFilePlugin->open(name,mode,size);
      RESET_USER_UID;
      return r;
    };
    return chosenFilePlugin->open(name,mode,size);
  }
  logger.msg(Arc::ERROR, "Unknown open mode %i", mode);
  error_description="Unknown/unsupported request.";
  return 1;
}

int JobPlugin::close(bool eof) {
  if(!initialized || chosenFilePlugin == NULL) return 1;
  if(!rsl_opened) {
    // file transfer finished
    int r = 0;
    if((getuid()==0) && config.StrictSession()) {
      SET_USER_UID(chosenFilePlugin);
      r = chosenFilePlugin->close(eof);
      RESET_USER_UID;
    } else {
      r = chosenFilePlugin->close(eof);
    }
    if(!store_job_id.empty()) CommFIFO::Signal(config.ControlDir(), store_job_id);
    return r;
  };
  // Job description/action request transfer finished
  rsl_opened=false;
  if(job_id.length() == 0) {
    error_description="There is no job ID defined.";
    logger.msg(Arc::ERROR, "%s", error_description);
    return 1;
  };
  if(!eof) { delete_job_id(); return 0; }; /* download was canceled */
  /* *************************************************
   * store RSL (description)                         *
   ************************************************* */
  std::string rsl_fname=config.ControlDir()+"/job."+job_id+".description";
  /* analyze rsl (checking, substituting, etc)*/
  JobDescriptionHandler job_desc_handler(config);
  JobLocalDescription job_desc;
  // Initial parsing of job/action request
  JobReqResult parse_result = job_desc_handler.parse_job_req(job_id,job_desc,true);
  if (parse_result != JobReqSuccess) {
    error_description="Failed to parse job/action description.";
    logger.msg(Arc::ERROR, "%s: %s", error_description, parse_result.failure);
    delete_job_id();
    return 1;
  };
  if(job_desc.action.length() == 0) job_desc.action="request";
  if(job_desc.action == "cancel") {
    // Request to cancel existing job
    delete_job_id();
    if(job_desc.jobid.length() == 0) {
      error_description="Missing ID in request to cancel job.";
      logger.msg(Arc::ERROR, "%s", error_description);
      return 1;
    };
    // fall back to RESTful interface
    return removefile(job_desc.jobid);
  };
  if(job_desc.action == "clean") {
    // Request to remove existing job
    delete_job_id();
    if(job_desc.jobid.length() == 0) {
      error_description="Missing ID in request to clean job.";
      logger.msg(Arc::ERROR, "%s", error_description);
      return 1;
    };
    // fall back to RESTful interface
    return removedir(job_desc.jobid);
  };
  if(job_desc.action == "renew") {
    // Request to renew delegated credentials
    delete_job_id();
    if(job_desc.jobid.length() == 0) {
      error_description="Missing ID in request to renew credentials.";
      logger.msg(Arc::ERROR, "%s", error_description);
      return 1;
    };
    // fall back to RESTful interface
    return checkdir(job_desc.jobid);
  };
  if(job_desc.action == "restart") {
    // Request to restart failed job
    delete_job_id();
    if(job_desc.jobid.length() == 0) {
      error_description="Missing ID in request to restart job.";
      logger.msg(Arc::ERROR, "%s", error_description);
      return 1;
    };
    const char* logname;
    std::string id;
    if(!is_allowed(job_desc.jobid.c_str(),IS_ALLOWED_LIST,NULL,&id,&logname)) return 1;
    if(job_desc.jobid != id) {
      error_description="Wrong ID specified.";
      logger.msg(Arc::ERROR, "%s", error_description);
      return 1;
    };
    JobLocalDescription job_desc;
    if(!job_local_read_file(id,config,job_desc)) {
      error_description="Job is probably corrupted: can't read internal information.";
      logger.msg(Arc::ERROR, "%s", error_description);
      return 1;
    };
    if(job_desc.failedstate.empty()) {
      error_description="Job can't be restarted.";
      logger.msg(Arc::ERROR, "%s", error_description);
      return 1;
    };
    if(job_desc.reruns <= 0) {
      error_description="Job run out number of allowed retries.";
      logger.msg(Arc::ERROR, "%s", error_description);
      return 1;
    };
    if(!job_restart_mark_put(GMJob(id,user),config)) {
      error_description="Failed to report restart request.";
      logger.msg(Arc::ERROR, "%s", error_description);
      return 1;
    };
    CommFIFO::Signal(config.ControlDir(), id);
    return 0;
  };
  if(job_desc.action != "request") {
    delete_job_id();
    error_description="Wrong action in job RSL description.";
    logger.msg(Arc::ERROR, "%s", error_description);
    logger.msg(Arc::ERROR, "action(%s) != request", job_desc.action);
    return 1;
  };
  // Request for creating new job
  if(readonly) {
    delete_job_id();
    error_description="You are not allowed to submit new jobs to this service.";
    logger.msg(Arc::ERROR, "%s", error_description);
    return 1;
  };
  if((job_desc.jobid.length() != 0) && (job_desc.jobid != job_id)) {
    // Client may specify it's own job ID
    int h_old=::open(rsl_fname.c_str(),O_RDONLY);
    delete_job_id(); 
    if(readonly) {
      ::close(h_old);
      remove(rsl_fname.c_str());
      error_description="New jobs are not allowed.";
      logger.msg(Arc::ERROR, "%s", error_description);
      return 1;
    };
    if(!make_job_id(job_desc.jobid)) {
      ::close(h_old);
      remove(rsl_fname.c_str());
      error_description="Failed to allocate requested job ID: "+job_desc.jobid;
      logger.msg(Arc::ERROR, "%s", error_description);
      return 1;
    };
    rsl_fname=config.ControlDir()+"/job."+job_id+".description";
    {
      int l = -1;
      if(h_old != -1) {
        int h=::open(rsl_fname.c_str(),O_WRONLY,0600);
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
        logger.msg(Arc::ERROR, "Failed writing job description");
        remove(rsl_fname.c_str());
        error_description="Failed to store job description.";
        delete_job_id(); return 1;
      };
    };
  };
  // Check for proper LRMS name in request. If there is no LRMS name 
  // in user configuration that means service is opaque frontend and
  // accepts any LRMS in request.
  if((!job_desc.lrms.empty()) && (!config.DefaultLRMS().empty())) {
    if(job_desc.lrms != config.DefaultLRMS()) {
      error_description="Request for LRMS "+job_desc.lrms+" is not allowed.";
      logger.msg(Arc::ERROR, "%s", error_description);
      delete_job_id(); 
      return 1;
    };
  };
  if(job_desc.lrms.empty()) job_desc.lrms=config.DefaultLRMS();
  // Check for proper queue in request. 
  if(job_desc.queue.empty()) job_desc.queue=config.DefaultQueue();
  if(job_desc.queue.empty()) {
    error_description="Request has no queue defined.";
    logger.msg(Arc::ERROR, "%s", error_description);
    delete_job_id(); 
    return 1;
  };
  if(!avail_queues.empty()) { // If no queues configured - service takes any
    for(std::list<std::string>::iterator q = avail_queues.begin();;++q) {
      if(q == avail_queues.end()) {
        error_description="Requested queue "+job_desc.queue+" does not match any of available queues.";
        logger.msg(Arc::ERROR, "%s", error_description);
        delete_job_id(); 
        return 1;
      };
      if(*q == job_desc.queue) break; 
    };
  };
  std::list<std::pair<bool,std::string> > const& matching_groups = config.MatchingGroups(job_desc.queue.c_str());  
  if(!matching_groups.empty()) {
    // here access limit defined for requested queue - check against user group(s)
    bool allowed = false;
    for(std::list<std::pair<bool,std::string> >::const_iterator group = matching_groups.begin(); group != matching_groups.end(); ++group) {
      //if(user_a.check_group(group->second)) {
      if(user_s.user.check_group(group->second)) {
        allowed = group->first;
        break;
      };
    };
    if(!allowed) {
      error_description="Requested queue "+job_desc.queue+" is not allowed for this user";
      logger.msg(Arc::ERROR, "%s", error_description);
      delete_job_id(); 
      return 1;
    };
  };
  /* ***********************************************
   * Collect delegation identifiers                *
   *********************************************** */
  std::list<std::string> deleg_ids;
  for(std::list<FileData>::iterator f = job_desc.inputdata.begin();
                                      f != job_desc.inputdata.end();++f) {
    if(!f->cred.empty()) deleg_ids.push_back(f->cred);
  };
  for(std::list<FileData>::iterator f = job_desc.outputdata.begin();
                                      f != job_desc.outputdata.end();++f) {
    if(!f->cred.empty()) deleg_ids.push_back(f->cred);
  };
  /* ****************************************
   * Preprocess job request                 *
   **************************************** */
  std::string session_dir(config.SessionRoot(job_id) + '/' + job_id);
  GMJob job(job_id,user,session_dir,JOB_STATE_ACCEPTED);
  if(!job_desc_handler.process_job_req(job, job_desc)) {
    error_description="Failed to preprocess job description.";
    logger.msg(Arc::ERROR, "%s", error_description);
    delete_job_id(); 
    return 1;
  };
  // Also pick up global delegation id if any
  if(!job_desc.delegationid.empty()) {
    deleg_ids.push_back(job_desc.delegationid);
  };
  /* ****************************************
   * Start local file                       *
   **************************************** */
  /* !!!!! some parameters are unchecked here - rerun,diskspace !!!!! */
  job_desc.jobid=job_id;
  job_desc.starttime=time(NULL);
  job_desc.DN=subject;
  job_desc.sessiondir=config.SessionRoot(job_id)+'/'+job_id;
  if(port != 0) {
    job_desc.clientname=
       Arc::tostring(host[0])+"."+Arc::tostring(host[1])+"."+
       Arc::tostring(host[2])+"."+Arc::tostring(host[3])+":"+
       Arc::tostring(port);
  };
  std::string globalid = endpoint;
  if(!globalid.empty()) {
    if(globalid[globalid.length()-1] != '/') globalid+="/";
    globalid+=job_id;
    job_desc.globalid=globalid;
  };
  job_desc.headnode=endpoint;
  job_desc.interface="org.nordugrid.gridftpjob";
  if(matched_vo != NULL) {
    job_desc.localvo.push_back(matched_vo);
  };
  if(matched_voms != NULL) {
    for(std::vector<voms_fqan_t>::const_iterator f = matched_voms->fqans.begin();
                           f != matched_voms->fqans.end(); ++f) {
      std::string fqan;
      f->str(fqan);
      job_desc.voms.push_back(fqan);
    };
  };
  // If no authorized VOMS was identified just report those from credentials
  if(job_desc.voms.empty()) {
    //const std::vector<struct voms_t>& all_voms = user_a.voms();
    const std::vector<struct voms_t>& all_voms = user_s.user.voms();
    for(std::vector<struct voms_t>::const_iterator v = all_voms.begin();
                             v != all_voms.end(); ++v) {
      for(std::vector<voms_fqan_t>::const_iterator f = v->fqans.begin();
                             f != v->fqans.end(); ++f) {
        std::string fqan;
        f->str(fqan);
        job_desc.voms.push_back(fqan);
      };
    };
  };
  // If still no VOMS information is available take forced one from configuration
  if(job_desc.voms.empty()) {
    std::string forced_voms = config.ForcedVOMS(job_desc.queue.c_str());
    if(forced_voms.empty()) forced_voms = config.ForcedVOMS();
    if(!forced_voms.empty()) job_desc.voms.push_back(forced_voms);
  };

  /* ***********************************************
   * Try to create delegation and proxy file       *
   *********************************************** */
  if(!proxy_fname.empty()) {
    Arc::Credential cred(proxy_fname, proxy_fname, config.CertDir(), "");
    using namespace ArcCredential; // needed for macro expansion
    if (!CERT_IS_RFC_PROXY(cred.GetType())) {
      error_description="Non-RFC proxies are not supported.";
      logger.msg(Arc::ERROR, "%s", error_description);
      delete_job_id();
      return 1;
    }
    std::string proxy_data;
    (void)Arc::FileRead(proxy_fname, proxy_data);
    if(!proxy_data.empty()) {
      if(proxy_is_deleg && job_desc.delegationid.empty()) {
        // If we have gridftp delegation and no other generic delegation provided - store it
        ARex::DelegationStore deleg(config.DelegationDir(), deleg_db_type, false);
        std::string deleg_id;
        if(!deleg.AddCred(deleg_id, subject, proxy_data)) {
          error_description="Failed to store delegation.";
          logger.msg(Arc::ERROR, "%s", error_description);
          delete_job_id(); 
          return 1;
        };
        job_desc.delegationid = deleg_id;
        deleg_ids.push_back(deleg_id); // one more delegation id
      };
#if 0
      // And store public credentials into proxy file
      try {
        Arc::Credential ci(proxy_fname, proxy_fname, config.CertDir(), "");
        job_desc.expiretime = ci.GetEndTime();
        std::string user_cert;
        ci.OutputCertificate(user_cert);
        ci.OutputCertificateChain(user_cert);
        if(!job_proxy_write_file(job,config,user_cert)) {
          error_description="Failed to store user credentials.";
          logger.msg(Arc::ERROR, "%s", error_description);
          delete_job_id(); 
          return 1;
        };
      } catch (std::exception&) {
        job_desc.expiretime = time(NULL);
      };
#else
      // For backward compatibility during conversion time
      // store full proxy into proxy file
      if(!job_proxy_write_file(job,config,proxy_data)) {
        error_description="Failed to store user credentials.";
        logger.msg(Arc::ERROR, "%s", error_description);
        delete_job_id(); 
        return 1;
      };
      job_desc.expiretime = time(NULL);
      if(proxy_is_deleg) {
        try {
          Arc::Credential ci(proxy_data, "", config.CertDir(), "", "", false);
          job_desc.expiretime = ci.GetEndTime();
        } catch (std::exception&) {
        };
      };
#endif
    };
  }
  /* ******************************************
   * Write local file                         *
   ****************************************** */
  if(!job_local_write_file(job,config,job_desc)) {
    logger.msg(Arc::ERROR, "Failed writing local description");
    delete_job_id(); 
    error_description="Failed to create job description.";
    return 1;
  };
  /* ******************************************
   * Write access policy                      *
   ****************************************** */
  if(!parse_result.acl.empty()) {
    if(!job_acl_write_file(job_id,config,parse_result.acl)) {
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
  if(cont_plugins) cont_plugins->run(job,config,results);
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
  /* *******************************************
   * Create session directory                  *
   ******************************************* */
  if (!config.CreateSessionDirectory(job.SessionDir(), job.get_user())) {
    logger.msg(Arc::ERROR, "Failed to create session directory %s", job.SessionDir());
    delete_job_id();
    error_description="Failed to create session directory.";
    return 1;
  }

  /* **********************************************************
   * Create status file (do it last so GM picks job up here)  *
   ********************************************************** */
  if(!job_state_write_file(job,config,JOB_STATE_ACCEPTED)) {
    logger.msg(Arc::ERROR, "Failed writing status");
    delete_job_id(); 
    error_description="Failed registering job in A-REX.";
    return 1;
  };

  // Put lock on delegated credentials
  // Can do that after creating status file because delegations are 
  // fresh and hence won't be deleted while locking.
  if(!deleg_ids.empty()) {
    deleg_ids.sort();
    deleg_ids.unique();
    ARex::DelegationStore store(config.DelegationDir(),deleg_db_type,false);
    if(!store.LockCred(job_id,deleg_ids,subject)) {
      logger.msg(Arc::ERROR, "Failed to lock delegated credentials: %s", store.GetFailure());
      delete_job_id();
      error_description="Failed to lock delegated credentials.";
      return 1;
    };
  }

  CommFIFO::Signal(config.ControlDir(), job_id);
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
  if((getuid()==0) && config.StrictSession()) {
    SET_USER_UID(chosenFilePlugin);
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
    if((getuid()==0) && config.StrictSession()) {
      SET_USER_UID(chosenFilePlugin);
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
  std::string rsl_fname=config.ControlDir()+"/job."+job_id+".description";
  int h=::open(rsl_fname.c_str(),O_WRONLY|O_CREAT,0600);
  if(h == -1) {
    error_description="Failed to open job description file " + rsl_fname;
    return 1;
  };
  if(::lseek(h,offset,SEEK_SET) != offset) {
    ::close(h);
    error_description="Failed to seek in job description file " + rsl_fname;
    return 1;
  };
  for(;size;) {
    ssize_t l = ::write(h,buf,size);
    if(l <= 0) {
      ::close(h);
      error_description="Failed to write job description file " + rsl_fname;
      return 1;
    };
    size-=l; buf+=l;
  };
  fix_file_owner(rsl_fname,user);
  ::close(h);
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
    std::string cdir=control_dir;
    Glib::Dir *dir=new Glib::Dir(cdir);
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
              dir_list.push_back(DirEntry(false,id));
            };
          };
        };
      };
      dir->close();
      delete dir;
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
  if(!is_allowed(name,IS_ALLOWED_LIST,NULL,&id,&logname,&log)) return 1;
  if(logname) {
    std::string controldir = getControlDir(id);
    if (controldir.empty()) {
      error_description="No control information found for this job.";
      return 1;
    }
    config.SetControlDir(controldir);
    if((*logname) != 0) {
      if(strchr(logname,'/') != NULL) return 1; /* no subdirs */
      if(strncmp(logname,"proxy",5) == 0) return 1;
      id=config.ControlDir()+"/job."+id+"."+logname;
      struct stat st;
      if(::stat(id.c_str(),&st) != 0) return 1;
      if(!S_ISREG(st.st_mode)) return 1;
      DirEntry dent(true,logname);
      if(strncmp(logname,"proxy",5) != 0) dent.may_read=true;
      dir_list.push_back(dent);
      return -1;
    };
    Glib::Dir* d=new Glib::Dir(config.ControlDir());
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
    delete d;
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
  chosenFilePlugin.reset(makeFilePlugin(id));
  if((getuid()==0) && config.StrictSession()) {
    SET_USER_UID(chosenFilePlugin);
    int r=chosenFilePlugin->readdir(name,dir_list,mode);
    RESET_USER_UID;
    return r;
  };
  return chosenFilePlugin->readdir(name,dir_list,mode);
}

int JobPlugin::checkdir(std::string &dirname) {
  if(!initialized) return 1;
  /* chdir to /new will create new job */
  if(dirname.length() == 0) return 0; /* root */
  if(dirname == "new") { /* new job */
    if(readonly) {
      error_description="New jobs are not allowed.";
      logger.msg(Arc::ERROR, "%s", error_description);
      return 1;
    };
    if(!make_job_id()) {
      error_description="Failed to allocate ID for job.";
      logger.msg(Arc::ERROR, "%s", error_description);
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
  if(!is_allowed(dirname.c_str(),IS_ALLOWED_LIST,NULL,&id,&logname)) return 1;
  std::string controldir = getControlDir(id);
  if (controldir.empty()) {
    error_description="No control information found for this job.";
    return 1;
  }
  config.SetControlDir(controldir);
  if(logname) {
    if((*logname) != 0) {
      error_description="There is no such special subdirectory.";
      return 1; /* log directory has no subdirs */
    };
    return 0;
  };
  if((dirname == id) && (!proxy_fname.empty()) && proxy_is_deleg) {  /* cd to session directory - renew proxy request */
    JobLocalDescription job_desc;
    if(!job_local_read_file(id,config,job_desc)) {
      error_description="Job is probably corrupted: can't read internal information.";
      logger.msg(Arc::ERROR, "%s", error_description);
      return 1;
    };
    /* check if new proxy is better than old one */
    std::string old_proxy_fname=config.ControlDir()+"/job."+id+".proxy";
    Arc::Time new_proxy_expires;
    Arc::Time old_proxy_expires;
    std::string proxy_data;
    std::string user_cert;
    try {
      (void)Arc::FileRead(proxy_fname, proxy_data);
      if(proxy_data.empty()) {
        error_description="Failed to obtain delegation content.";
        logger.msg(Arc::ERROR, "%s", error_description);
        return 1;
      };
      Arc::Credential new_ci(proxy_data, proxy_data, config.CertDir(), "", "", false);
      new_proxy_expires = new_ci.GetEndTime();
      new_ci.OutputCertificate(user_cert);
      new_ci.OutputCertificateChain(user_cert);
    } catch (std::exception&) { };
    try {
      Arc::Credential old_ci(old_proxy_fname, "", config.CertDir(), "");
      old_proxy_expires = old_ci.GetEndTime();
    } catch (std::exception&) { };
    if(new_proxy_expires > old_proxy_expires) {
      /* try to renew proxy */
      logger.msg(Arc::INFO, "Renewing proxy for job %s", id);
      ARex::DelegationStore deleg(config.DelegationDir(),deleg_db_type,false);
      if((!job_desc.delegationid.empty()) && deleg.PutCred(job_desc.delegationid, subject, proxy_data)) {
        // Also store public content into job.#.proxy
        // Ignore error because main store is already updated
        GMJob job(id, user, "", JOB_STATE_ACCEPTED);
#if 0
        (void)job_proxy_write_file(job, config, user_cert);
#else
        // For backward compatibility during transitional period store whole proxy
        (void)job_proxy_write_file(job, config, proxy_data);
#endif
        logger.msg(Arc::INFO, "New proxy expires at %s", Arc::TimeStamp(Arc::Time(new_proxy_expires), Arc::UserTime));
        job_desc.expiretime=new_proxy_expires;
        if(!job_local_write_file(job,config,job_desc)) {
          logger.msg(Arc::ERROR, "Failed to write 'local' information");
        };
      } else {
        logger.msg(Arc::ERROR, "Failed to renew proxy");
      };
    } else {
      logger.msg(Arc::WARNING, "New proxy expiry time is not later than old proxy, not renewing proxy");
    };
  };
  chosenFilePlugin.reset(makeFilePlugin(id));
  if((getuid()==0) && config.StrictSession()) {
    SET_USER_UID(chosenFilePlugin);
    int r=chosenFilePlugin->checkdir(dirname);
    RESET_USER_UID;
    return r;
  };
  return chosenFilePlugin->checkdir(dirname);
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
  if(!is_allowed(name.c_str(),IS_ALLOWED_LIST,NULL,&id,&logname)) return 1;
  std::string controldir = getControlDir(id);
  if (controldir.empty()) {
    error_description="No control information found for this job.";
    return 1;
  }
  config.SetControlDir(controldir);
  if(logname) {
    if((*logname) == 0) { /* directory itself */
      info.is_file=false; info.name=""; info.may_dirlist=true;
    }
    else {
      if(strncmp(logname,"proxy",5) == 0) {
        error_description="There is no such special file.";
        return 1;
      };
      id=config.ControlDir()+"/job."+id+"."+logname;
      logger.msg(Arc::INFO, "Checking file %s", id);
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
  chosenFilePlugin.reset(makeFilePlugin(id));
  if((getuid()==0) && config.StrictSession()) {
    SET_USER_UID(chosenFilePlugin);
    int r=chosenFilePlugin->checkfile(name,info,mode);
    RESET_USER_UID;
    return r;
  };
  return chosenFilePlugin->checkfile(name,info,mode);
}

bool JobPlugin::delete_job_id(void) {
  if(job_id.length() != 0) {
    std::string controldir = getControlDir(job_id);
    if (controldir.empty()) {
      error_description="No control information found for this job.";
      return false;
    }    
    config.SetControlDir(controldir);
    std::string sessiondir = getSessionDir(job_id);
    if (sessiondir.empty()) {
      // session dir could have already been cleaned, so set to first in list
      sessiondir = config.SessionRoots().at(0);
    }    
    config.SetSessionRoot(sessiondir);
    job_clean_final(GMJob(job_id,user,sessiondir+"/"+job_id),config);
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
  // So far assume control directory is on local fs.
  // TODO: add locks or links for NFS
  // check the new ID is not used in any control dir
  std::string fname=control_dir+"/job."+id+".description";
  int h = ::open(fname.c_str(),O_RDWR | O_CREAT | O_EXCL,0600);
  if(h == -1)
    return false;
  fix_file_owner(fname,user);
  close(h);
  delete_job_id();
  job_id=id;
  return true;
}

bool JobPlugin::make_job_id(void) {
  bool found = false;
  delete_job_id();
  for(int i=0;i<100;i++) {
    //std::string id=Arc::tostring((unsigned int)getpid())+
    //               Arc::tostring((unsigned int)time(NULL))+
    //               Arc::tostring(rand(),1);
    std::string id;
    Arc::GUID(id);
    // create job.id.description file
    std::string fname=control_dir+"/job."+id+".description";
    int h = ::open(fname.c_str(),O_RDWR | O_CREAT | O_EXCL,0600);
    // So far assume control directory is on local fs.
    // TODO: add locks or links for NFS
    if(h == -1) {
      if(errno == EEXIST) continue;
      logger.msg(Arc::ERROR, "Failed to create file in %s", control_dir);
      return false;
    };
    // safe to use this id
    job_id = id;
    fix_file_owner(fname,user);
    close(h);
    break;
  };
  if(job_id.empty()) {
    logger.msg(Arc::ERROR, "Out of tries while allocating new job ID");
    return false;
  };
  return true;
}

/*
  name - name of file to access
  perm - permission to check
  jobid - returns id extracted from name
  logname - name of log file (errors, status, etc.)
  log - stdlog of job
  spec_dir - if file belogs to virtual directory 
  returns true if access rights include the specified permission. For special
  files true is returned and spec_dir is set to true. Distinction between
  files is processed at higher levels.
  In case of error, error_description is set.
*/
bool JobPlugin::is_allowed(const char* name,int perm,bool* spec_dir,std::string* jobid,char const ** logname,std::string* log) {
  if(logname) (*logname) = NULL;
  if(log) (*log)="";
  if(spec_dir) (*spec_dir)=false;
  JobId id(name);
  if(id == "info") { // directory which contains list of jobs-directories
    if(spec_dir) (*spec_dir)=false;
    if((perm & (IS_ALLOWED_READ | IS_ALLOWED_LIST)) == perm) return true;
    error_description = "Not allowed for this job: permission denied";
    return false;
  };
  if(strncmp(id.c_str(),"info/",5) == 0) {
    if(spec_dir) (*spec_dir)=true;
    name+=5; id=name;
    std::string::size_type n=id.find('/'); if(n != std::string::npos) id.erase(n);
    if(jobid) (*jobid)=id;
    if(id.length() == 0) {
      error_description = "No job id found";
      return false;
    }
    const char* l_name = name+id.length();
    if(l_name[0] == '/') l_name++;
    if(logname) { (*logname)=l_name; };
    JobLocalDescription job_desc;
    std::string controldir = getControlDir(id);
    if (controldir.empty()) {
      error_description="No control information found for this job.";
      return false;
    }    
    config.SetControlDir(controldir);
    if(!job_local_read_file(id,config,job_desc)) {
      error_description = "Not allowed for this job: "+Arc::StrError(errno);
      return false;
    }
    if(job_desc.DN != subject) {
      // Not an owner. Check acl.
      std::string acl_file = config.ControlDir()+"/job."+id+".acl";
      struct stat st;
      if(stat(acl_file.c_str(),&st) == 0) {
        if(S_ISREG(st.st_mode)) {
          int res = 0;
          res |= check_acl(acl_file.c_str(), true, id);
          if ( (res & perm) == perm) return true;
          error_description = "Not allowed for this job: permission denied";
        };
      };
      return false;
    };
    //if(strncmp(l_name,"proxy",5) == 0) return (IS_ALLOWED_LIST);
    //if(strncmp(l_name,"acl",3) != 0) return (IS_ALLOWED_READ | IS_ALLOWED_LIST);;
    return true;
  };
  std::string::size_type n=id.find('/'); if(n != std::string::npos) id.erase(n);
  if(jobid) (*jobid)=id;
  JobLocalDescription job_desc;
  std::string controldir = getControlDir(id);
  if (controldir.empty()) {
    error_description="No control information found for this job.";
    return false;
  }    
  config.SetControlDir(controldir);
  if(!job_local_read_file(id,config,job_desc)) {
    logger.msg(Arc::ERROR, "Failed to read job's local description for job %s from %s", id, config.ControlDir());
    if (errno == ENOENT) error_description="No such job";
    else error_description=Arc::StrError(errno);
    return false;
  }
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
    std::string acl_file = config.ControlDir()+"/job."+id+".acl";
    struct stat st;
    if(stat(acl_file.c_str(),&st) == 0) {
      if(S_ISREG(st.st_mode)) {
        res |= check_acl(acl_file.c_str(), spec, id);
      };
    };
  };
  if ((res & perm) == perm) return true;
  error_description="Not allowed for this job: permission denied";
  return false;
}

/*
std::string JobPlugin::getDefaultSessionDir() {
  // if multiple session dirs are defined, don't use remote dirs
  if (session_dirs.size() > 1) {
    return session_dirs.at(0);
  }
  else if(gm_dirs_info.size() > 0) {
    return gm_dirs_info.at(0).session_dir;
  }
}
*/

/*
 * Methods to deal with multple control and session dirs
 */
DirectUserFilePlugin * JobPlugin::makeFilePlugin(std::string id) {
  // get session dir
  uid_t suid = 0;
  gid_t sgid = 0;
  std::string sd = getSessionDir(id, &suid, &sgid);
  if (sd.empty()) {
    // This id is not in any of sessiondir 
    // Shouldn't be possible but use first in list
    sd = session_dirs.at(0);
    suid = user.get_uid();
    sgid = user.get_gid();
  }
  return new DirectUserFilePlugin(sd, suid, sgid, user_s);
}

bool JobPlugin::chooseControlAndSessionDir(std::string /* job_id */, std::string& controldir, std::string& sessiondir) {

  if (session_dirs_non_draining.empty()) {
    // no active control or session dirs available
    logger.msg(Arc::ERROR, "No non-draining session directories available");
    return false;
  }
  controldir = control_dir;
  // choose randomly from non-draining session dirs
  sessiondir = session_dirs_non_draining.at(rand() % session_dirs_non_draining.size());
  logger.msg(Arc::INFO, "Using control directory %s", controldir);
  logger.msg(Arc::INFO, "Using session directory %s", sessiondir);
  return true;
}  
  

std::string JobPlugin::getControlDir(std::string id) {
  return control_dir;
}

std::string JobPlugin::getSessionDir(std::string const& id, uid_t* uid, gid_t* gid) {

  // look for this id's session dir
  struct stat st;
  for (unsigned int i = 0; i < session_dirs.size(); i++) {
    std::string sessiondir(session_dirs.at(i) + '/' + id);
    if (::stat(sessiondir.c_str(), &st) == 0 && S_ISDIR(st.st_mode)) {
      if(uid) *uid = st.st_uid;
      if(gid) *gid = st.st_gid;
      return session_dirs.at(i);
    }
  }
  // no session dir found
  if(uid) *uid = 0;
  if(gid) *gid = 0;
  return "";
}
