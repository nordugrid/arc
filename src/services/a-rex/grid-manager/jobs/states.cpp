#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

/*
  Filename: states.cc
  keeps list of states
  acts on states
*/

#include <string>
#include <list>
#include <iostream>

#include "../files/info_files.h"
#include "../jobs/job_request.h"
#include "../run/run_parallel.h"
#include "../conf/environment.h"
#include "../mail/send_mail.h"
/* #include "../url/url_options.h" */
#include "../log/job_log.h"
#include "../conf/conf_file.h"
#include "../jobs/users.h"
#include "../jobs/job.h"
#include "../jobs/plugins.h"
#ifdef HAVE_MYPROXY_H
#include "../misc/proxy.h"
#include "../misc/myproxy_proxy.h"
#endif
#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <arc/StringConv.h>
#include <arc/URL.h>

static Arc::Logger& logger = Arc::Logger::getRootLogger();

#include "states.h"


long int JobsList::jobs_num[JOB_STATE_NUM] = { 0, 0, 0, 0, 0, 0, 0, 0 };
long int JobsList::max_jobs_processing=DEFAULT_MAX_JOBS;
long int JobsList::max_jobs_processing_emergency=1;
long int JobsList::max_jobs_running=-1;
long int JobsList::max_jobs=-1;
int JobsList::max_downloads=-1;
unsigned long long int JobsList::min_speed=0;
time_t JobsList::min_speed_time=300;
unsigned long long int JobsList::min_average_speed=0;
time_t JobsList::max_inactivity_time=300;
bool JobsList::use_secure_transfer=false; /* secure data transfer is OFF by default !!! */
bool JobsList::use_passive_transfer=false;
bool JobsList::use_local_transfer=false;
bool JobsList::cache_registration=false;
unsigned int JobsList::wakeup_period = 120; // default wakeup every 3 min.


#ifdef NO_GLOBUS_CODE
ContinuationPlugins::ContinuationPlugins(void) { };
ContinuationPlugins::~ContinuationPlugins(void) { };
bool ContinuationPlugins::add(const char* state,unsigned int timeout,const char* command) { return true; };
bool ContinuationPlugins::add(job_state_t state,unsigned int timeout,const char* command) { return true; };
bool ContinuationPlugins::add(const char* state,const char* options,const char* command) { return true; };
bool ContinuationPlugins::add(job_state_t state,const char* options,const char* command) { return true; };
ContinuationPlugins::action_t ContinuationPlugins::run(const JobDescription &job,const JobUser& user,int& result,std::string& response) { return act_pass; };
void RunPlugin::set(const std::string& cmd) { };
void RunPlugin::set(char const * const * args) { };
#endif

JobsList::JobsList(JobUser &user,ContinuationPlugins &plugins) {
  this->user=&user;
  this->plugins=&plugins;
  jobs.clear();
}

JobsList::~JobsList(void){
}

JobsList::iterator JobsList::FindJob(const JobId &id){
  iterator i;
  for(i=jobs.begin();i!=jobs.end();++i) {
    if((*i) == id) break;
  };
  return i;
}

bool JobsList::AddJobNoCheck(const JobId &id,uid_t uid,gid_t gid){
  iterator i;
  return AddJobNoCheck(id,i,uid,gid);
}

bool JobsList::AddJobNoCheck(const JobId &id,JobsList::iterator &i,uid_t uid,gid_t gid){
  i=jobs.insert(jobs.end(),JobDescription(id,user->SessionRoot() + "/" + id));
  i->keep_finished=user->KeepFinished();
  i->keep_deleted=user->KeepDeleted();
  i->set_uid(uid,gid);
  return true;
}

bool JobsList::AddJob(const JobId &id,uid_t uid,gid_t gid){
  /* jobs should be unique */
  if(FindJob(id) != jobs.end()) return false;
  logger.msg(Arc::INFO,"%s: Added",id);
  iterator i=jobs.insert(jobs.end(),
         JobDescription(id,user->SessionRoot() + "/" + id));
  i->keep_finished=user->KeepFinished();
  i->keep_deleted=user->KeepDeleted();
  i->set_uid(uid,gid);
  return true;
}

bool JobsList::AddJob(JobUser &user,const JobId &id,uid_t uid,gid_t gid){
  if((&user) != NULL) {
    if((this->user) == NULL) { this->user = &user; }
    else {
      if(this->user != &user) { /* incompatible user */
        return false;
      };
    };
  };
  return AddJob(id,uid,gid);
}

#ifndef NO_GLOBUS_CODE

bool JobsList::ActJob(const JobId &id,bool hard_job)  {
  iterator i=FindJob(id);
  if(i == jobs.end()) return false;
  return ActJob(i,hard_job);
}

bool JobsList::ActJobs(bool hard_job) {
  bool res = true;
  bool once_more = false;
  bool postpone_preparing = false;
  bool postpone_finishing = false;
  if((max_jobs_processing != -1) && 
     (!use_local_transfer) && 
     ((JOB_NUM_PROCESSING*3) > (max_jobs_processing*2))) {
    if(JOB_NUM_PREPARING > JOB_NUM_FINISHING) { 
      postpone_preparing=true; 
    } else if(JOB_NUM_PREPARING < JOB_NUM_FINISHING) {
      postpone_finishing=true;
    };
  };
  // first pass - optionally skipping some states
  for(iterator i=jobs.begin();i!=jobs.end();) {
    if(i->job_state == JOB_STATE_UNDEFINED) { once_more=true; }
    else if(((i->job_state == JOB_STATE_ACCEPTED) && postpone_preparing) ||
            ((i->job_state == JOB_STATE_INLRMS) && postpone_finishing)  ) {
      once_more=true;
      i++; continue;
    };
    res &= ActJob(i,hard_job);
  };
  // second pass - process skipped states and new jobs
  if(once_more) for(iterator i=jobs.begin();i!=jobs.end();) {
    res &= ActJob(i,hard_job);
  };
  return res;
}

bool JobsList::DestroyJobs(bool finished,bool active) {
  bool res = true;
  for(iterator i=jobs.begin();i!=jobs.end();) {
    res &= DestroyJob(i,finished,active);
  };
  return res;
}

