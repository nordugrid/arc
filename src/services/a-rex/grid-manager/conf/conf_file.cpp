#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include <pwd.h>

#include <arc/StringConv.h>
#include "../jobs/users.h"
#include "../jobs/states.h"
#include "../jobs/plugins.h"
#include "conf.h"
#include "conf_sections.h"
#include "environment.h"
#include "gridmap.h"
#include "../run/run_plugin.h"
#include "conf_file.h"

#ifndef HAVE_SETENV
#include <glibmm/miscutils.h>
#define setenv Glib::setenv
#endif

static Arc::Logger& logger = Arc::Logger::getRootLogger();

JobLog job_log;
ContinuationPlugins plugins;
RunPlugin cred_plugin;

bool configure_serviced_users(JobUsers &users,uid_t my_uid,const std::string &my_username,JobUser &my_user,Daemon* daemon) {
  std::ifstream cfile;
  std::string session_root("");
  std::string cache_dir("");
  std::string cache_data_dir("");
  std::string cache_link_dir("");
  bool private_cache = false;
  std::string default_lrms("");
  std::string default_queue("");
  std::string default_reruns_s("");
  std::string default_diskspace_s("");
  std::string last_control_dir("");
  time_t default_ttl = DEFAULT_KEEP_FINISHED;
  time_t default_ttr = DEFAULT_KEEP_DELETED;
  int default_reruns = DEFAULT_JOB_RERUNS;
  int default_diskspace = DEFAULT_DISKSPACE;
  int max_jobs = -1;
  int max_jobs_processing = -1;
  int max_jobs_processing_emergency = -1;
  int max_jobs_running = -1;
  int max_downloads = -1;
  unsigned long long int min_speed;
  time_t min_speed_time;
  unsigned long long int min_average_speed;
  time_t max_inactivity_time;
  bool use_secure_transfer = true;
  bool use_local_transfer = false;
  bool superuser = (my_uid == 0);
  long long int cache_max = 0;
  long long int cache_min = 0;
  bool cache_registration = false;
  bool strict_session = false;
  unsigned int wakeup_period;
  std::string central_control_dir("");
  ConfigSections* cf = NULL;
  std::string infosys_user("");

  /* read configuration and add users and other things */
  if(!config_open(cfile)) {
    logger.msg(Arc::ERROR,"Can't read configuration file."); return false;
  };
  if(central_configuration) {
    cf=new ConfigSections(cfile);
    cf->AddSection("common");
    cf->AddSection("grid-manager");
    cf->AddSection("infosys");
  };
  /* process configuration information here */
  for(;;) {
    std::string rest;
    std::string command;
    if(central_configuration) {
      cf->ReadNext(command,rest);
      if(cf->SectionNum() == 2) { // infosys - looking for user name only
        if(command == "user") infosys_user=rest;
        continue;
      };
      if(cf->SectionNum() == 0) { // infosys user may be in common too
        if(command == "user") infosys_user=rest;
      };
    } else {
      command = config_read_line(cfile,rest);
    };
    if(daemon) {
      int r = daemon->config(command,rest);
      if(r == 0) continue;
      if(r == -1) goto exit;
    } else {
      int r = Daemon::skip_config(command);
      if(r == 0) continue;
    };
    if(command.length() == 0) {
      if(central_control_dir.length() != 0) {
        command="control"; rest=central_control_dir+" .";
        central_control_dir="";
      } else {
        break;
      };
    };
    // pbs,gnu_time,etc. it is ugly hack here
    if(central_configuration && (command == "pbs_bin_path")) {
      setenv("PBS_BIN_PATH",rest.c_str(),1);
    } else if(central_configuration && (command == "pbs_log_path")) {
      setenv("PBS_LOG_PATH",rest.c_str(),1);
    } else if(central_configuration && (command == "gnu_time")) {
      setenv("GNU_TIME",rest.c_str(),1);
    } else if(central_configuration && (command == "tmpdir")) {
      setenv("TMP_DIR",rest.c_str(),1);
    } else if(central_configuration && (command == "runtimedir")) {
      setenv("RUNTIME_CONFIG_DIR",rest.c_str(),1);
    } else if(central_configuration && (command == "shared_filesystem")) {
      if(rest == "NO") rest="no";
      setenv("RUNTIME_NODE_SEES_FRONTEND",rest.c_str(),1);
    } else if(central_configuration && (command == "scratchdir")) {
      setenv("RUNTIME_LOCAL_SCRATCH_DIR",rest.c_str(),1);
    } else if(central_configuration && (command == "shared_scratch")) {
      setenv("RUNTIME_FRONTEND_SEES_NODE",rest.c_str(),1);
    } else if(central_configuration && (command == "nodename")) {
      setenv("NODENAME",rest.c_str(),1);
    } else if((!central_configuration && (command == "cache")) || 
              (central_configuration  && (command == "cachedir"))) { 
      cache_dir = config_next_arg(rest);
      cache_link_dir = config_next_arg(rest);
      if(rest.length() != 0) {
        logger.msg(Arc::WARNING,"junk in cache command"); goto exit;
      };
      private_cache=false;
    }
    else if(command == "privatecache") { 
      cache_dir = config_next_arg(rest);
      cache_link_dir = config_next_arg(rest);
      if(rest.length() != 0) {
        logger.msg(Arc::WARNING,"junk in privatecache command"); goto exit;
      };
      private_cache=true;
    }
    else if(command == "cachedata") {
      cache_data_dir = config_next_arg(rest);
      if(rest.length() != 0) {
        logger.msg(Arc::WARNING,"junk in cachedata command"); goto exit;
      };
    }
    else if(command == "cachesize") {
      long long int i;
      std::string cache_size_s = config_next_arg(rest);
      if(cache_size_s.length() == 0) {
        i=0;
      }
      else {
        if(!Arc::stringto(cache_size_s,i)) {
          logger.msg(Arc::ERROR,"wrong number in cachesize"); goto exit;
        };
      };
      cache_max=i;
      cache_size_s = config_next_arg(rest);
      if(cache_size_s.length() == 0) {
        i=cache_max;
      }
      else {
        if(!Arc::stringto(cache_size_s,i)) {
          logger.msg(Arc::ERROR,"wrong number in cachesize"); goto exit;
        };
      };
      cache_min=i;
    }
    else if(command == "joblog") { /* where to write job inforamtion */ 
      std::string fname = config_next_arg(rest);  /* empty is allowed too */
      job_log.SetOutput(fname.c_str());
    }
    else if(command == "jobreport") { /* service to report information to */ 
      for(;;) {
        std::string url = config_next_arg(rest); 
        if(url.length() == 0) break;
        unsigned int i;
        if(Arc::stringto(url,i)) {
          job_log.SetExpiration(i);
          continue;
        };
        job_log.SetReporter(url.c_str());
      };
    }
    else if(command == "maxjobs") { /* maximum number of the jobs to support */ 
      std::string max_jobs_s = config_next_arg(rest);
      long int i;
      max_jobs = -1; max_jobs_processing = -1; max_jobs_running = -1;
      if(max_jobs_s.length() != 0) {
        if(!Arc::stringto(max_jobs_s,i)) {
          logger.msg(Arc::ERROR,"wrong number in maxjobs"); goto exit;
        };
        if(i<0) i=-1; max_jobs=i;
      };
      max_jobs_s = config_next_arg(rest);
      if(max_jobs_s.length() != 0) {
        if(!Arc::stringto(max_jobs_s,i)) {
          logger.msg(Arc::ERROR,"wrong number in maxjobs"); goto exit;
        };
        if(i<0) i=-1; max_jobs_running=i;
      };
      JobsList::SetMaxJobs(
              max_jobs,max_jobs_running);
    }
    else if(command == "maxload") { /* maximum number of the jobs processed on frontend */
      std::string max_jobs_s = config_next_arg(rest);
      long int i;
      max_jobs_processing=-1;
      max_jobs_processing_emergency=-1;
      max_downloads = -1;
      if(max_jobs_s.length() != 0) {
        if(!Arc::stringto(max_jobs_s,i)) {
          logger.msg(Arc::ERROR,"wrong number in maxload"); goto exit;
        };
        if(i<0) i=-1; max_jobs_processing=i;
      };
      max_jobs_s = config_next_arg(rest);
      if(max_jobs_s.length() != 0) {
        if(!Arc::stringto(max_jobs_s,i)) {
          logger.msg(Arc::ERROR,"wrong number in maxload"); goto exit;
        };
        if(i<0) i=-1; max_jobs_processing_emergency=i;
      };
      max_jobs_s = config_next_arg(rest);
      if(max_jobs_s.length() != 0) {
        if(!Arc::stringto(max_jobs_s,i)) {
          logger.msg(Arc::ERROR,"wrong number in maxload"); goto exit;
        };
        if(i<0) i=-1; max_downloads=i;
      };
      JobsList::SetMaxJobsLoad(
              max_jobs_processing,max_jobs_processing_emergency,max_downloads);
    }
    else if(command == "speedcontrol") {  
      std::string speed_s = config_next_arg(rest);
      min_speed=0; min_speed_time=300;
      min_average_speed=0; max_inactivity_time=300;
      if(speed_s.length() != 0) {
        if(!Arc::stringto(speed_s,min_speed)) {
          logger.msg(Arc::ERROR,"wrong number in speedcontrol"); goto exit;
        };
      };
      speed_s = config_next_arg(rest);
      if(speed_s.length() != 0) {
        if(!Arc::stringto(speed_s,min_speed_time)) {
          logger.msg(Arc::ERROR,"wrong number in speedcontrol"); goto exit;
        };
      };
      speed_s = config_next_arg(rest);
      if(speed_s.length() != 0) {
        if(!Arc::stringto(speed_s,min_average_speed)) {
          logger.msg(Arc::ERROR,"wrong number in speedcontrol"); goto exit;
        };
      };
      speed_s = config_next_arg(rest);
      if(speed_s.length() != 0) {
        if(!Arc::stringto(speed_s,max_inactivity_time)) {
          logger.msg(Arc::ERROR,"wrong number in speedcontrol"); goto exit;
        };
      };
      JobsList::SetSpeedControl(
              min_speed,min_speed_time,min_average_speed,max_inactivity_time);
    }
    else if(command == "wakeupperiod") { 
      std::string wakeup_s = config_next_arg(rest);
      if(wakeup_s.length() != 0) {
        if(!Arc::stringto(wakeup_s,wakeup_period)) {
          logger.msg(Arc::ERROR,"wrong number in wakeupperiod"); goto exit;
        };
        JobsList::SetWakeupPeriod(wakeup_period);
      };
    }
    else if(command == "securetransfer") {
      std::string s = config_next_arg(rest);
      if(strcasecmp("yes",s.c_str()) == 0) {
        use_secure_transfer=true;
      } 
      else if(strcasecmp("no",s.c_str()) == 0) {
        use_secure_transfer=false;
      }
      else {
        logger.msg(Arc::ERROR,"wrong option in securetransfer"); goto exit;
      };
      JobsList::SetSecureTransfer(use_secure_transfer);
    }
    else if(command == "norootpower") {
      std::string s = config_next_arg(rest);
      if(strcasecmp("yes",s.c_str()) == 0) {
        strict_session=true;
      }
      else if(strcasecmp("no",s.c_str()) == 0) {
        strict_session=false;
      }
      else {
        logger.msg(Arc::ERROR,"wrong option in norootpower"); goto exit;
      };
    }
    else if(command == "localtransfer") {
      std::string s = config_next_arg(rest);
      if(strcasecmp("yes",s.c_str()) == 0) {
        use_local_transfer=true;
      }
      else if(strcasecmp("no",s.c_str()) == 0) {
        use_local_transfer=false;
      }
      else {
        logger.msg(Arc::ERROR,"wrong option in localtransfer"); goto exit;
      };
      JobsList::SetLocalTransfer(use_local_transfer);
    }
    else if(command == "cacheregistration") {
      std::string s = config_next_arg(rest);
      if(strcasecmp("yes",s.c_str()) == 0) {
        cache_registration=true;
      }
      else if(strcasecmp("no",s.c_str()) == 0) {
        cache_registration=false;
      }
      else {
        logger.msg(Arc::ERROR,"wrong option in cacheregistration"); goto exit;
      };
      JobsList::SetCacheRegistration(cache_registration);
    }
    else if(command == "mail") { /* internal address from which to send mail */ 
      support_mail_address = config_next_arg(rest);
      if(support_mail_address.length() == 0) {
        logger.msg(Arc::ERROR,"mail is empty"); goto exit;
      };
    }
    else if(command == "defaultttl") { /* time to keep job after finished */ 
      char *ep;
      std::string default_ttl_s = config_next_arg(rest);
      if(default_ttl_s.length() == 0) {
        logger.msg(Arc::ERROR,"defaultttl is empty"); goto exit;
      };
      default_ttl=strtoul(default_ttl_s.c_str(),&ep,10);
      if(*ep != 0) {
        logger.msg(Arc::ERROR,"wrong number in defaultttl command"); goto exit;
      };
      default_ttl_s = config_next_arg(rest);
      if(default_ttl_s.length() != 0) {
        if(rest.length() != 0) {
          logger.msg(Arc::ERROR,"junk in defaultttl command"); goto exit;
        };
        default_ttr=strtoul(default_ttl_s.c_str(),&ep,10);
        if(*ep != 0) {
          logger.msg(Arc::ERROR,"wrong number in defaultttl command"); goto exit;
        };
      } else {
        default_ttr=DEFAULT_KEEP_DELETED;
      };
    }
    else if(command == "maxrerun") { /* number of retries allowed */ 
      default_reruns_s = config_next_arg(rest);
      if(default_reruns_s.length() == 0) {
        logger.msg(Arc::ERROR,"maxrerun is empty"); goto exit;
      };
      if(rest.length() != 0) {
        logger.msg(Arc::ERROR,"junk in maxrerun command"); goto exit;
      };
      char *ep;
      default_reruns=strtoul(default_reruns_s.c_str(),&ep,10);
      if(*ep != 0) {
        logger.msg(Arc::ERROR,"wrong number in maxrerun command"); goto exit;
      };      
    }
    else if(command == "diskspace") { /* maximal amount of disk space */ 
      default_diskspace_s = config_next_arg(rest);
      if(default_diskspace_s.length() == 0) {
        logger.msg(Arc::ERROR,"diskspace is empty"); goto exit;
      };
      if(rest.length() != 0) {
        logger.msg(Arc::ERROR,"junk in diskspace command"); goto exit;
      };
      char *ep;
      default_diskspace=strtoull(default_diskspace_s.c_str(),&ep,10);
      if(*ep != 0) {
        logger.msg(Arc::ERROR,"wrong number in diskspace command"); goto exit;
      };      
    }
    else if((!central_configuration && (command == "defaultlrms")) || 
            (central_configuration  && (command == "lrms"))) {
                                     /* set default lrms type and queue 
                                     (optionally). Applied to all following
                                     'control' commands. MUST BE thing */
      default_lrms = config_next_arg(rest);
      default_queue = config_next_arg(rest);
      if(default_lrms.length() == 0) {
        logger.msg(Arc::ERROR,"defaultlrms is empty"); goto exit;
      };
      if(rest.length() != 0) {
        logger.msg(Arc::ERROR,"junk in defaultlrms command"); goto exit;
      };
    }
    else if(command == "authplugin") { /* set plugin to be called on 
                                          state changes */
      std::string state_name = config_next_arg(rest);
      if(state_name.length() == 0) {
        logger.msg(Arc::ERROR,"state name for plugin is missing"); goto exit;
      };
      std::string options_s = config_next_arg(rest);
      if(options_s.length() == 0) {
        logger.msg(Arc::ERROR,"Options for plugin are missing"); goto exit;
      };
      if(!plugins.add(state_name.c_str(),options_s.c_str(),rest.c_str())) {
        logger.msg(Arc::ERROR,"Failed to register plugin for state %s",state_name);      
        goto exit;
      };
    }
    else if(command == "localcred") {
      std::string timeout_s = config_next_arg(rest);
      if(timeout_s.length() == 0) {
        logger.msg(Arc::ERROR,"timeout for plugin is missing"); goto exit;
      };
      char *ep;
      int to = strtoul(timeout_s.c_str(),&ep,10);
      if((*ep != 0) || (to<0)) {
        logger.msg(Arc::ERROR,"wrong number for timeout in plugin command");
        goto exit;
      };
      cred_plugin = rest;
      cred_plugin.timeout(to);
    }
    else if((!central_configuration && (command == "session")) ||
            (central_configuration  && (command == "sessiondir"))) {
                                /* set session root directory - applied
                                   to all following 'control' commands */
      session_root = config_next_arg(rest);
      if(session_root.length() == 0) {
        logger.msg(Arc::ERROR,"session root dir is missing"); goto exit;
      };
      if(rest.length() != 0) {
        logger.msg(Arc::ERROR,"junk in session root command"); goto exit;
      };
      if(session_root == "*") session_root="";
    } else if(central_configuration  && (command == "controldir")) {
      central_control_dir=rest;
    } else if(command == "control") {
      std::string control_dir = config_next_arg(rest);
      if(control_dir.length() == 0) {
        logger.msg(Arc::ERROR,"missing directory in control command"); goto exit;
      };
      if(control_dir == "*") control_dir="";
      if(command == "controldir") rest=".";
      for(;;) {
        std::string username = config_next_arg(rest);
        if(username.length() == 0) break;
        if(username == "*") {  /* add all gridmap users */
          if(!gridmap_user_list(rest)) {
            logger.msg(Arc::ERROR,"Can't read users in gridmap file %s",globus_gridmap); goto exit;
          };
          continue;
        };
        if(username == ".") {  /* accept all users in this control directory */
           /* !!!!!!! substitutions involving user names won't work !!!!!!!  */
           if(superuser) { username=""; }
           else { username=my_username; };
        };
        /* add new user to list */
        if(superuser || (my_username == username)) {
          if(users.HasUser(username)) { /* first is best */
            continue;
          };
          JobUsers::iterator user=users.AddUser(username,&cred_plugin,
                                       control_dir,session_root);
          if(user == users.end()) { /* bad bad user */
            logger.msg(Arc::WARNING,"Warning: creation of user \"%s\" failed",username);
          }
          else {
            std::string control_dir_ = control_dir;
            std::string session_root_ = session_root;
            std::string cache_dir_ = cache_dir;
            std::string cache_data_dir_ = cache_data_dir;
            std::string cache_link_dir_ = cache_link_dir;
            user->SetLRMS(default_lrms,default_queue);
            user->SetKeepFinished(default_ttl);
            user->SetKeepDeleted(default_ttr);
            user->SetReruns(default_reruns);
            user->SetDiskSpace(default_diskspace);
            user->substitute(control_dir_);
            user->substitute(session_root_);
            user->substitute(cache_dir_);
            user->substitute(cache_data_dir_);
            user->substitute(cache_link_dir_);
            user->SetControlDir(control_dir_);
            user->SetSessionRoot(session_root_);
            user->SetCacheDir(cache_dir_,cache_data_dir_,cache_link_dir_,private_cache);
            user->SetCacheSize(cache_max,cache_min);
            user->SetStrictSession(strict_session);
            /* add helper to poll for finished jobs */
            std::string cmd_ = nordugrid_libexec_loc;
            make_escaped_string(control_dir_);
            cmd_+="/scan-"+default_lrms+"-job";
            make_escaped_string(cmd_);
            cmd_+=" ";
            cmd_+=control_dir_;
            user->add_helper(cmd_);
            /* creating empty list of jobs */
            JobsList *jobs = new JobsList(*user,plugins); 
            (*user)=jobs; /* back-associate jobs with user :) */
          };
        };
      };
      last_control_dir = control_dir;
    }
    else if(command == "helper") {
      std::string helper_user = config_next_arg(rest);
      if(helper_user.length() == 0) {
        logger.msg(Arc::ERROR,"user for helper program is missing"); goto exit;
      };
      if(rest.length() == 0) {
        logger.msg(Arc::ERROR,"helper program is missing"); goto exit;
      };
      if(helper_user == "*") {  /* go through all configured users */
        for(JobUsers::iterator user=users.begin();user!=users.end();++user) {
          if(!(user->has_helpers())) {
            std::string rest_=rest;
            user->substitute(rest_);
            user->add_helper(rest_);
          };    
        };
      }
      else if(helper_user == ".") { /* special helper */
        std::string session_root_ = session_root;
        std::string control_dir_ = last_control_dir;
        my_user.SetLRMS(default_lrms,default_queue);
        my_user.SetKeepFinished(default_ttl);
        my_user.SetKeepDeleted(default_ttr);
        my_user.substitute(session_root_);
        my_user.substitute(control_dir_);
        my_user.SetSessionRoot(session_root_);
        my_user.SetControlDir(control_dir_);
        std::string my_helper=rest;
        users.substitute(my_helper);
        my_user.substitute(my_helper);
        my_user.add_helper(my_helper);
      }
      else {
        /* look for that user */
        JobUsers::iterator user=users.find(helper_user);
        if(user == users.end()) {
          logger.msg(Arc::ERROR,"%s user for helper program is not configured",helper_user);
          goto exit;
        };
        user->substitute(rest);
        user->add_helper(rest);
      };
    };
  };
  if(central_configuration) delete cf;
  config_close(cfile);
  if(infosys_user.length()) {
    struct passwd pw_;
    struct passwd *pw;
    char buf[BUFSIZ];
    getpwnam_r(infosys_user.c_str(),&pw_,buf,BUFSIZ,&pw);
    if(pw != NULL) {
      if(pw->pw_uid != 0) {
        for(JobUsers::iterator user=users.begin();user!=users.end();++user) {
          if(pw->pw_uid != user->get_uid()) {
            if(pw->pw_gid != user->get_gid()) {
              user->SetShareLevel(JobUser::jobinfo_share_group);
            } else {
              user->SetShareLevel(JobUser::jobinfo_share_all);
            };
          };
        };
      };
    };
  };
  return true;
exit:
  if(central_configuration) delete cf;
  config_close(cfile);
  return false;
}

bool print_serviced_users(const JobUsers &users) {
  for(JobUsers::const_iterator user = users.begin();user!=users.end();++user) {
    logger.msg(Arc::INFO,"Added user : %s",user->UnixName());
    logger.msg(Arc::INFO,"\tSession root dir : %s",user->SessionRoot());
    logger.msg(Arc::INFO,"\tControl dir      : %s",user->ControlDir());
    logger.msg(Arc::INFO,"\tdefault LRMS     : %s",user->DefaultLRMS());
    logger.msg(Arc::INFO,"\tdefault queue    : %s",user->DefaultQueue());
    logger.msg(Arc::INFO,"\tdefault ttl      : %u",user->KeepFinished());
    if(user->CacheDir().length() != 0) {
      if(user->CacheDataDir().length() == 0) {
        logger.msg(Arc::INFO,"\tCache dir        : %s (%s)",user->CacheDir(),user->CachePrivate()?"private":"global");
      } else {
        logger.msg(Arc::INFO,"\tCache info dir   : %s (%s)",user->CacheDir(),user->CachePrivate()?"private":"global");
        logger.msg(Arc::INFO,"\tCache data dir   : %s",user->CacheDataDir());
      };
    };
  };
  return true;
}