/* returns false if had to run external process */
bool JobsList::DestroyJob(JobsList::iterator &i,bool finished,bool active) {
  logger.msg(Arc::INFO,"%s: Destroying",i->job_id);
  job_state_t new_state=i->job_state;
  if(new_state == JOB_STATE_UNDEFINED) {
    if((new_state=job_state_read_file(i->job_id,*user))==JOB_STATE_UNDEFINED) {
      logger.msg(Arc::ERROR,"%s: Can't read state - no comments, just cleaning",i->job_id);
      job_clean_final(*i,*user);
      if(i->local) { delete i->local; }; i=jobs.erase(i);
      return true;
    };
  };
  i->job_state = new_state;
  if((new_state == JOB_STATE_FINISHED) && (!finished)) { ++i; return true; };
  if(!active) { ++i; return true; };
  if((new_state != JOB_STATE_INLRMS) || 
     (job_lrms_mark_check(i->job_id,*user))) {
    logger.msg(Arc::INFO,"%s: Cleaning control and session directories",i->job_id);
    job_clean_final(*i,*user);
    if(i->local) { delete i->local; }; i=jobs.erase(i);
    return true;
  };
  logger.msg(Arc::INFO,"%s: This job may be still running - canceling",i->job_id);
  bool state_changed = false;
  if(!state_submiting(i,state_changed,true)) {
    logger.msg(Arc::WARNING,"%s: Cancelation failed (probably job finished) - cleaning anyway",i->job_id);
    job_clean_final(*i,*user);
    if(i->local) { delete i->local; }; i=jobs.erase(i);
    return true;
  };
  if(!state_changed) { ++i; return false; }; /* child still running */
  logger.msg(Arc::INFO,"%s: Cancelation probably succeeded - cleaning",i->job_id);
  job_clean_final(*i,*user);
  if(i->local) { delete i->local; };
  i=jobs.erase(i);
  return true;
}

/* do processing necessary in case of failure */
bool JobsList::FailedJob(const JobsList::iterator &i) {
  /* put mark - failed */
  if(!job_failed_mark_put(*i,*user,i->failure_reason)) return false;
  /* make all output files non-uploadable */
  std::list<FileData> fl;
  if(!job_output_read_file(i->job_id,*user,fl)) return true; /* no file - no error */
  for(std::list<FileData>::iterator ifl=fl.begin();ifl!=fl.end();++ifl) {
    // Remove destination without "preserve" option
    std::string value = Arc::URL(ifl->lfn).Option("preserve");
    if(value != "yes") ifl->lfn="";
  };
  if(!job_output_write_file(*i,*user,fl)) return false;
  if(!(i->local)) {
    JobLocalDescription *job_desc = new JobLocalDescription;
    if(!job_local_read_file(i->job_id,*user,*job_desc)) {
      logger.msg(Arc::ERROR,"%s: Failed reading local information.",i->job_id);
      delete job_desc;
    }
    else {
      i->local=job_desc;
    };
  };
  if(i->local) {
    i->local->uploads=0;
    job_local_write_file(*i,*user,*(i->local));
  };
  return true;
}

bool JobsList::GetLocalDescription(const JobsList::iterator &i) {
  if(!i->GetLocalDescription(*user)) {
    logger.msg(Arc::ERROR,"%s: Failed reading local information.",i->job_id);
    return false;
  };
  return true;
}

bool JobsList::state_submiting(const JobsList::iterator &i,bool &state_changed,bool cancel) {
  if(i->child == NULL) {
    /* no child was running yet, or recovering from fault */
    /* write grami file for globus-script-X-submit */
    JobLocalDescription* job_desc;
    if(i->local) { job_desc=i->local; }
    else {
      job_desc=new JobLocalDescription;
      if(!job_local_read_file(i->job_id,*user,*job_desc)) {
        logger.msg(Arc::ERROR,"%s: Failed reading local information.",i->job_id);
        if(!cancel) i->AddFailure("Internal error: can't read local file");
        delete job_desc;
        return false;
      };
      i->local=job_desc;
    };
    if(!cancel) {  /* in case of cancel all preparations are already done */
      const char *local_transfer_s = NULL;
      if(use_local_transfer) { 
        local_transfer_s="joboption_localtransfer=yes";
      };
      if(!write_grami(*i,*user,local_transfer_s)) {
        logger.msg(Arc::ERROR,"%s: Failed creating grami file.",i->job_id);
        return false;
      };
      if(!set_execs(*i,*user,i->SessionDir())) {
        logger.msg(Arc::ERROR,"%s: Failed setting executable permissions.",i->job_id);
        return false;
      };
      /* precreate file to store diagnostics from lrms */
      job_diagnostics_mark_put(*i,*user);
      job_lrmsoutput_mark_put(*i,*user);
    };
    /* submit/cancel job to LRMS using submit/cancel-X-job */
    std::string cmd;
    if(cancel) { cmd=nordugrid_libexec_loc+"/cancel-"+job_desc->lrms+"-job"; }
    else { cmd=nordugrid_libexec_loc+"/submit-"+job_desc->lrms+"-job"; };
    if(!cancel) {
      logger.msg(Arc::INFO,"%s: state SUBMITTING: starting child: %s",i->job_id,cmd);
    } else {
      logger.msg(Arc::INFO,"%s: state CANCELING: starting child: %s",i->job_id,cmd);
    };
    std::string grami = user->ControlDir()+"/job."+(*i).job_id+".grami";
    char* args[3] ={ (char*)cmd.c_str(), (char*)grami.c_str(), NULL };
    job_errors_mark_put(*i,*user);
    if(!RunParallel::run(*user,*i,args,&(i->child))) {
      if(!cancel) {
        i->AddFailure("Failed initiating job submission to LRMS");
        logger.msg(Arc::ERROR,"%s: Failed running submission process.",i->job_id);
      } else {
        logger.msg(Arc::ERROR,"%s: Failed running cancel process.",i->job_id);
      };
      return false;
    };
    return true;
  }
  else {
    /* child was run - check exit code */
    if(i->child->Running()) {
      /* child is running - come later */
      return true;
    };
    if(!cancel) {
      logger.msg(Arc::INFO,"%s: state SUBMITTING: child exited with code %i",i->job_id,i->child->Result());
    } else {
      logger.msg(Arc::INFO,"%s: state CANCELING: child exited with code %i",i->job_id,i->child->Result());
    };
    if(cancel) job_diagnostics_mark_move(*i,*user);
    if(i->child->Result() != 0) { 
      if(!cancel) {
        logger.msg(Arc::ERROR,"%s: Job submission to LRMS failed.",i->job_id);
        JobFailStateRemember(i,JOB_STATE_SUBMITING);
      } else {
        logger.msg(Arc::ERROR,"%s: Failed to cancel running job.",i->job_id);
      };
      delete i->child; i->child=NULL;
      if(!cancel) i->AddFailure("Job submission to LRMS failed");
      return false;
    };
    delete i->child; i->child=NULL;
    if(!cancel) {
      /* success code - get LRMS job id */
      std::string local_id=read_grami(i->job_id,*user);
      if(local_id.length() == 0) {
        logger.msg(Arc::ERROR,"%s: Failed obtaining lrms id.",i->job_id);
        i->AddFailure("Failed extracting LRMS ID due to some internal error");
        JobFailStateRemember(i,JOB_STATE_SUBMITING);
        return false;
      };
      /* put id into local information file */
      if(!GetLocalDescription(i)) {
        i->AddFailure("Internal error");
        return false;
      };   
      /*
      JobLocalDescription *job_desc;
      if(i->local) { job_desc=i->local; }
      else { job_desc=new JobLocalDescription; };
      if(i->local == NULL) {
        if(!job_local_read_file(i->job_id,*user,*job_desc)) {
          logger.msg(Arc::ERROR,"%s: Failed reading local information.",i->job_id);
          i->AddFailure("Internal error");
          delete job_desc; return false;
        };
        i->local=job_desc;
      };
      */
      i->local->localid=local_id;
      if(!job_local_write_file(*i,*user,*(i->local))) {
        i->AddFailure("Internal error");
        logger.msg(Arc::ERROR,"%s: Failed writing local information.",i->job_id);
        return false;
      };
    };
    /* move to next state */
    state_changed=true;
    return true;
  };
}

bool JobsList::state_loading(const JobsList::iterator &i,bool &state_changed,bool up) {
  /* RSL was analyzed/parsed - now run child process downloader
     to download job input files and to wait for user uploaded ones */
  if(i->child == NULL) { /* no child started */
    logger.msg(Arc::INFO,"%s: state: PREPARING/FINISHING: starting new child",i->job_id);
    /* no child was running yet, or recovering from fault */
    /* run it anyway and exit code will give more inforamtion */
    bool switch_user = (user->CachePrivate() || user->StrictSession());
    std::string cmd; 
    if(up) { cmd=nordugrid_libexec_loc+"/uploader"; }
    else { cmd=nordugrid_libexec_loc+"/downloader"; };
    uid_t user_id = user->get_uid();
    if(user_id == 0) user_id=i->get_uid();
    std::string user_id_s = Arc::tostring(user_id);
    std::string max_files_s;
    std::string min_speed_s;
    std::string min_speed_time_s;
    std::string min_average_speed_s;
    std::string max_inactivity_time_s;
    int argn=3;
    char* args[] = {
      (char*)(cmd.c_str()),
      "-U",
      (char*)(user_id_s.c_str()),
      NULL, // -n
      NULL, // (-n)
      NULL, // -c
      NULL, // -p
      NULL, // -l
      NULL, // -Z
      NULL, // -s
      NULL, // (-s)
      NULL, // -S
      NULL, // (-S)
      NULL, // -a
      NULL, // (-a)
      NULL, // -i
      NULL, // (-i)
      NULL, // id
      NULL, // control
      NULL, // session
      NULL, // cache
      NULL, // cache data
      NULL, // cache link
      NULL,
      NULL
    };
    if(JobsList::max_downloads > 0) {
      max_files_s=Arc::tostring(JobsList::max_downloads);
      args[argn]="-n"; argn++;
      args[argn]=(char*)(max_files_s.c_str()); argn++;
    };
    if(!use_secure_transfer) { 
      args[argn]="-c"; argn++;
    };
    if(!use_passive_transfer) { 
      args[argn]="-p"; argn++;
    };
    if(use_local_transfer) { 
      args[argn]="-l"; argn++;
    };
    if(central_configuration) { 
      args[argn]="-Z"; argn++;
    };
    if(JobsList::min_speed) {
      min_speed_s=Arc::tostring(JobsList::min_speed);
      min_speed_time_s=Arc::tostring(JobsList::min_speed_time);
      args[argn]="-s"; argn++; 
      args[argn]=(char*)(min_speed_s.c_str()); argn++;
      args[argn]="-S"; argn++; 
      args[argn]=(char*)(min_speed_time_s.c_str()); argn++;
    };
    if(JobsList::min_average_speed) {
      min_average_speed_s=Arc::tostring(JobsList::min_average_speed);
      args[argn]="-a"; argn++; 
      args[argn]=(char*)(min_average_speed_s.c_str()); argn++;
    };
    if(JobsList::max_inactivity_time) {
      max_inactivity_time_s=Arc::tostring(JobsList::max_inactivity_time);
      args[argn]="-i"; argn++; 
      args[argn]=(char*)(max_inactivity_time_s.c_str()); argn++;
    };
    args[argn]=(char*)(i->job_id.c_str()); argn++;
    args[argn]=(char*)(user->ControlDir().c_str()); argn++;
    args[argn]=(char*)(i->SessionDir().c_str()); argn++;
    if(user->CachePrivate() || ((!user->CachePrivate()) && (!switch_user))) {
      if(user->CacheDir().length() != 0) {
        args[argn]=(char*)(user->CacheDir().c_str()); argn++;
        if(user->CacheDataDir().length() != 0) {
          args[argn]=(char*)(user->CacheDataDir().c_str()); argn++;
        } else {
          args[argn]=(char*)(user->CacheDir().c_str()); argn++;
        };
        if(user->CacheLinkDir().length() != 0) {
          args[argn]=(char*)(user->CacheLinkDir().c_str()); argn++;
        };
      };
    };
    if(!up) { logger.msg(Arc::INFO,"%s: State PREPARING: starting child: %s",i->job_id,args[0]); }
    else { logger.msg(Arc::INFO,"%s: State FINISHING: starting child: %s",i->job_id,args[0]); };
    job_errors_mark_put(*i,*user);
    job_restart_mark_remove(i->job_id,*user);
    if(!RunParallel::run(*user,*i,args,&(i->child),switch_user)) {
      logger.msg(Arc::ERROR,"%s: Failed to run down/uploader process.",i->job_id);
      if(up) {
        i->AddFailure("Failed to run uploader (post-processing)");
      } else {
        i->AddFailure("Failed to run downloader (pre-processing)");
      };
      return false;
    };
  } else {
    if(i->child->Running()) {
      logger.msg(Arc::INFO,"%s: State: PREPARING/FINISHING: child is running",i->job_id);
      /* child is running - come later */
      return true;
    };
    /* child was run - check exit code */
    if(!up) { logger.msg(Arc::INFO,"%s: State: PREPARING: child exited with code: %i",i->job_id,i->child->Result()); }
    else { logger.msg(Arc::INFO,"%s: State: FINISHING: child exited with code: %i",i->job_id,i->child->Result()); };
    if(i->child->Result() != 0) { 
      if(i->child->Result() == 1) { 
        /* unrecoverable failure detected - all we can do is to kill the job */
        if(up) {
          logger.msg(Arc::ERROR,"%s: State: FINISHING: unrecoverable error detected (exit code 1)",i->job_id);
          i->AddFailure("Failed in files upload (post-processing)");
        } else {
          logger.msg(Arc::ERROR,"%s: State: PREPARING: unrecoverable error detected (exit code 1)",i->job_id);
          i->AddFailure("Failed in files download (pre-processing)");
        };
      } else if(i->child->Result() == 3) {
        /* in case of expired credentials there is a chance to get them 
           from credentials server - so far myproxy only */
#ifdef HAVE_MYPROXY_H
        if(GetLocalDescription(i)) {
          i->AddFailure("Internal error");
          if(i->local->credentialserver.length()) {
            std::string new_proxy_file =
                    user->ControlDir()+"/job."+i->job_id+".proxy.tmp";
            std::string old_proxy_file =
                    user->ControlDir()+"/job."+i->job_id+".proxy";
            remove(new_proxy_file.c_str());
            int h = open(new_proxy_file.c_str(),
                    O_WRONLY | O_CREAT | O_EXCL,S_IRUSR | S_IWUSR);
            if(h!=-1) {
              close(h);
              if(myproxy_renew(old_proxy_file.c_str(),new_proxy_file.c_str(),
                      i->local->credentialserver.c_str())) {
                renew_proxy(old_proxy_file.c_str(),new_proxy_file.c_str());
                /* imitate rerun request */
                job_restart_mark_put(*i,*user);
              };
            };
          };
        };
#endif
        if(up) {
          logger.msg(Arc::ERROR,"%s: State: FINISHING: credentials probably expired (exit code 3)",i->job_id);
          i->AddFailure("Failed in files upload due to expired credentials - try to renew");
        } else {
          logger.msg(Arc::ERROR,"%s: State: PREPARING: credentials probably expired (exit code 3)",i->job_id);
          i->AddFailure("Failed in files download due to expired credentials - try to renew");
        };
      } else {
        if(up) {
          logger.msg(Arc::ERROR,"%s: State: FINISHING: some error detected (exit code %i). Recover from such type of errors is not supported yet.",i->job_id,i->child->Result());
          i->AddFailure("Failed in files upload (post-processing)");
        } else {
          logger.msg(Arc::ERROR,"%s: State: PREPARING: some error detected (exit code %i). Recover from such type of errors is not supported yet.",i->job_id,i->child->Result());
          i->AddFailure("Failed in files download (pre-processing)");
        };
      };
      delete i->child; i->child=NULL;
      if(up) {
        JobFailStateRemember(i,JOB_STATE_FINISHING);
      } else {
        JobFailStateRemember(i,JOB_STATE_PREPARING);
      };
      return false;
    };
    /* success code - move to next state */
    state_changed=true;
    delete i->child; i->child=NULL;
  };
  return true;
}

bool JobsList::JobPending(JobsList::iterator &i) {
  if(i->job_pending) return true;
  i->job_pending=true; 
  return job_state_write_file(*i,*user,i->job_state,true);
}

job_state_t JobsList::JobFailStateGet(const JobsList::iterator &i) {
  if(!GetLocalDescription(i)) {
    return JOB_STATE_UNDEFINED;
  };
  if(i->local->failedstate.length() == 0) { return JOB_STATE_UNDEFINED; };
  for(int n = 0;states_all[n].name != NULL;n++) {
    if(!strcmp(states_all[n].name,i->local->failedstate.c_str())) {
      i->local->failedstate="";
      if(i->local->reruns <= 0) {
        logger.msg(Arc::ERROR,"%s: Job is not allowed to be rerun anymore.",i->job_id);
        job_local_write_file(*i,*user,*(i->local));
        return JOB_STATE_UNDEFINED;
      };
      i->local->reruns--;
      job_local_write_file(*i,*user,*(i->local));
      return states_all[n].id;
    };
  };
  logger.msg(Arc::ERROR,"%s: Job failed in unknown state. Won't rerun.",i->job_id);
  i->local->failedstate="";
  job_local_write_file(*i,*user,*(i->local));
  return JOB_STATE_UNDEFINED;
}

bool JobsList::RecreateTransferLists(const JobsList::iterator &i) {
  // Recreate list of output and input files
  std::list<FileData> fl_old;
  std::list<FileData> fl_new;
  std::list<FileData> fi_old;
  std::list<FileData> fi_new;
  // keep local info
  if(!GetLocalDescription(i)) return false;
  // keep current lists
  if(!job_output_read_file(i->job_id,*user,fl_old)) {
    logger.msg(Arc::ERROR,"%s: Failed to read list of output files.",i->job_id);
    return false;
  };
  if(!job_input_read_file(i->job_id,*user,fi_old)) {
    logger.msg(Arc::ERROR,"%s: Failed to read list of input files.",i->job_id);
    return false;
  };
  // recreate lists by reprocessing RSL 
  JobLocalDescription job_desc; // placeholder
  if(!process_job_req(*user,*i,job_desc)) {
    logger.msg(Arc::ERROR,"%s: Reprocessing RSL failed.",i->job_id);
    return false;
  };
  // Restore 'local'
  if(!job_local_write_file(*i,*user,*(i->local))) return false;
  // Read new lists
  if(!job_output_read_file(i->job_id,*user,fl_new)) {
    logger.msg(Arc::ERROR,"%s: Failed to read reprocessed list of output files.",i->job_id);
    return false;
  };
  if(!job_input_read_file(i->job_id,*user,fi_new)) {
    logger.msg(Arc::ERROR,"%s: Failed to read reprocessed list of input files.",i->job_id);
    return false;
  };
  // remove uploaded files
  i->local->uploads=0;
  for(std::list<FileData>::iterator i_new = fl_new.begin();
                                    i_new!=fl_new.end();) {
    if(!(i_new->has_lfn())) { ++i_new; continue; }; // user file - keep
    std::list<FileData>::iterator i_old = fl_old.begin();
    for(;i_old!=fl_old.end();++i_old) {
      if((*i_new) == (*i_old)) break;
    };
    if(i_old != fl_old.end()) { ++i_new; i->local->uploads++; continue; };
    i_new=fl_new.erase(i_new);
  };
  if(!job_output_write_file(*i,*user,fl_new)) return false;
  // remove existing files
  i->local->downloads=0;
  for(std::list<FileData>::iterator i_new = fi_new.begin();
                                    i_new!=fi_new.end();) {
    std::string path = i->session_dir+"/"+i_new->pfn;
    struct stat st;
    if(::stat(path.c_str(),&st) == -1) {
      ++i_new; i->local->downloads++;
    } else {
      i_new=fi_new.erase(i_new);
    };
  };
  if(!job_input_write_file(*i,*user,fi_new)) return false;
  return true;
}

bool JobsList::JobFailStateRemember(const JobsList::iterator &i,job_state_t state) {
  if(!(i->local)) {
    JobLocalDescription *job_desc = new JobLocalDescription;
    if(!job_local_read_file(i->job_id,*user,*job_desc)) {
      logger.msg(Arc::ERROR,"%s: Failed reading local information.",i->job_id);
      delete job_desc; return false;
    }
    else {
      i->local=job_desc;
    };
  };
  if(state == JOB_STATE_UNDEFINED) {
    i->local->failedstate="";
    return job_local_write_file(*i,*user,*(i->local));
  };
  if(i->local->failedstate.length() == 0) {
    i->local->failedstate=states_all[state].name;
    return job_local_write_file(*i,*user,*(i->local));
  };
  return true;
}

void JobsList::ActJobUndefined(JobsList::iterator &i,bool /*hard_job*/,
                               bool& once_more,bool& /*delete_job*/,
                               bool& job_error,bool& /*state_changed*/) {
        /* read state from file */
        /* undefined means job just detected - read it's status */
        /* but first check if it's not too many jobs in system  */
        if((JOB_NUM_ACCEPTED < max_jobs) || (max_jobs == -1)) {
          job_state_t new_state=job_state_read_file(i->job_id,*user);
          if(new_state == JOB_STATE_UNDEFINED) { /* something failed */
            logger.msg(Arc::ERROR,"%s: Reading status of new job failed.",i->job_id);
            job_error=true; i->AddFailure("Failed reading status of the job");
            return;
          };
          //  By keeping once_more==false jobs does not cycle here but
          // goes out and registers it's state in counters. This allows
          // to maintain limits properly after restart. Except FINISHED
          // jobs because they are not kept in memory and should be 
          // processed immediately.
          i->job_state = new_state; /* this can be any state, if we are
                                         recovering after failure */
          if(new_state == JOB_STATE_ACCEPTED) {
            // parse request (do it here because any other processing can 
            // read 'local' and then we never know if it was new job)
            JobLocalDescription *job_desc;
            job_desc = new JobLocalDescription;
            job_desc->sessiondir=i->session_dir;
            /* first phase of job - just  accepted - parse request */
            logger.msg(Arc::INFO,"%s: State: ACCEPTED: parsing RSL",i->job_id);
            if(!process_job_req(*user,*i,*job_desc)) {
              logger.msg(Arc::ERROR,"%s: Processing RSL failed.",i->job_id);
              job_error=true; i->AddFailure("Could not process RSL");
              delete job_desc;
              return; /* go to next job */
            };
            i->local=job_desc;
            // prepare information for logger
            job_log.make_file(*i,*user);
          } else if(new_state == JOB_STATE_FINISHED) {
            once_more=true;
          } else if(new_state == JOB_STATE_DELETED) {
            once_more=true;
          } else {
            logger.msg(Arc::INFO,"%s: %s: New job belongs to %i/%i",i->job_id.c_str(),
                JobDescription::get_state_name(new_state),i->get_uid(),i->get_gid());
            // Make it clean state after restart
            job_state_write_file(*i,*user,i->job_state);
          };
        }; // Not doing JobPending here because that job kind of does not exist.
        return;
}

void JobsList::ActJobAccepted(JobsList::iterator &i,bool /*hard_job*/,
                              bool& once_more,bool& /*delete_job*/,
                              bool& job_error,bool& state_changed) {
      /* accepted state - job was just accepted by jobmager-ng and we already
         know that it is accepted - now we are analyzing/parsing request,
         or it can also happen we are waiting for user specified time */
        logger.msg(Arc::INFO,"%s: State: ACCEPTED",i->job_id);
        if(!GetLocalDescription(i)) {
          job_error=true; i->AddFailure("Internal error");
          return; /* go to next job */
        };
        if(i->local->dryrun) {
          logger.msg(Arc::INFO,"%s: State: ACCEPTED: dryrun",i->job_id);
          i->AddFailure("User requested dryrun. Job skiped.");
          job_error=true; 
          return; /* go to next job */
        };
        if((max_jobs_processing == -1) ||
           (use_local_transfer) ||
           (i->local->downloads == 0) ||
           (JOB_NUM_PROCESSING < max_jobs_processing) ||
           ((JOB_NUM_FINISHING >= max_jobs_processing) && 
            (JOB_NUM_PREPARING < max_jobs_processing_emergency))) {
          /* check for user specified time */
          if(i->local->processtime.defined()) {
            logger.msg(Arc::INFO,"%s: State: ACCEPTED: have processtime %i",i->job_id.c_str(),
                  i->local->processtime.str().c_str());
            if((i->local->processtime) <= time(NULL)) {
              logger.msg(Arc::INFO,"%s: State: ACCEPTED: moving to PREPARING",i->job_id);
              state_changed=true; once_more=true;
              i->job_state = JOB_STATE_PREPARING;
            };
          }
          else {
            logger.msg(Arc::INFO,"%s: State: ACCEPTED: moving to PREPARING",i->job_id);
            state_changed=true; once_more=true;
            i->job_state = JOB_STATE_PREPARING;
          };
          if(state_changed) {
            /* gather some frontend specific information for user,
               do it only once */
            std::string cmd = nordugrid_libexec_loc+"/frontend-info-collector";
            char const * const args[2] = { cmd.c_str(), NULL };
            job_controldiag_mark_put(*i,*user,args);
          };
        } else JobPending(i);
        return;
}

void JobsList::ActJobPreparing(JobsList::iterator &i,bool /*hard_job*/,
                               bool& once_more,bool& /*delete_job*/,
                               bool& job_error,bool& state_changed) {
        /* preparing state - means job is parsed and we are going to download or
           already downloading input files. process downloader is run for
           that. it also checks for files user interface have to upload itself*/
        logger.msg(Arc::INFO,"%s: State: PREPARING",i->job_id);
        if(i->job_pending || state_loading(i,state_changed,false)) {
          if(i->job_pending || state_changed) {
            if((JOB_NUM_RUNNING<max_jobs_running) || (max_jobs_running==-1)) {
              i->job_state = JOB_STATE_SUBMITING;
              state_changed=true; once_more=true;
            } else {
              state_changed=false;
              JobPending(i);
            };
          };
        } else {
          if(i->GetFailure().length() == 0)
            i->AddFailure("downloader failed (pre-processing)");
          job_error=true;
          return; /* go to next job */
        };
        return;
}

void JobsList::ActJobSubmiting(JobsList::iterator &i,bool /*hard_job*/,
                               bool& once_more,bool& /*delete_job*/,
                               bool& job_error,bool& state_changed) {
        /* state submitting - everything is ready for submission - 
           so run submission */
        logger.msg(Arc::INFO,"%s: State: SUBMITTING",i->job_id);
        if(state_submiting(i,state_changed)) {
          if(state_changed) {
            i->job_state = JOB_STATE_INLRMS;
            once_more=true;
          };
        } else {
          job_error=true;
          return; /* go to next job */
        };
        return;
}

void JobsList::ActJobCanceling(JobsList::iterator &i,bool /*hard_job*/,
                               bool& once_more,bool& /*delete_job*/,
                               bool& job_error,bool& state_changed) {
        /* This state is like submitting, only -rm instead of -submit */
        logger.msg(Arc::INFO,"%s: State: CANCELING",i->job_id);
        if(state_submiting(i,state_changed,true)) {
          if(state_changed) {
            i->job_state = JOB_STATE_FINISHING;
            once_more=true;
          };
        }
        else { job_error=true; };
        return;
}

void JobsList::ActJobInlrms(JobsList::iterator &i,bool /*hard_job*/,
                            bool& once_more,bool& /*delete_job*/,
                            bool& job_error,bool& state_changed) {
        logger.msg(Arc::INFO,"%s: State: INLRMS",i->job_id);
        if(!GetLocalDescription(i)) {
          i->AddFailure("Failed reading local job information");
          job_error=true;
          return; /* go to next job */
        };
        if(i->job_pending || job_lrms_mark_check(i->job_id,*user)) {
          if(!i->job_pending) {
            logger.msg(Arc::INFO,"%s: Job finished.",i->job_id);
            job_diagnostics_mark_move(*i,*user);
            LRMSResult ec = job_lrms_mark_read(i->job_id,*user);
            if(ec.code() != 0) {
              logger.msg(Arc::INFO,"%s: State: INLRMS: exit message is %i %s",i->job_id,ec.code(),ec.description());
              /*
              * check if not asked to rerun job *
              JobLocalDescription *job_desc = i->local;
              if(job_desc->reruns > 0) { * rerun job once more *
                job_desc->reruns--;
                job_desc->localid="";
                job_local_write_file(*i,*user,*job_desc);
                job_lrms_mark_remove(i->job_id,*user);
                logger.msg(Arc::INFO,"%s: State: INLRMS: job restarted",i->job_id);
                i->job_state = JOB_STATE_SUBMITING; 
                // INLRMS slot is already taken by this job, so resubmission
                // can be done without any checks
              } else {
              */
                i->AddFailure("LRMS error: ("+
                      Arc::tostring(ec.code())+") "+ec.description());
                job_error=true;
                //i->job_state = JOB_STATE_FINISHING;
                JobFailStateRemember(i,JOB_STATE_INLRMS);
                // This does not require any special postprocessing and
                // can go to next state directly
              /*
              };
              */
              state_changed=true; once_more=true;
              return;
            } else {
              // i->job_state = JOB_STATE_FINISHING;
            };
          };
          if((max_jobs_processing == -1) ||
             (use_local_transfer) ||
             (i->local->uploads == 0) ||
             (JOB_NUM_PROCESSING < max_jobs_processing) ||
             ((JOB_NUM_PREPARING >= max_jobs_processing) &&
              (JOB_NUM_FINISHING < max_jobs_processing_emergency))) {
            state_changed=true; once_more=true;
            i->job_state = JOB_STATE_FINISHING;
          } else JobPending(i);
        };
        return;
}

void JobsList::ActJobFinishing(JobsList::iterator &i,bool hard_job,
                               bool& once_more,bool& /*delete_job*/,
                               bool& job_error,bool& state_changed) {
        logger.msg(Arc::INFO,"%s: State: FINISHING",i->job_id);
        if(state_loading(i,state_changed,true)) {
          if(state_changed) {
            i->job_state = JOB_STATE_FINISHED;
            once_more=true; hard_job=true;
          };
        } else {
          // i->job_state = JOB_STATE_FINISHED;
          state_changed=true; /* to send mail */
          once_more=true; hard_job=true;
          if(i->GetFailure().length() == 0)
            i->AddFailure("uploader failed (post-processing)");
          job_error=true;
          return; /* go to next job */
        };
        return;
}

static time_t prepare_cleanuptime(JobId &job_id,time_t &keep_finished,JobsList::iterator &i,JobUser &user) {
  JobLocalDescription job_desc;
  time_t t = -1;
  /* read lifetime - if empty it wont be overwritten */
  job_local_read_file(job_id,user,job_desc);
  if(!Arc::stringto(job_desc.lifetime,t)) t = keep_finished;
  if(t > keep_finished) t = keep_finished;
  time_t last_changed=job_state_time(job_id,user);
  t=last_changed+t; job_desc.cleanuptime=t;
  job_local_write_file(*i,user,job_desc);
  return t;
}

void JobsList::ActJobFinished(JobsList::iterator &i,bool hard_job,
                              bool& /*once_more*/,bool& /*delete_job*/,
                              bool& /*job_error*/,bool& state_changed) {
        if(job_clean_mark_check(i->job_id,*user)) {
          logger.msg(Arc::INFO,"%s: Job is requested to clean - deleting.",i->job_id);
          /* delete everything */
          job_clean_final(*i,*user);
        } else {
          if(job_restart_mark_check(i->job_id,*user)) { 
            job_restart_mark_remove(i->job_id,*user); 
            /* request to rerun job - check if can */
            // Get information about failed state and forget it
            job_state_t state_ = JobFailStateGet(i);
            if(state_ == JOB_STATE_PREPARING) {
              if(RecreateTransferLists(i)) {
                job_failed_mark_remove(i->job_id,*user);
                // state_changed=true;
                i->job_state = JOB_STATE_ACCEPTED;
                JobPending(i); // make it go to end of state immediately
                return;
              };
            } else if((state_ == JOB_STATE_SUBMITING) ||
                      (state_ == JOB_STATE_INLRMS)) {
              if(RecreateTransferLists(i)) {
                job_failed_mark_remove(i->job_id,*user);
                // state_changed=true;
                if(i->local->downloads > 0) {
                  // missing input files has to be re-downloaded
                  i->job_state = JOB_STATE_ACCEPTED;
                } else {
                  i->job_state = JOB_STATE_PREPARING;
                };
                JobPending(i); // make it go to end of state immediately
                return;
              };
            } else if(state_ == JOB_STATE_FINISHING) {
              if(RecreateTransferLists(i)) {
                job_failed_mark_remove(i->job_id,*user);
                // state_changed=true;
                i->job_state = JOB_STATE_INLRMS;
                JobPending(i); // make it go to end of state immediately
                return;
              };
            } else {
              logger.msg(Arc::ERROR,"%s: Can't rerun on request - not a suitable state.",i->job_id);
            };
          };
          if(hard_job) { /* try to minimize load */
            time_t t = -1;
            if(!job_local_read_cleanuptime(i->job_id,*user,t)) {
              /* must be first time - create cleanuptime */
              t=prepare_cleanuptime(i->job_id,i->keep_finished,i,*user);
            };
            /* check if it is not time to remove that job completely */
            if((time(NULL)-t) >= 0) {
              logger.msg(Arc::INFO,"%s: Job is too old - deleting.",i->job_id);
              if(i->keep_deleted) {
                job_clean_deleted(*i,*user);
                i->job_state = JOB_STATE_DELETED;
                state_changed=true;
              } else {
                /* delete everything */
                job_clean_final(*i,*user);
              };
            };
          };
        };
        return;
}

void JobsList::ActJobDeleted(JobsList::iterator &i,bool hard_job,
                             bool& /*once_more*/,bool& /*delete_job*/,
                             bool& /*job_error*/,bool& /*state_changed*/) {
        if(hard_job) { /* try to minimize load */
          time_t t = -1;
          if(!job_local_read_cleanuptime(i->job_id,*user,t)) {
            /* should not happen - delete job */
            JobLocalDescription job_desc;
            /* read lifetime - if empty it wont be overwritten */
            job_clean_final(*i,*user);
          } else {
            /* check if it is not time to remove remnants of that */
            if((time(NULL)-(t+i->keep_deleted)) >= 0) {
              logger.msg(Arc::INFO,"%s: Job is ancient - delete rest of information.",i->job_id);
              /* delete everything */
              job_clean_final(*i,*user);
            };
          };
        };
        return;
}

/* Do job's processing: check&change state, run necessary external
   programs, do necessary things. Also advance pointer and/or delete
   slot if necessary */
bool JobsList::ActJob(JobsList::iterator &i,bool hard_job) {
  bool once_more     = true;
  bool delete_job    = false;
  bool job_error     = false;
  bool state_changed = false;
  job_state_t old_state = i->job_state;
  bool old_pending = i->job_pending;
  while(once_more) {
    once_more     = false;
    delete_job    = false;
    job_error     = false;
    state_changed = false;
 /* some states can not be canceled (or there is no sense to do that) */
/*
       (i->job_state != JOB_STATE_FINISHING) &&
*/
    if((i->job_state != JOB_STATE_CANCELING) &&
       (i->job_state != JOB_STATE_FINISHED) &&
       (i->job_state != JOB_STATE_DELETED)) {
      if(job_cancel_mark_check(i->job_id,*user)) {
        logger.msg(Arc::INFO,"%s: Canceling job (%s) because of user request",i->job_id,user->UnixName());
        /* kill running child */
        if(i->child) { 
          i->child->Kill(0);
          delete i->child; i->child=NULL;
        };
        /* put some explanation */
        i->AddFailure("User requested to cancel the job");
        /* behave like if job failed */
        if(!FailedJob(i)) {
          /* DO NOT KNOW WHAT TO DO HERE !!!!!!!!!! */
        };
        /* special processing for INLRMS case */
        if(i->job_state == JOB_STATE_INLRMS) {
          i->job_state = JOB_STATE_CANCELING;
        }
        else if(i->job_state == JOB_STATE_FINISHING) {
          i->job_state = JOB_STATE_FINISHED;
        }
        else {
          i->job_state = JOB_STATE_FINISHING;
        };
        job_cancel_mark_remove(i->job_id,*user);
        state_changed=true;
        once_more=true;
      };
    };
    if(!state_changed) switch(i->job_state) {
    /* undefined state - not actual state - job was just added but
       not analyzed yet */
      case JOB_STATE_UNDEFINED: {
       ActJobUndefined(i,hard_job,once_more,delete_job,job_error,state_changed);
      }; break;
      case JOB_STATE_ACCEPTED: {
       ActJobAccepted(i,hard_job,once_more,delete_job,job_error,state_changed);
      }; break;
      case JOB_STATE_PREPARING: {
       ActJobPreparing(i,hard_job,once_more,delete_job,job_error,state_changed);
      }; break;
      case JOB_STATE_SUBMITING: {
       ActJobSubmiting(i,hard_job,once_more,delete_job,job_error,state_changed);
      }; break;
      case JOB_STATE_CANCELING: {
       ActJobCanceling(i,hard_job,once_more,delete_job,job_error,state_changed);
      }; break;
      case JOB_STATE_INLRMS: {
       ActJobInlrms(i,hard_job,once_more,delete_job,job_error,state_changed);
      }; break;
      case JOB_STATE_FINISHING: {
       ActJobFinishing(i,hard_job,once_more,delete_job,job_error,state_changed);
      }; break;
      case JOB_STATE_FINISHED: {
       ActJobFinished(i,hard_job,once_more,delete_job,job_error,state_changed);
      }; break;
      case JOB_STATE_DELETED: {
       ActJobDeleted(i,hard_job,once_more,delete_job,job_error,state_changed);
      }; break;
      default: { // should destroy job with unknown state ?!
      };
    };
    do {
      // Process errors which happened during processing this job
      if(job_error) {
        job_error=false;
        // always cause rerun - in order not to loose state change
        // Failed job - move it to proper state
        logger.msg(Arc::ERROR,"%s: Job failure detected.",i->job_id);
        if(!FailedJob(i)) { /* something is really wrong */
          i->AddFailure("Failed during processing failure");
          delete_job=true;
        } else { /* just move job to proper state */
          if((i->job_state == JOB_STATE_FINISHED) ||
             (i->job_state == JOB_STATE_DELETED)) {
            // Normally these stages should not generate errors
            // so ignore them
          } else if(i->job_state == JOB_STATE_FINISHING) {
            // No matter if FINISHING fails - it still goes to FINISHED
            i->job_state = JOB_STATE_FINISHED;
            state_changed=true;
            once_more=true;
          } else {
            // Any other failure should cause transfer to FINISHING
            i->job_state = JOB_STATE_FINISHING;
            state_changed=true;
            once_more=true;
          };
          i->job_pending=false;
        };
      };
      // Process state changes, also those generated by error processing
      if(state_changed) {
        state_changed=false;
        i->job_pending=false;
        // Report state change into log
        logger.msg(Arc::INFO,"%s: State: %s from %s",
              i->job_id.c_str(),JobDescription::get_state_name(i->job_state),
              JobDescription::get_state_name(old_state));
        if(!job_state_write_file(*i,*user,i->job_state)) {
          i->AddFailure("Failed writing job status");
          job_error=true;
        } else {
          // talk to external plugin to ask if we can proceed
          if(plugins) {
            int result;
            std::string response;
            ContinuationPlugins::action_t act =
                     plugins->run(*i,*user,result,response);
            // analyze result
            if(act == ContinuationPlugins::act_fail) {
              logger.msg(Arc::ERROR,"%s: Plugin in state %s : %s",
                  i->job_id.c_str(),states_all[i->get_state()].name,response.c_str());
              i->AddFailure(std::string("Plugin at state ")+
                  states_all[i->get_state()].name+" failed: "+response);
              job_error=true;
            } else if(act == ContinuationPlugins::act_log) {
              // Scream but go ahead
              logger.msg(Arc::WARNING,"%s: Plugin at state %s : %s",
                  i->job_id.c_str(),states_all[i->get_state()].name,response.c_str());
            } else if(act == ContinuationPlugins::act_pass) {
              // Just continue quietly
            } else {
              logger.msg(Arc::ERROR,"%s: Plugin execution failed",i->job_id);
              i->AddFailure(std::string("Failed running plugin at state ")+
                  states_all[i->get_state()].name);
              job_error=true;
            };
          };
          // Processing to be done on state changes 
          job_log.make_file(*i,*user);
          if(i->job_state == JOB_STATE_FINISHED) {
            if(i->GetLocalDescription(*user)) {
              job_stdlog_move(*i,*user,i->local->stdlog);
            };
            job_clean_finished(i->job_id,*user);
            job_log.finish_info(*i,*user);
            prepare_cleanuptime(i->job_id,i->keep_finished,i,*user);
          } else if(i->job_state == JOB_STATE_PREPARING) {
            job_log.start_info(*i,*user);
          };
        };
        /* send mail after errora and change are processed */
        /* do not send if something really wrong happened to avoid email DoS */
        if(!delete_job) send_mail(*i,*user);
      };
      // Keep repeating till error goes out
    } while(job_error);
    if(delete_job) { 
      logger.msg(Arc::ERROR,"%s: Delete request due to internal problems",i->job_id);
      i->job_state = JOB_STATE_FINISHED; /* move to finished in order to 
                                            remove from list */
      i->job_pending=false;
      job_state_write_file(*i,*user,i->job_state); 
      i->AddFailure("Serious troubles (problems during processing problems)");
      FailedJob(i);  /* put some marks */
      if(i->GetLocalDescription(*user)) {
        job_stdlog_move(*i,*user,i->local->stdlog);
      };
      job_clean_finished(i->job_id,*user);  /* clean status files */
      once_more=true; hard_job=true; /* to process some things in local */
    };
  };
  /* FINISHED+DELETED jobs are not kept in list - only in files */
  /* if job managed to get here with state UNDEFINED - 
     means we are overloaded with jobs - do not keep them in list */
  if((i->job_state == JOB_STATE_FINISHED) ||
     (i->job_state == JOB_STATE_DELETED) ||
     (i->job_state == JOB_STATE_UNDEFINED)) {
    /* this is the ONLY place there jobs are removed from memory */
    /* update counters */
    if(!old_pending) jobs_num[old_state]--;
    if(i->local) { delete i->local; };
    i=jobs.erase(i);
  }
  else {
    /* update counters */
    if(!old_pending) jobs_num[old_state]--;
    if(!i->job_pending) jobs_num[i->job_state]++;
    ++i;
  };
  return true;
}

#endif //  NO_GLOBUS_CODE

class JobFDesc {
 public:
  JobId id;
  uid_t uid;
  gid_t gid;
  time_t t;
  JobFDesc(char* s,unsigned int n):id(s,n),uid(0),gid(0),t(-1) { };
  bool operator<(JobFDesc &right) { return (t < right.t); };
};

/* find new jobs - sort by date to implement FIFO */
bool JobsList::ScanNewJobs(bool /*hard_job*/) {
  struct dirent file_;
  struct dirent *file;
  std::string cdir=user->ControlDir();
  DIR *dir=opendir(cdir.c_str());
  if(dir == NULL) {
    logger.msg(Arc::ERROR,"Failed reading control directory: %s",user->ControlDir());
    return false;
  };
  std::list<JobFDesc> ids;
  for(;;) {
    readdir_r(dir,&file_,&file);
    if(file == NULL) break;
    int l=strlen(file->d_name);
    if(l>(4+7)) {  /* job id contains at least 1 character */
      if(!strncmp(file->d_name,"job.",4)) {
        if(!strncmp((file->d_name)+(l-7),".status",7)) {
          JobFDesc id((file->d_name)+4,l-7-4);
          if(FindJob(id.id) == jobs.end()) {
            std::string fname=cdir+'/'+file->d_name;
            uid_t uid;
            gid_t gid;
            time_t t;
            if(check_file_owner(fname,*user,uid,gid,t)) {
              /* add it to the list */
              id.uid=uid; id.gid=gid; id.t=t;
              ids.push_back(id);
            };
          };
        };
      };
    };
  };
  closedir(dir);
  /* sorting by date */
  ids.sort();
  for(std::list<JobFDesc>::iterator id=ids.begin();id!=ids.end();++id) {
    iterator i;
    /* adding job with file's uid/gid */
    if(AddJobNoCheck(id->id,i,id->uid,id->gid)) {
/*    ActJob(i,hard_job);  */
    };
 /* failed AddJob most probably means it is already in list or limit exceeded */
  };
  return true;
}

