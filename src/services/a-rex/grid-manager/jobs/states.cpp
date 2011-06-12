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
#include "../misc/proxy.h"
#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <glibmm.h>
#include <arc/DateTime.h>
#include <arc/StringConv.h>
#include <arc/URL.h>
#include <arc/FileUtils.h>
#include <arc/credential/VOMSUtil.h>
#include <arc/data/FileCache.h>
#include <arc/Utils.h>

#include "dtr_generator.h"

static Arc::Logger& logger = Arc::Logger::getRootLogger();

#include "states.h"


JobsListConfig::JobsListConfig(void) {
  for(int n = 0;n<JOB_STATE_NUM;n++) jobs_num[n]=0;
  jobs_pending = 0;
  max_jobs_processing=DEFAULT_MAX_LOAD;
  max_jobs_processing_emergency=1;
  max_jobs_running=-1;
  max_jobs_total=-1;
  max_jobs=-1;
  max_jobs_per_dn=-1;
  max_downloads=-1;
  max_processing_share = 0;
  //limited_share;
  share_type = "";
  min_speed=0;
  min_speed_time=300;
  min_average_speed=0;
  max_inactivity_time=300;
  max_retries=DEFAULT_MAX_RETRIES;
  use_secure_transfer=false; /* secure data transfer is OFF by default !!! */
  use_passive_transfer=false;
  use_local_transfer=false;
  use_new_data_staging=false;
  wakeup_period = 120; // default wakeup every 2 mins
}

#ifdef NO_GLOBUS_CODE
ContinuationPlugins::ContinuationPlugins(void) { };
ContinuationPlugins::~ContinuationPlugins(void) { };
bool ContinuationPlugins::add(const char* state,unsigned int timeout,const char* command) { return true; };
bool ContinuationPlugins::add(job_state_t state,unsigned int timeout,const char* command) { return true; };
bool ContinuationPlugins::add(const char* state,const char* options,const char* command) { return true; };
bool ContinuationPlugins::add(job_state_t state,const char* options,const char* command) { return true; };
void ContinuationPlugins::run(const JobDescription &job,const JobUser& user,std::list<ContinuationPlugins::result_t>& results) { };
void RunPlugin::set(const std::string& cmd) { };
void RunPlugin::set(char const * const * args) { };
#endif

JobsList::JobsList(JobUser &user,ContinuationPlugins &plugins) {
  this->user=&user;
  this->plugins=&plugins;
  this->old_dir=NULL;
  this->dtr_generator=NULL;
  jobs.clear();
}
 
JobsList::~JobsList(void){}

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
  i=jobs.insert(jobs.end(),JobDescription(id,user->SessionRoot(id) + "/" + id));
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
         JobDescription(id,user->SessionRoot(id) + "/" + id));
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

bool JobsList::ActJob(const JobId &id)  {
  iterator i=FindJob(id);
  if(i == jobs.end()) return false;
  return ActJob(i);
}

void JobsList::CalculateShares(){
/**
   * transfer share calculation - look at current share
   * for preparing and finishing states, then look at
   * jobs ready to go into preparing or finished and set
   * the share. Need to do it here because in the ActJob*
   * methods we don't have an overview of all jobs.
   * In those methods we check the share to see if each
   * job can proceed.
*/

  JobsListConfig& jcfg = user->Env().jobs_cfg();
  // clear shares with 0 count
  std::map<std::string, int> preparing_job_share_copy = preparing_job_share;
  std::map<std::string, int> finishing_job_share_copy = finishing_job_share;
  preparing_job_share.clear();
  finishing_job_share.clear();
  for (std::map<std::string, int>::iterator i = preparing_job_share_copy.begin(); i != preparing_job_share_copy.end(); i++)
    if (i->second != 0) preparing_job_share[i->first] = i->second;
  for (std::map<std::string, int>::iterator i = finishing_job_share_copy.begin(); i != finishing_job_share_copy.end(); i++)
    if (i->second != 0) finishing_job_share[i->first] = i->second;
  
  // counters of current and potential preparing/finishing jobs
  std::map<std::string, int> pre_preparing_job_share = preparing_job_share;
  std::map<std::string, int> pre_finishing_job_share = finishing_job_share;

  for (iterator i=jobs.begin();i!=jobs.end();i++) {
    if (i->job_state == JOB_STATE_ACCEPTED) {
      // is job ready to move to preparing?
      if (i->retries == 0 && i->local->processtime != -1) {
        if (i->local->processtime <= time(NULL)) {
            pre_preparing_job_share[i->transfer_share]++;
        }
      }
      else if (i->next_retry <= time(NULL)) {
        pre_preparing_job_share[i->transfer_share]++;
      }
    }
    else if (i->job_state == JOB_STATE_INLRMS) {
      // is job ready to move to finishing?
      if (job_lrms_mark_check(i->job_id,*user) && i->next_retry <= time(NULL)) {
        pre_finishing_job_share[i->transfer_share]++;
      }
    }
  };
  
  // Now calculate how many of limited transfer shares are active
  // We need to try to preserve the maximum number of transfer threads 
  // for each active limited share. Jobs that belong to limited 
  // shares will be excluded from calculation of a share limit later
  int privileged_total_pre_preparing = 0;
  int privileged_total_pre_finishing = 0;
  int privileged_jobs_processing = 0;
  int privileged_preparing_job_share = 0;
  int privileged_finishing_job_share = 0;
  for (std::map<std::string, int>::iterator i = jcfg.limited_share.begin(); i != jcfg.limited_share.end(); i++) {
    if (pre_preparing_job_share.find(i->first) != pre_preparing_job_share.end()) {
      privileged_preparing_job_share++;
      privileged_jobs_processing += i->second;
      privileged_total_pre_preparing += pre_preparing_job_share[i->first];
    }
    if (pre_finishing_job_share.find(i->first) != pre_finishing_job_share.end()) {
      privileged_finishing_job_share++;
      privileged_jobs_processing += i->second;
      privileged_total_pre_finishing += pre_finishing_job_share[i->first];
    }
  }
  int unprivileged_jobs_processing = jcfg.max_jobs_processing - privileged_jobs_processing;

  // calculate the number of slots that can be allocated per unprivileged share
  // count the total number of unprivileged jobs (pre)preparing
  int total_pre_preparing = 0;
  int unprivileged_preparing_limit;
  int unprivileged_preparing_job_share = pre_preparing_job_share.size() - privileged_preparing_job_share;
  for (std::map<std::string, int>::iterator i = pre_preparing_job_share.begin(); i != pre_preparing_job_share.end(); i++) { 
    total_pre_preparing += i->second;
  }
  // exclude privileged jobs
  total_pre_preparing -= privileged_total_pre_preparing;
  if (jcfg.max_jobs_processing == -1 || unprivileged_preparing_job_share <= (unprivileged_jobs_processing / jcfg.max_processing_share))
    unprivileged_preparing_limit = jcfg.max_processing_share;
  else if (unprivileged_preparing_job_share > unprivileged_jobs_processing || unprivileged_preparing_job_share <= 0)
    unprivileged_preparing_limit = 1;
  else if (total_pre_preparing <= unprivileged_jobs_processing)
    unprivileged_preparing_limit = jcfg.max_processing_share;
  else
    unprivileged_preparing_limit = unprivileged_jobs_processing / unprivileged_preparing_job_share;

  // count the total number of jobs (pre)finishing
  int total_pre_finishing = 0;
  int unprivileged_finishing_limit;
  int unprivileged_finishing_job_share = pre_finishing_job_share.size() - privileged_finishing_job_share;
  for (std::map<std::string, int>::iterator i = pre_finishing_job_share.begin(); i != pre_finishing_job_share.end(); i++) {
    total_pre_finishing += i->second;
  }
  // exclude privileged jobs
  total_pre_finishing -= privileged_total_pre_finishing;
  if (jcfg.max_jobs_processing == -1 || unprivileged_finishing_job_share <= (unprivileged_jobs_processing / jcfg.max_processing_share))
    unprivileged_finishing_limit = jcfg.max_processing_share;
  else if (unprivileged_finishing_job_share > unprivileged_jobs_processing || unprivileged_finishing_job_share <= 0)
    unprivileged_finishing_limit = 1;
  else if (total_pre_finishing <= unprivileged_jobs_processing)
    unprivileged_finishing_limit = jcfg.max_processing_share;
  else
    unprivileged_finishing_limit = unprivileged_jobs_processing / unprivileged_finishing_job_share;

  // if there are queued jobs for both preparing and finishing, split the share between the two states
  if (jcfg.max_jobs_processing > 0 && total_pre_preparing > unprivileged_jobs_processing/2 && total_pre_finishing > unprivileged_jobs_processing/2) {
    unprivileged_preparing_limit = unprivileged_preparing_limit < 2 ? 1 : unprivileged_preparing_limit/2;
    unprivileged_finishing_limit = unprivileged_finishing_limit < 2 ? 1 : unprivileged_finishing_limit/2;
  }

  if (jcfg.max_jobs_processing > 0 && privileged_total_pre_preparing > privileged_jobs_processing/2 && privileged_total_pre_finishing > privileged_jobs_processing/2)
  for (std::map<std::string, int>::iterator i = jcfg.limited_share.begin(); i != jcfg.limited_share.end(); i++)
    i->second = i->second < 2 ? 1 : i->second/2; 
      
  preparing_max_share = pre_preparing_job_share;
  finishing_max_share = pre_finishing_job_share;
  for (std::map<std::string, int>::iterator i = preparing_max_share.begin(); i != preparing_max_share.end(); i++){
    if (jcfg.limited_share.find(i->first) != jcfg.limited_share.end())
      i->second = jcfg.limited_share[i->first];
    else
      i->second = unprivileged_preparing_limit;
  }
  for (std::map<std::string, int>::iterator i = finishing_max_share.begin(); i != finishing_max_share.end(); i++){
    if (jcfg.limited_share.find(i->first) != jcfg.limited_share.end())
      i->second = jcfg.limited_share[i->first];
    else
      i->second = unprivileged_finishing_limit;
  }
}

bool JobsList::ActJobs(void) {
  JobsListConfig& jcfg = user->Env().jobs_cfg();
/*
   * Need to calculate the shares here here because in the ActJob*
   * methods we don't have an overview of all jobs.
   * In those methods we check the share to see if each
   * job can proceed.
*/
  if (!jcfg.share_type.empty() && jcfg.max_processing_share > 0) {
    CalculateShares();
  }

  bool res = true;
  bool once_more = false;
  bool postpone_preparing = false;
  bool postpone_finishing = false;
  if((jcfg.max_jobs_processing != -1) && 
     (!jcfg.use_local_transfer) && 
     ((JOB_NUM_PROCESSING*3) > (jcfg.max_jobs_processing*2))) {
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
    res &= ActJob(i);
  };

  /* Recalculation of the shares before the second pass
   * to update the shares that appeared as a result of 
   * moving some jobs to ACCEPTED during the first pass
  */
  if (!jcfg.share_type.empty() && jcfg.max_processing_share > 0) {
    CalculateShares();
  }

  // second pass - process skipped states and new jobs
  if(once_more) for(iterator i=jobs.begin();i!=jobs.end();) {
    res &= ActJob(i);
  };

  // debug info on jobs per DN
  logger.msg(Arc::VERBOSE, "Current jobs in system (PREPARING to FINISHING) per-DN (%i entries)", jcfg.jobs_dn.size());
  for (std::map<std::string, ZeroUInt>::iterator it = jcfg.jobs_dn.begin(); it != jcfg.jobs_dn.end(); ++it)
    logger.msg(Arc::VERBOSE, "%s: %i", it->first, (unsigned int)(it->second));

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
  if(!state_submitting(i,state_changed,true)) {
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
  bool r = true;
  /* put mark - failed */
  //if(!job_failed_mark_put(*i,*user,i->failure_reason)) return false;
  if(job_failed_mark_add(*i,*user,i->failure_reason)) {
    i->failure_reason = "";
  } else {
    r = false;
  };
  /* make all output files non-uploadable */
  std::list<FileData> fl;
  if(job_output_read_file(i->job_id,*user,fl)) {
    for(std::list<FileData>::iterator ifl=fl.begin();ifl!=fl.end();++ifl) {
      // Remove destination without "preserve" option
      std::string value = Arc::URL(ifl->lfn).Option("preserve");
      if(value != "yes") ifl->lfn="";
    };
    if(!job_output_write_file(*i,*user,fl)) {
      r=false;
      logger.msg(Arc::ERROR,"%s: Failed writing list of output files: %s",i->job_id,Arc::StrError(errno));
    };
  } else {
    r=false;
    logger.msg(Arc::ERROR,"%s: Failed reading list of output files",i->job_id);
  }
  if(GetLocalDescription(i)) {
    i->local->uploads=0;
    job_local_write_file(*i,*user,*(i->local));
  } else {
    r=false;
  };
  return r;
}

bool JobsList::GetLocalDescription(const JobsList::iterator &i) {
  if(!i->GetLocalDescription(*user)) {
    logger.msg(Arc::ERROR,"%s: Failed reading local information",i->job_id);
    return false;
  };
  return true;
}

bool JobsList::state_submitting(const JobsList::iterator &i,bool &state_changed,bool cancel) {
  JobsListConfig& jcfg = user->Env().jobs_cfg();
  if(i->child == NULL) {
    /* no child was running yet, or recovering from fault */
    /* write grami file for globus-script-X-submit */
    JobLocalDescription* job_desc;
    if(i->local) { job_desc=i->local; }
    else {
      job_desc=new JobLocalDescription;
      if(!job_local_read_file(i->job_id,*user,*job_desc)) {
        logger.msg(Arc::ERROR,"%s: Failed reading local information",i->job_id);
        if(!cancel) i->AddFailure("Internal error: can't read local file");
        delete job_desc;
        return false;
      };
      i->local=job_desc;
    };
    if(!cancel) {  /* in case of cancel all preparations are already done */
      const char *local_transfer_s = NULL;
      if(jcfg.use_local_transfer) { 
        local_transfer_s="joboption_localtransfer=yes";
      };
      if(!write_grami(*i,*user,local_transfer_s)) {
        logger.msg(Arc::ERROR,"%s: Failed creating grami file",i->job_id);
        return false;
      };
      if(!set_execs(*i,*user,i->SessionDir())) {
        logger.msg(Arc::ERROR,"%s: Failed setting executable permissions",i->job_id);
        return false;
      };
      /* precreate file to store diagnostics from lrms */
      job_diagnostics_mark_put(*i,*user);
      job_lrmsoutput_mark_put(*i,*user);
    };
    /* submit/cancel job to LRMS using submit/cancel-X-job */
    std::string cmd;
    if(cancel) { cmd=user->Env().nordugrid_data_loc()+"/cancel-"+job_desc->lrms+"-job"; }
    else { cmd=user->Env().nordugrid_data_loc()+"/submit-"+job_desc->lrms+"-job"; };
    if(!cancel) {
      logger.msg(Arc::INFO,"%s: state SUBMITTING: starting child: %s",i->job_id,cmd);
    } else {
      if(!job_lrms_mark_check(i->job_id,*user)) {
        logger.msg(Arc::INFO,"%s: state CANCELING: starting child: %s",i->job_id,cmd);
      } else {
        logger.msg(Arc::INFO,"%s: Job has completed already. No action taken to cancel",i->job_id);
        state_changed=true;
        return true;
      }
    };
    std::string grami = user->ControlDir()+"/job."+(*i).job_id+".grami";
    std::string cfg_path = user->Env().nordugrid_config_loc();
    char const * args[5] ={ cmd.c_str(), "--config", cfg_path.c_str(), grami.c_str(), NULL };
    job_errors_mark_put(*i,*user);
    if(!RunParallel::run(*user,*i,args,&(i->child))) {
      if(!cancel) {
        i->AddFailure("Failed initiating job submission to LRMS");
        logger.msg(Arc::ERROR,"%s: Failed running submission process",i->job_id);
      } else {
        logger.msg(Arc::ERROR,"%s: Failed running cancel process",i->job_id);
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
    if(i->child->Result() != 0) { 
      if(!cancel) {
        logger.msg(Arc::ERROR,"%s: Job submission to LRMS failed",i->job_id);
        JobFailStateRemember(i,JOB_STATE_SUBMITTING);
      } else {
        logger.msg(Arc::ERROR,"%s: Failed to cancel running job",i->job_id);
      };
      delete i->child; i->child=NULL;
      if(!cancel) i->AddFailure("Job submission to LRMS failed");
      return false;
    };
    if(!cancel) {
      delete i->child; i->child=NULL;
      /* success code - get LRMS job id */
      std::string local_id=read_grami(i->job_id,*user);
      if(local_id.length() == 0) {
        logger.msg(Arc::ERROR,"%s: Failed obtaining lrms id",i->job_id);
        i->AddFailure("Failed extracting LRMS ID due to some internal error");
        JobFailStateRemember(i,JOB_STATE_SUBMITTING);
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
          logger.msg(Arc::ERROR,"%s: Failed reading local information",i->job_id);
          i->AddFailure("Internal error");
          delete job_desc; return false;
        };
        i->local=job_desc;
      };
      */
      i->local->localid=local_id;
      if(!job_local_write_file(*i,*user,*(i->local))) {
        i->AddFailure("Internal error");
        logger.msg(Arc::ERROR,"%s: Failed writing local information: %s",i->job_id,Arc::StrError(errno));
        return false;
      };
    } else {
      /* job diagnostics collection done in backgroud (scan-*-job script) */
      if(!job_lrms_mark_check(i->job_id,*user)) {
        /* job diag not yet collected - come later */
        return true;
      } else {
        logger.msg(Arc::INFO,"%s: state CANCELING: job diagnostics collected",i->job_id);
        delete i->child; i->child=NULL;
        job_diagnostics_mark_move(*i,*user);
      };
    };
    /* move to next state */
    state_changed=true;
    return true;
  };
}

bool JobsList::state_loading(const JobsList::iterator &i,bool &state_changed,bool up,bool &retry) {
  JobsListConfig& jcfg = user->Env().jobs_cfg();

  if (jcfg.use_new_data_staging && dtr_generator) {  /***** new data staging ******/
    // Job is in data staging - check progress
    if (dtr_generator->queryJobFinished(*i)) {
      // Finished - check for failure
      if (!i->GetFailure(*user).empty()) {
        JobFailStateRemember(i, (up ? JOB_STATE_FINISHING : JOB_STATE_PREPARING));
        return false;
      }
      // check for user-uploadable files
      if (!up) {
        int result = dtr_generator->checkUploadedFiles(*i);
        if (result == 2)
          return true;
        if (result == 0) {
          state_changed=true;
          return true;
        }
        // error
        return false;
      }
      else {
        state_changed = true;
        return true;
      }
    }
    else {
      // not finished yet
      logger.msg(Arc::VERBOSE, "%s: State: %s: still in data staging", i->job_id, (up ? "FINISHING" : "PREPARING"));
      return true;
    }
  }
  else {  /*************** old downloader/uploader *********************/

    /* RSL was analyzed/parsed - now run child process downloader
       to download job input files and to wait for user uploaded ones */
    if(i->child == NULL) { /* no child started */
      logger.msg(Arc::INFO,"%s: state: %s: starting new child",i->job_id,up?"FINISHING":"PREPARING");
      /* no child was running yet, or recovering from fault */
      /* run it anyway and exit code will give more inforamtion */
      bool switch_user = (user->CachePrivate() || user->StrictSession());
      std::string cmd;
      if(up) { cmd=user->Env().nordugrid_libexec_loc()+"/uploader"; }
      else { cmd=user->Env().nordugrid_libexec_loc()+"/downloader"; };
      uid_t user_id = user->get_uid();
      if(user_id == 0) user_id=i->get_uid();
      std::string user_id_s = Arc::tostring(user_id);
      std::string max_files_s;
      std::string min_speed_s;
      std::string min_speed_time_s;
      std::string min_average_speed_s;
      std::string max_inactivity_time_s;
      int argn=4;
      const char* args[] = {
        cmd.c_str(),
        "-U",
        user_id_s.c_str(),
        "-f",
        NULL, // -n
        NULL, // (-n)
        NULL, // -c
        NULL, // -p
        NULL, // -l
        NULL, // -s
        NULL, // (-s)
        NULL, // -S
        NULL, // (-S)
        NULL, // -a
        NULL, // (-a)
        NULL, // -i
        NULL, // (-i)
        NULL, // -d
        NULL, // (-d)
        NULL, // -C
        NULL, // (-C)
        NULL, // -r
        NULL, // (-r)
        NULL, // id
        NULL, // control
        NULL, // session
        NULL,
        NULL
      };
      if(jcfg.max_downloads > 0) {
        max_files_s=Arc::tostring(jcfg.max_downloads);
        args[argn]="-n"; argn++;
        args[argn]=(char*)(max_files_s.c_str()); argn++;
      };
      if(!jcfg.use_secure_transfer) {
        args[argn]="-c"; argn++;
      };
      if(jcfg.use_passive_transfer) {
        args[argn]="-p"; argn++;
      };
      if(jcfg.use_local_transfer) {
        args[argn]="-l"; argn++;
      };
      if(jcfg.min_speed) {
        min_speed_s=Arc::tostring(jcfg.min_speed);
        min_speed_time_s=Arc::tostring(jcfg.min_speed_time);
        args[argn]="-s"; argn++;
        args[argn]=(char*)(min_speed_s.c_str()); argn++;
        args[argn]="-S"; argn++;
        args[argn]=(char*)(min_speed_time_s.c_str()); argn++;
      };
      if(jcfg.min_average_speed) {
        min_average_speed_s=Arc::tostring(jcfg.min_average_speed);
        args[argn]="-a"; argn++;
        args[argn]=(char*)(min_average_speed_s.c_str()); argn++;
      };
      if(jcfg.max_inactivity_time) {
        max_inactivity_time_s=Arc::tostring(jcfg.max_inactivity_time);
        args[argn]="-i"; argn++;
        args[argn]=(char*)(max_inactivity_time_s.c_str()); argn++;
      };
      std::string debug_level = Arc::level_to_string(Arc::Logger::getRootLogger().getThreshold());
      std::string cfg_path = user->Env().nordugrid_config_loc();
      if (!debug_level.empty()) {
        args[argn]="-d"; argn++;
        args[argn]=(char*)(debug_level.c_str()); argn++;
      }
      if (!user->Env().nordugrid_config_loc().empty()) {
        args[argn]="-C"; argn++;
        args[argn]=(char*)(cfg_path.c_str()); argn++;
      }
      if(!jcfg.preferred_pattern.empty()) {
        args[argn]="-r"; argn++;
        args[argn]=(char*)(jcfg.preferred_pattern.c_str()); argn++;
      }
      args[argn]=(char*)(i->job_id.c_str()); argn++;
      args[argn]=(char*)(user->ControlDir().c_str()); argn++;
      args[argn]=(char*)(i->SessionDir().c_str()); argn++;

      logger.msg(Arc::INFO,"%s: State %s: starting child: %s",i->job_id,up?"FINISHING":"PREPARING",args[0]);
      job_errors_mark_put(*i,*user);
      // Remove restart mark because restart point may change. Keep it if we are
      // already processing failed job.
      if(!job_failed_mark_check(i->job_id,*user)) job_restart_mark_remove(i->job_id,*user);
      if(!RunParallel::run(*user,*i,(char**)args,&(i->child),switch_user)) {
        if(up) {
          logger.msg(Arc::ERROR,"%s: Failed to run uploader process",i->job_id);
          i->AddFailure("Failed to run uploader (post-processing)");
        } else {
          logger.msg(Arc::ERROR,"%s: Failed to run downloader process",i->job_id);
          i->AddFailure("Failed to run downloader (pre-processing)");
        };
        return false;
      };
    } else {
      if(i->child->Running()) {
        logger.msg(Arc::VERBOSE,"%s: State: PREPARING/FINISHING: child is running",i->job_id);
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
        } else if(i->child->Result() == 4) { // retryable cache error
          logger.msg(Arc::DEBUG, "%s: State: PREPARING/FINISHING: retryable error", i->job_id);
          delete i->child; i->child=NULL;
          retry = true;
          return true;
        }
        else {
          // Because we have no information (anymore) if failure happened
          // due to expired credentials let's just check them
          std::string old_proxy_file =
                      user->ControlDir()+"/job."+i->job_id+".proxy";
          Arc::Credential cred(old_proxy_file,"","","");
          // TODO: check if cred is at all there
          if(cred.GetEndTime() < Arc::Time()) {
            // Credential is expired
            logger.msg(Arc::ERROR,"%s: State: %s: credentials probably expired (exit code %i)",i->job_id,up?"FINISHING":"PREPARING",i->child->Result());
            /* in case of expired credentials there is a chance to get them
               from credentials server - so far myproxy only */
            if(GetLocalDescription(i)) {
              if(i->local->credentialserver.length()) {
                std::string new_proxy_file =
                        user->ControlDir()+"/job."+i->job_id+".proxy.tmp";
                remove(new_proxy_file.c_str());
                int h = ::open(new_proxy_file.c_str(),
                        O_WRONLY | O_CREAT | O_EXCL,S_IRUSR | S_IWUSR);
                if(h!=-1) {
                  close(h);
                  logger.msg(Arc::INFO,"%s: State: %s: trying to renew credentials",i->job_id,up?"FINISHING":"PREPARING");
                  if(myproxy_renew(old_proxy_file.c_str(),new_proxy_file.c_str(),
                          i->local->credentialserver.c_str())) {
                    renew_proxy(old_proxy_file.c_str(),new_proxy_file.c_str());
                    /* imitate rerun request */
                    job_restart_mark_put(*i,*user);
                  } else {
                    logger.msg(Arc::ERROR,"%s: State: %s: failed to renew credentials",i->job_id,up?"FINISHING":"PREPARING");
                  };
                } else {
                  logger.msg(Arc::ERROR,"%s: State: %s: failed to create temporary proxy for renew: %s",i->job_id,up?"FINISHING":"PREPARING",new_proxy_file);
                };
              };
            } else {
              i->AddFailure("Internal error");
            };
            if(up) {
              i->AddFailure("Failed in files upload probably due to expired credentials - try to renew");
            } else {
              i->AddFailure("Failed in files download probably due to expired credentials - try to renew");
            };
          } else {
            // Credentials were alright
            logger.msg(Arc::ERROR,"%s: State: %s: some error detected (exit code %i). Recover from such type of errors is not supported yet.",i->job_id,up?"FINISHING":"PREPARING",i->child->Result());
            if(up) {
              i->AddFailure("Failed in files upload (post-processing)");
            } else {
              i->AddFailure("Failed in files download (pre-processing)");
            };
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
  }
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
        logger.msg(Arc::ERROR,"%s: Job is not allowed to be rerun anymore",i->job_id);
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
    logger.msg(Arc::ERROR,"%s: Failed to read list of output files",i->job_id);
    return false;
  };
  if(!job_input_read_file(i->job_id,*user,fi_old)) {
    logger.msg(Arc::ERROR,"%s: Failed to read list of input files",i->job_id);
    return false;
  };
  // recreate lists by reprocessing RSL 
  JobLocalDescription job_desc; // placeholder
  if(!process_job_req(*user,*i,job_desc)) {
    logger.msg(Arc::ERROR,"%s: Reprocessing RSL failed",i->job_id);
    return false;
  };
  // Restore 'local'
  if(!job_local_write_file(*i,*user,*(i->local))) return false;
  // Read new lists
  if(!job_output_read_file(i->job_id,*user,fl_new)) {
    logger.msg(Arc::ERROR,"%s: Failed to read reprocessed list of output files",i->job_id);
    return false;
  };
  if(!job_input_read_file(i->job_id,*user,fi_new)) {
    logger.msg(Arc::ERROR,"%s: Failed to read reprocessed list of input files",i->job_id);
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
      logger.msg(Arc::ERROR,"%s: Failed reading local information",i->job_id);
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

void JobsList::ActJobUndefined(JobsList::iterator &i,
                               bool& once_more,bool& /*delete_job*/,
                               bool& job_error,bool& state_changed) {
        JobsListConfig& jcfg = user->Env().jobs_cfg();
        /* read state from file */
        /* undefined means job just detected - read it's status */
        /* but first check if it's not too many jobs in system  */
        if((JOB_NUM_ACCEPTED < jcfg.max_jobs) || (jcfg.max_jobs == -1)) {
          job_state_t new_state=job_state_read_file(i->job_id,*user);
          if(new_state == JOB_STATE_UNDEFINED) { /* something failed */
            logger.msg(Arc::ERROR,"%s: Reading status of new job failed",i->job_id);
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
            state_changed = true; // at least that makes email notification
            // parse request (do it here because any other processing can 
            // read 'local' and then we never know if it was new job)
            JobLocalDescription *job_desc;
            job_desc = new JobLocalDescription;
            job_desc->sessiondir=i->session_dir;
            /* first phase of job - just  accepted - parse request */
            logger.msg(Arc::INFO,"%s: State: ACCEPTED: parsing job description",i->job_id);
            if(!process_job_req(*user,*i,*job_desc)) {
              logger.msg(Arc::ERROR,"%s: Processing job description failed",i->job_id);
              job_error=true; i->AddFailure("Could not process job description");
              delete job_desc;
              return; /* go to next job */
            };
            i->local=job_desc;
            // set transfer share
            if (!jcfg.use_new_data_staging && !jcfg.share_type.empty()) {
              std::string user_proxy_file = job_proxy_filename(i->get_id(), *user).c_str();
              std::string cert_dir = "/etc/grid-security/certificates";
              std::string v = user->Env().cert_dir_loc();
              if(! v.empty()) cert_dir = v;
                Arc::Credential u(user_proxy_file,"",cert_dir,"");
                const std::string share = getCredentialProperty(u,jcfg.share_type);
                i->set_share(share);
                logger.msg(Arc::INFO, "%s: adding to transfer share %s",i->get_id(),i->transfer_share);
            }
            job_desc->transfershare = i->transfer_share;
            job_local_write_file(*i,*user,*job_desc);
            i->local->transfershare=i->transfer_share;
            job_state_write_file(*i,*user,i->job_state);

            // prepare information for logger
            user->Env().job_log().make_file(*i,*user);
          } else if(new_state == JOB_STATE_FINISHED) {
            once_more=true;
            job_state_write_file(*i,*user,i->job_state);
          } else if(new_state == JOB_STATE_DELETED) {
            once_more=true;
            job_state_write_file(*i,*user,i->job_state);
          } else {
            logger.msg(Arc::INFO,"%s: %s: New job belongs to %i/%i",i->job_id.c_str(),
                JobDescription::get_state_name(new_state),i->get_uid(),i->get_gid());
            // Make it clean state after restart
            job_state_write_file(*i,*user,i->job_state);
            i->retries = jcfg.max_retries;
            // set transfer share and counters
            JobLocalDescription *job_desc = new JobLocalDescription;
            if (!jcfg.use_new_data_staging && !jcfg.share_type.empty()) {
              std::string user_proxy_file = job_proxy_filename(i->get_id(), *user).c_str();
              std::string cert_dir = "/etc/grid-security/certificates";
              std::string v = user->Env().cert_dir_loc();
              if(! v.empty()) cert_dir = v;
                Arc::Credential u(user_proxy_file,"",cert_dir,"");
                const std::string share = getCredentialProperty(u,jcfg.share_type);
                i->set_share(share);
                logger.msg(Arc::INFO, "%s: adding to transfer share %s",i->get_id(),i->transfer_share);
            }
            job_local_read_file(i->job_id, *user, *job_desc);
            job_desc->transfershare = i->transfer_share;
            job_local_write_file(*i,*user,*job_desc);
            i->local=job_desc;
            if (new_state == JOB_STATE_PREPARING) preparing_job_share[i->transfer_share]++;
            if (new_state == JOB_STATE_FINISHING) finishing_job_share[i->transfer_share]++;
            i->Start();
            if (jcfg.use_new_data_staging && dtr_generator) {
              if((new_state == JOB_STATE_PREPARING || new_state == JOB_STATE_FINISHING)) {
                dtr_generator->receiveJob(*i);
              };
            };
            // add to DN map
            // here we don't enforce the per-DN limit since the jobs are
            // already in the system
            if (job_desc->DN.empty()) {
              logger.msg(Arc::ERROR, "Failed to get DN information from .local file for job %s", i->job_id);
              job_error=true; i->AddFailure("Could not get DN information for job");
              return; /* go to next job */
            };
            jcfg.jobs_dn[job_desc->DN]++;
          };
        }; // Not doing JobPending here because that job kind of does not exist.
        return;
}

void JobsList::ActJobAccepted(JobsList::iterator &i,
                              bool& once_more,bool& /*delete_job*/,
                              bool& job_error,bool& state_changed) {
        JobsListConfig& jcfg = user->Env().jobs_cfg();
      /* accepted state - job was just accepted by A-REX and we already
         know that it is accepted - now we are analyzing/parsing request,
         or it can also happen we are waiting for user specified time */
        logger.msg(Arc::VERBOSE,"%s: State: ACCEPTED",i->job_id);
        if(!GetLocalDescription(i)) {
          job_error=true; i->AddFailure("Internal error");
          return; /* go to next job */
        };
        if(i->local->dryrun) {
          logger.msg(Arc::INFO,"%s: State: ACCEPTED: dryrun",i->job_id);
          i->AddFailure("User requested dryrun. Job skipped.");
          job_error=true; 
          return; /* go to next job */
        };
        // check per-DN limit on processing jobs
        if(jcfg.max_jobs_per_dn < 0 || jcfg.jobs_dn[i->local->DN] < jcfg.max_jobs_per_dn)
        {
          // apply limits for old data staging
          if (jcfg.use_new_data_staging ||
              (jcfg.max_jobs_processing == -1) ||
              (jcfg.use_local_transfer) ||
              ((i->local->downloads == 0) && (i->local->rtes == 0)) ||
              (((JOB_NUM_PROCESSING < jcfg.max_jobs_processing) ||
               ((JOB_NUM_FINISHING >= jcfg.max_jobs_processing) &&
                (JOB_NUM_PREPARING < jcfg.max_jobs_processing_emergency))) &&
              (i->next_retry <= time(NULL)) &&
              (jcfg.share_type.empty() || preparing_job_share[i->transfer_share] < preparing_max_share[i->transfer_share]))) {

            /* check for user specified time */
            if(i->retries == 0 && i->local->processtime != -1 && (i->local->processtime) > time(NULL)) {
              logger.msg(Arc::INFO,"%s: State: ACCEPTED: has process time %s",i->job_id.c_str(),
                    i->local->processtime.str(Arc::UserTime));
              return;
            }
            // add to per-DN job list
            jcfg.jobs_dn[i->local->DN]++;
            logger.msg(Arc::INFO,"%s: State: ACCEPTED: moving to PREPARING",i->job_id);
            state_changed=true; once_more=true;
            i->job_state = JOB_STATE_PREPARING;
            /* if first pass then reset retries */
            if (i->retries ==0) i->retries = jcfg.max_retries;
            preparing_job_share[i->transfer_share]++;
            i->Start();
            if (jcfg.use_new_data_staging && dtr_generator) {
              dtr_generator->receiveJob(*i);
            };

            /* gather some frontend specific information for user,
               do it only once */
            if(state_changed && i->retries == jcfg.max_retries) {
              std::string cmd = user->Env().nordugrid_libexec_loc()+"/frontend-info-collector";
              char const * const args[2] = { cmd.c_str(), NULL };
              job_controldiag_mark_put(*i,*user,args);
            }
          } else JobPending(i);
        } else JobPending(i);
        return;
}

void JobsList::ActJobPreparing(JobsList::iterator &i,
                               bool& once_more,bool& /*delete_job*/,
                               bool& job_error,bool& state_changed) {
        JobsListConfig& jcfg = user->Env().jobs_cfg();
        /* preparing state - means job is in data staging system, so check
           if it has finished and whether all user uploadable files have been
           uploaded. */
        logger.msg(Arc::VERBOSE,"%s: State: PREPARING",i->job_id);
        bool retry = false;
        if(i->job_pending || state_loading(i,state_changed,false,retry)) {
          if(i->job_pending || state_changed) {
            if (state_changed) preparing_job_share[i->transfer_share]--;
            // Here we have branch. Either job is ordinary one and goes to SUBMIT
            // or it has no executable and hence goes to FINISHING
            if(!GetLocalDescription(i)) {
              logger.msg(Arc::ERROR,"%s: Failed obtaining local job information.",i->job_id);
              i->AddFailure("Internal error");
              job_error=true;
              return;
            };
            if(i->local->arguments.size()) {
              // with new data staging, when a job fails or is cancelled we wait until all
              // DTRs complete before continuing, so here we check for failure
              if(jcfg.use_new_data_staging && dtr_generator) {
                if(job_failed_mark_check(i->job_id, *user)) {
                  state_changed=true; once_more=true;
                  i->job_state = JOB_STATE_FINISHING;
                  dtr_generator->receiveJob(*i);
                  finishing_job_share[i->transfer_share]++;
                  return;
                };
              };
              if((JOB_NUM_RUNNING<jcfg.max_jobs_running) || (jcfg.max_jobs_running==-1)) {
                i->job_state = JOB_STATE_SUBMITTING;
                state_changed=true; once_more=true;
                i->retries = jcfg.max_retries;
              } else {
                state_changed=false;
                JobPending(i);
              };
            } else if(jcfg.use_new_data_staging && dtr_generator) {
              state_changed=true; once_more=true;
              i->job_state = JOB_STATE_FINISHING;
              dtr_generator->receiveJob(*i);
              finishing_job_share[i->transfer_share]++;
            } else if((jcfg.max_jobs_processing == -1) ||
                      (jcfg.use_local_transfer) ||
                      (i->local->uploads == 0) ||
                      (((JOB_NUM_PROCESSING < jcfg.max_jobs_processing) ||
                        ((JOB_NUM_PREPARING >= jcfg.max_jobs_processing) &&
                         (JOB_NUM_FINISHING < jcfg.max_jobs_processing_emergency)
                        )
                       ) &&
                       (i->next_retry <= time(NULL)) &&
                       (jcfg.share_type.empty() ||
                        finishing_job_share[i->transfer_share] < finishing_max_share[i->transfer_share])
                      )
                     ) {
              i->job_state = JOB_STATE_FINISHING;
              state_changed=true; once_more=true;
              i->retries = jcfg.max_retries;
              finishing_job_share[i->transfer_share]++;
            } else {
              JobPending(i);
            };
          }
          else if (retry){
            preparing_job_share[i->transfer_share]--;
            if(--i->retries == 0) { // no tries left
              logger.msg(Arc::ERROR,"%s: Data staging failed. No retries left.",i->job_id);
              i->AddFailure("Data staging failed (pre-processing)");
              job_error=true;
              JobFailStateRemember(i,JOB_STATE_PREPARING);
              return;
            }
            /* set next retry time
               exponential back-off algorithm - wait 10s, 40s, 90s, 160s,...
               with a bit of randomness thrown in - vary by up to 50% of wait_time */
            int wait_time = 10 * (jcfg.max_retries - i->retries) * (jcfg.max_retries - i->retries);
            int randomness = (rand() % wait_time) - (wait_time/2);
            wait_time += randomness;
            i->next_retry = time(NULL) + wait_time;
            logger.msg(Arc::ERROR,"%s: Download failed. %d retries left. Will wait for %ds before retrying",i->job_id,i->retries,wait_time);
            /* set back to ACCEPTED */
            i->job_state = JOB_STATE_ACCEPTED;
            if (--(jcfg.jobs_dn[i->local->DN]) <= 0)
              jcfg.jobs_dn.erase(i->local->DN);
            state_changed = true;
          }; 
        } 
        else {
          if(i->GetFailure(*user).length() == 0)
            i->AddFailure("Data staging failed (pre-processing)");
          job_error=true;
          preparing_job_share[i->transfer_share]--;
          return; /* go to next job */
        };
        return;
}

void JobsList::ActJobSubmitting(JobsList::iterator &i,
                                bool& once_more,bool& /*delete_job*/,
                                bool& job_error,bool& state_changed) {
        /* state submitting - everything is ready for submission - 
           so run submission */
        logger.msg(Arc::VERBOSE,"%s: State: SUBMITTING",i->job_id);
        if(state_submitting(i,state_changed)) {
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

void JobsList::ActJobCanceling(JobsList::iterator &i,
                               bool& once_more,bool& /*delete_job*/,
                               bool& job_error,bool& state_changed) {
        JobsListConfig& jcfg = user->Env().jobs_cfg();
        /* This state is like submitting, only -rm instead of -submit */
        logger.msg(Arc::VERBOSE,"%s: State: CANCELING",i->job_id);
        if(state_submitting(i,state_changed,true)) {
          if(state_changed) {
            i->job_state = JOB_STATE_FINISHING;
            if (jcfg.use_new_data_staging && dtr_generator) {
              dtr_generator->receiveJob(*i);
            };
            finishing_job_share[i->transfer_share]++;
            once_more=true;
          };
        }
        else { job_error=true; };
        return;
}

void JobsList::ActJobInlrms(JobsList::iterator &i,
                            bool& once_more,bool& /*delete_job*/,
                            bool& job_error,bool& state_changed) {
        JobsListConfig& jcfg = user->Env().jobs_cfg();
        logger.msg(Arc::VERBOSE,"%s: State: INLRMS",i->job_id);
        if(!GetLocalDescription(i)) {
          i->AddFailure("Failed reading local job information");
          job_error=true;
          return; /* go to next job */
        };
        /* only check lrms job status on first pass */
        if(i->retries == 0 || i->retries == jcfg.max_retries) {
          if(i->job_pending || job_lrms_mark_check(i->job_id,*user)) {
            if(!i->job_pending) {
              logger.msg(Arc::INFO,"%s: Job finished",i->job_id);
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
                i->job_state = JOB_STATE_SUBMITTING;
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
            if(jcfg.use_new_data_staging && dtr_generator) {
              state_changed=true; once_more=true;
              i->job_state = JOB_STATE_FINISHING;
              dtr_generator->receiveJob(*i);
              finishing_job_share[i->transfer_share]++;
            }
            else if ((jcfg.max_jobs_processing == -1) ||
              (jcfg.use_local_transfer) ||
              (i->local->uploads == 0) ||
              (((JOB_NUM_PROCESSING < jcfg.max_jobs_processing) ||
               ((JOB_NUM_PREPARING >= jcfg.max_jobs_processing) &&
                (JOB_NUM_FINISHING < jcfg.max_jobs_processing_emergency))) &&
               (i->next_retry <= time(NULL)) &&
               (jcfg.share_type.empty() || finishing_job_share[i->transfer_share] < finishing_max_share[i->transfer_share]))) {
                 state_changed=true; once_more=true;
                 i->job_state = JOB_STATE_FINISHING;
                 /* if first pass then reset retries */
                 if (i->retries == 0) i->retries = jcfg.max_retries;
                 finishing_job_share[i->transfer_share]++;
            } else JobPending(i);
          }
        } else if((jcfg.max_jobs_processing == -1) ||
            (jcfg.use_local_transfer) ||
            (i->local->uploads == 0) ||
            (((JOB_NUM_PROCESSING < jcfg.max_jobs_processing) ||
             ((JOB_NUM_PREPARING >= jcfg.max_jobs_processing) &&
              (JOB_NUM_FINISHING < jcfg.max_jobs_processing_emergency))) &&
            (i->next_retry <= time(NULL)) &&
            (jcfg.share_type.empty() || finishing_job_share[i->transfer_share] < finishing_max_share[i->transfer_share]))) {
              state_changed=true; once_more=true;
              i->job_state = JOB_STATE_FINISHING;
              finishing_job_share[i->transfer_share]++;
        } else {
          JobPending(i);
        };
}

void JobsList::ActJobFinishing(JobsList::iterator &i,
                               bool& once_more,bool& /*delete_job*/,
                               bool& job_error,bool& state_changed) {
        JobsListConfig& jcfg = user->Env().jobs_cfg();
        logger.msg(Arc::VERBOSE,"%s: State: FINISHING",i->job_id);
        bool retry = false;
        if(state_loading(i,state_changed,true,retry)) {
          if (retry) {
            finishing_job_share[i->transfer_share]--;
            if(--i->retries == 0) { // no tries left
              logger.msg(Arc::ERROR,"%s: Upload failed. No retries left.",i->job_id);
              i->AddFailure("uploader failed (post-processing)");
              job_error=true;
              JobFailStateRemember(i,JOB_STATE_FINISHING);
              return;
            };
            /* set next retry time
            exponential back-off algorithm - wait 10s, 40s, 90s, 160s,...
            with a bit of randomness thrown in - vary by up to 50% of wait_time */
            int wait_time = 10 * (jcfg.max_retries - i->retries) * (jcfg.max_retries - i->retries);
            int randomness = (rand() % wait_time) - (wait_time/2);
            wait_time += randomness;
            i->next_retry = time(NULL) + wait_time;
            logger.msg(Arc::ERROR,"%s: Upload failed. %d retries left. Will wait for %ds before retrying.",i->job_id,i->retries,wait_time);
            /* set back to INLRMS */
            i->job_state = JOB_STATE_INLRMS;
            state_changed = true;
            return;
          }
          else if(state_changed) {
            finishing_job_share[i->transfer_share]--;
            i->job_state = JOB_STATE_FINISHED;
            if(GetLocalDescription(i)) {
              if (--(jcfg.jobs_dn[i->local->DN]) <= 0)
                jcfg.jobs_dn.erase(i->local->DN);
            }
            once_more=true;
          }
          else {
            return; // still in data staging
          }
        } else {
          // i->job_state = JOB_STATE_FINISHED;
          state_changed=true; /* to send mail */
          once_more=true;
          if(i->GetFailure(*user).length() == 0)
            i->AddFailure("uploader failed (post-processing)");
          job_error=true;
          finishing_job_share[i->transfer_share]--;
          /* go to next job */
        };
        if (jcfg.use_new_data_staging) {
          // release cache
          try {
            CacheConfig cache_config(user->Env());
            Arc::FileCache cache(cache_config.getCacheDirs(),
                                 cache_config.getRemoteCacheDirs(),
                                 cache_config.getDrainingCacheDirs(),
                                 i->job_id, i->job_uid, i->job_gid);
            cache.Release();
          }
          catch (CacheConfigException e) {
            logger.msg(Arc::WARNING, "Error with cache configuration: %s. Cannot clean up files for job %s", e.what(), i->job_id);
          }
        }
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

void JobsList::ActJobFinished(JobsList::iterator &i,
                              bool& /*once_more*/,bool& /*delete_job*/,
                              bool& /*job_error*/,bool& state_changed) {
        if(job_clean_mark_check(i->job_id,*user)) {
          logger.msg(Arc::INFO,"%s: Job is requested to clean - deleting",i->job_id);
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
            } else if((state_ == JOB_STATE_SUBMITTING) ||
                      (state_ == JOB_STATE_INLRMS)) {
              if(RecreateTransferLists(i)) {
                job_failed_mark_remove(i->job_id,*user);
                // state_changed=true;
                if((i->local->downloads > 0) || (i->local->rtes > 0)) {
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
            } else if(state_ == JOB_STATE_UNDEFINED) {
              logger.msg(Arc::ERROR,"%s: Can't rerun on request",i->job_id);
            } else {
              logger.msg(Arc::ERROR,"%s: Can't rerun on request - not a suitable state",i->job_id);
            };
          };
          /*if(hard_job)*/ { /* try to minimize load */
            time_t t = -1;
            if(!job_local_read_cleanuptime(i->job_id,*user,t)) {
              /* must be first time - create cleanuptime */
              t=prepare_cleanuptime(i->job_id,i->keep_finished,i,*user);
            };
            /* check if it is not time to remove that job completely */
            if(((int)(time(NULL)-t)) >= 0) {
              logger.msg(Arc::INFO,"%s: Job is too old - deleting",i->job_id);
              if(i->keep_deleted) {
                // here we have to get the cache per-job dirs to be deleted
                CacheConfig cache_config;
                std::list<std::string> cache_per_job_dirs;
                try {
                  cache_config = CacheConfig(user->Env());
                }
                catch (CacheConfigException e) {
                  logger.msg(Arc::ERROR, "Error with cache configuration: %s", e.what());
                  job_clean_deleted(*i,*user);
                  i->job_state = JOB_STATE_DELETED;
                  state_changed=true;
                  return;
                }
                std::vector<std::string> conf_caches = cache_config.getCacheDirs();
                // add each dir to our list
                for (std::vector<std::string>::iterator it = conf_caches.begin(); it != conf_caches.end(); it++) {
                  cache_per_job_dirs.push_back(it->substr(0, it->find(" "))+"/joblinks");
                }
                // add remote caches
                std::vector<std::string> remote_caches = cache_config.getRemoteCacheDirs();
                for (std::vector<std::string>::iterator it = remote_caches.begin(); it != remote_caches.end(); it++) {
                  cache_per_job_dirs.push_back(it->substr(0, it->find(" "))+"/joblinks");
                }
                // add draining caches
                std::vector<std::string> draining_caches = cache_config.getDrainingCacheDirs();
                for (std::vector<std::string>::iterator it = draining_caches.begin(); it != draining_caches.end(); it++) {
                  cache_per_job_dirs.push_back(it->substr(0, it->find(" "))+"/joblinks");
                }
                job_clean_deleted(*i,*user,cache_per_job_dirs);
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

void JobsList::ActJobDeleted(JobsList::iterator &i,
                             bool& /*once_more*/,bool& /*delete_job*/,
                             bool& /*job_error*/,bool& /*state_changed*/) {
        /*if(hard_job)*/ { /* try to minimize load */
          time_t t = -1;
          if(!job_local_read_cleanuptime(i->job_id,*user,t)) {
            /* should not happen - delete job */
            JobLocalDescription job_desc;
            /* read lifetime - if empty it wont be overwritten */
            job_clean_final(*i,*user);
          } else {
            /* check if it is not time to remove remnants of that */
            if((time(NULL)-(t+i->keep_deleted)) >= 0) {
              logger.msg(Arc::INFO,"%s: Job is ancient - delete rest of information",i->job_id);
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
bool JobsList::ActJob(JobsList::iterator &i) {
  JobsListConfig& jcfg = user->Env().jobs_cfg();
  bool once_more     = true;
  bool delete_job    = false;
  bool job_error     = false;
  bool state_changed = false;
  job_state_t old_state = i->job_state;
  job_state_t old_reported_state = i->job_state;
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
       (i->job_state != JOB_STATE_DELETED) &&
       (i->job_state != JOB_STATE_SUBMITTING)) {
      if(job_cancel_mark_check(i->job_id,*user)) {
        logger.msg(Arc::INFO,"%s: Canceling job (%s) because of user request",i->job_id,user->UnixName());
        if (jcfg.use_new_data_staging && dtr_generator) {
          if ((i->job_state == JOB_STATE_PREPARING || i->job_state == JOB_STATE_FINISHING)) {
            dtr_generator->cancelJob(*i);
          };
        };
        /* kill running child */
        if(i->child) { 
          i->child->Kill(0);
          delete i->child; i->child=NULL;
        };
        /* update transfer share counters */
        if (i->job_state == JOB_STATE_PREPARING && !i->job_pending) preparing_job_share[i->transfer_share]--;
        else if (i->job_state == JOB_STATE_FINISHING) finishing_job_share[i->transfer_share]--;
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
          if(GetLocalDescription(i)) {
            if (--(jcfg.jobs_dn[i->local->DN]) <= 0)
              jcfg.jobs_dn.erase(i->local->DN);
          }
        }
        else if (!jcfg.use_new_data_staging || i->job_state != JOB_STATE_PREPARING) {
          // if preparing with new data staging we wait to get back all DTRs
          i->job_state = JOB_STATE_FINISHING;
          finishing_job_share[i->transfer_share]++;
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
       ActJobUndefined(i,once_more,delete_job,job_error,state_changed);
      }; break;
      case JOB_STATE_ACCEPTED: {
       ActJobAccepted(i,once_more,delete_job,job_error,state_changed);
      }; break;
      case JOB_STATE_PREPARING: {
       ActJobPreparing(i,once_more,delete_job,job_error,state_changed);
      }; break;
      case JOB_STATE_SUBMITTING: {
       ActJobSubmitting(i,once_more,delete_job,job_error,state_changed);
      }; break;
      case JOB_STATE_CANCELING: {
       ActJobCanceling(i,once_more,delete_job,job_error,state_changed);
      }; break;
      case JOB_STATE_INLRMS: {
       ActJobInlrms(i,once_more,delete_job,job_error,state_changed);
      }; break;
      case JOB_STATE_FINISHING: {
       ActJobFinishing(i,once_more,delete_job,job_error,state_changed);
      }; break;
      case JOB_STATE_FINISHED: {
       ActJobFinished(i,once_more,delete_job,job_error,state_changed);
      }; break;
      case JOB_STATE_DELETED: {
       ActJobDeleted(i,once_more,delete_job,job_error,state_changed);
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
        logger.msg(Arc::ERROR,"%s: Job failure detected",i->job_id);
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
            if(GetLocalDescription(i)) {
              if (--(jcfg.jobs_dn[i->local->DN]) <= 0)
                jcfg.jobs_dn.erase(i->local->DN);
            }
            state_changed=true;
            once_more=true;
          } else if (!jcfg.use_new_data_staging || i->job_state != JOB_STATE_PREPARING){
            // If job fails in preparing with new data staging, wait for DTRs to finish
            // Any other failure should cause transfer to go to FINISHING
            i->job_state = JOB_STATE_FINISHING;
            if (jcfg.use_new_data_staging && dtr_generator) {
              dtr_generator->receiveJob(*i);
            };
            finishing_job_share[i->transfer_share]++;
            state_changed=true;
            once_more=true;
          };
          i->job_pending=false;
        };
      };
      // Process state changes, also those generated by error processing
      if(old_reported_state != i->job_state) {
        if(old_reported_state != JOB_STATE_UNDEFINED) {
          // Report state change into log
          logger.msg(Arc::INFO,"%s: State: %s from %s",
                i->job_id.c_str(),JobDescription::get_state_name(i->job_state),
                JobDescription::get_state_name(old_reported_state));
        }
        old_reported_state=i->job_state;
      };
      if(state_changed) {
        state_changed=false;
        i->job_pending=false;
        if(!job_state_write_file(*i,*user,i->job_state)) {
          i->AddFailure("Failed writing job status: "+Arc::StrError(errno));
          job_error=true;
        } else {
          // talk to external plugin to ask if we can proceed
          if(plugins) {
            std::list<ContinuationPlugins::result_t> results;
            plugins->run(*i,*user,results);
            std::list<ContinuationPlugins::result_t>::iterator result = results.begin();
            while(result != results.end()) {
              // analyze results
              if(result->action == ContinuationPlugins::act_fail) {
                logger.msg(Arc::ERROR,"%s: Plugin at state %s : %s",
                    i->job_id.c_str(),states_all[i->get_state()].name,
                    result->response);
                i->AddFailure(std::string("Plugin at state ")+
                states_all[i->get_state()].name+" failed: "+(result->response));
                job_error=true;
              } else if(result->action == ContinuationPlugins::act_log) {
                // Scream but go ahead
                logger.msg(Arc::WARNING,"%s: Plugin at state %s : %s",
                    i->job_id.c_str(),states_all[i->get_state()].name,
                    result->response);
              } else if(result->action == ContinuationPlugins::act_pass) {
                // Just continue quietly
              } else {
                logger.msg(Arc::ERROR,"%s: Plugin execution failed",i->job_id);
                i->AddFailure(std::string("Failed running plugin at state ")+
                    states_all[i->get_state()].name);
                job_error=true;
              };
              ++result;
            };
          };
          // Processing to be done on state changes 
          user->Env().job_log().make_file(*i,*user);
          if(i->job_state == JOB_STATE_FINISHED) {
            if(i->GetLocalDescription(*user)) {
              job_stdlog_move(*i,*user,i->local->stdlog);
            };
            job_clean_finished(i->job_id,*user);
            user->Env().job_log().finish_info(*i,*user);
            prepare_cleanuptime(i->job_id,i->keep_finished,i,*user);
          } else if(i->job_state == JOB_STATE_PREPARING) {
            user->Env().job_log().start_info(*i,*user);
          };
        };
        /* send mail after error and change are processed */
        /* do not send if something really wrong happened to avoid email DoS */
        if(!delete_job) send_mail(*i,*user);
      };
      // Keep repeating till error goes out
    } while(job_error);
    if(delete_job) { 
      logger.msg(Arc::ERROR,"%s: Delete request due to internal problems",i->job_id);
      i->job_state = JOB_STATE_FINISHED; /* move to finished in order to 
                                            remove from list */
      if(i->GetLocalDescription(*user)) {
        if (--(jcfg.jobs_dn[i->local->DN]) == 0)
          jcfg.jobs_dn.erase(i->local->DN);
      }
      i->job_pending=false;
      job_state_write_file(*i,*user,i->job_state); 
      i->AddFailure("Serious troubles (problems during processing problems)");
      FailedJob(i);  /* put some marks */
      if(i->GetLocalDescription(*user)) {
        job_stdlog_move(*i,*user,i->local->stdlog);
      };
      job_clean_finished(i->job_id,*user);  /* clean status files */
      once_more=true; /* to process some things in local */
    };
  };
  /* FINISHED+DELETED jobs are not kept in list - only in files */
  /* if job managed to get here with state UNDEFINED - 
     means we are overloaded with jobs - do not keep them in list */
  if((i->job_state == JOB_STATE_FINISHED) ||
     (i->job_state == JOB_STATE_DELETED) ||
     (i->job_state == JOB_STATE_UNDEFINED)) {
    /* this is the ONLY place where jobs are removed from memory */
    /* update counters */
    if(!old_pending) {
      jcfg.jobs_num[old_state]--;
    } else {
      jcfg.jobs_pending--;
    };
    if(i->local) { delete i->local; };
    i=jobs.erase(i);
  }
  else {
    /* update counters */
    if(!old_pending) {
      jcfg.jobs_num[old_state]--;
    } else {
      jcfg.jobs_pending--;
    };
    if(!i->job_pending) {
      jcfg.jobs_num[i->job_state]++;
    } else {
      jcfg.jobs_pending++;
    }
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
  JobFDesc(const char* s,unsigned int n):id(s,n),uid(0),gid(0),t(-1) { };
  bool operator<(JobFDesc &right) { return (t < right.t); };
};

bool JobsList::RestartJobs(const std::string& cdir,const std::string& odir) {
  bool res = true;
  try {
    Glib::Dir dir(cdir);
    for(;;) {
      std::string file=dir.read_name();
      if(file.empty()) break;
      int l=file.length();
      if(l>(4+7)) {  /* job id contains at least 1 character */
        if(!strncmp(file.c_str(),"job.",4)) {
          if(!strncmp((file.c_str())+(l-7),".status",7)) {
            uid_t uid;
            gid_t gid;
            time_t t;
            std::string fname=cdir+'/'+file.c_str();
            std::string oname=odir+'/'+file.c_str();
            if(check_file_owner(fname,*user,uid,gid,t)) {
              if(::rename(fname.c_str(),oname.c_str()) != 0) {
                logger.msg(Arc::ERROR,"Failed to move file %s to %s",fname,oname);
                res=false;
              };
            };
          };
        };
      };
    };
    dir.close();
  } catch(Glib::FileError& e) {
    logger.msg(Arc::ERROR,"Failed reading control directory: %s",cdir);
    return false;
  };
  return res;
}

bool JobsList::RestartJob(const std::string& cdir,const std::string& odir,const std::string& id) {
  std::string file = "job." + id + ".status";
  std::string fname=cdir+'/'+file.c_str();
  std::string oname=odir+'/'+file.c_str();
  uid_t uid;
  gid_t gid;
  time_t t;
  if(check_file_owner(fname,*user,uid,gid,t)) {
    if(::rename(fname.c_str(),oname.c_str()) != 0) {
      logger.msg(Arc::ERROR,"Failed to move file %s to %s",fname,oname);
      return false;
    };
  };
  return true;
}

// This code is run at service restart
bool JobsList::RestartJobs(void) {
  std::string cdir=user->ControlDir();
  // Jobs from old version
  bool res1 = RestartJobs(cdir,cdir+"/restarting");
  // Jobs after service restart
  bool res2 = RestartJobs(cdir+"/processing",cdir+"/restarting");
  return res1 && res2;
}

bool JobsList::ScanJobs(const std::string& cdir,std::list<JobFDesc>& ids) {
  try {
    Glib::Dir dir(cdir);
    for(;;) {
      std::string file=dir.read_name();
      if(file.empty()) break;
      int l=file.length();
      if(l>(4+7)) {  /* job id contains at least 1 character */
        if(!strncmp(file.c_str(),"job.",4)) {
          if(!strncmp((file.c_str())+(l-7),".status",7)) {
            JobFDesc id((file.c_str())+4,l-7-4);
            if(FindJob(id.id) == jobs.end()) {
              std::string fname=cdir+'/'+file.c_str();
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
  } catch(Glib::FileError& e) {
    logger.msg(Arc::ERROR,"Failed reading control directory: %s",user->ControlDir());
    return false;
  };
  return true;
}

bool JobsList::ScanMarks(const std::string& cdir,const std::list<std::string>& suffices,std::list<JobFDesc>& ids) {
  try {
    Glib::Dir dir(cdir);
    for(;;) {
      std::string file=dir.read_name();
      if(file.empty()) break;
      int l=file.length();
      if(l>(4)) {  /* job id contains at least 1 character */
        if(!strncmp(file.c_str(),"job.",4)) {
          for(std::list<std::string>::const_iterator sfx = suffices.begin();
                       sfx != suffices.end();++sfx) {
            int ll = sfx->length();
            if(l > (ll+4)) {
              if(!strncmp(file.c_str()+(l-ll),sfx->c_str(),ll)) {
                JobFDesc id((file.c_str())+4,l-ll-4);
                if(FindJob(id.id) == jobs.end()) {
                  std::string fname=cdir+'/'+file.c_str();
                  uid_t uid;
                  gid_t gid;
                  time_t t;
                  if(check_file_owner(fname,*user,uid,gid,t)) {
                    /* add it to the list */
                    id.uid=uid; id.gid=gid; id.t=t;
                    ids.push_back(id);
                  };
                };
                break;
              };
            };
          };
        };
      };
    };
  } catch(Glib::FileError& e) {
    logger.msg(Arc::ERROR,"Failed reading control directory: %s",user->ControlDir());
    return false;
  };
  return true;
}

/* find new jobs - sort by date to implement FIFO */
bool JobsList::ScanNewJobs(void) {
  std::string cdir=user->ControlDir();
  std::list<JobFDesc> ids;
  // For picking up jobs after service restart
  std::string odir=cdir+"/restarting";
  if(!ScanJobs(odir,ids)) return false;
  // sorting by date
  ids.sort();
  for(std::list<JobFDesc>::iterator id=ids.begin();id!=ids.end();++id) {
    iterator i;
    AddJobNoCheck(id->id,i,id->uid,id->gid);
  }
  ids.clear();
  // For new jobs
  std::string ndir=cdir+"/accepting";
  if(!ScanJobs(ndir,ids)) return false;
  // sorting by date
  ids.sort();
  for(std::list<JobFDesc>::iterator id=ids.begin();id!=ids.end();++id) {
    iterator i;
    /* adding job with file's uid/gid */
    AddJobNoCheck(id->id,i,id->uid,id->gid);
  };
  return true;
}

bool JobsList::ScanOldJobs(int max_scan_time,int max_scan_jobs) {
  // We are going to scan a dir with a lot of
  // files here. So we scan it in parts and limit
  // scanning time.
  time_t start = time(NULL);
  if(max_scan_time < 10) max_scan_time=10; // some sane number - 10s
  std::string cdir=user->ControlDir()+"/finished";
  try {
    if(!old_dir) {
      old_dir = new Glib::Dir(cdir);
    };
    for(;;) {
      std::string file=old_dir->read_name();
      if(file.empty()) {
        old_dir->close();
        delete old_dir;
        old_dir=NULL;
        return false;
      };
      int l=file.length();
      if(l>(4+7)) {  /* job id contains at least 1 character */
        if(!strncmp(file.c_str(),"job.",4)) {
          if(!strncmp((file.c_str())+(l-7),".status",7)) {
            JobFDesc id((file.c_str())+4,l-7-4);
            if(FindJob(id.id) == jobs.end()) {
              std::string fname=cdir+'/'+file.c_str();
              uid_t uid;
              gid_t gid;
              time_t t;
              if(check_file_owner(fname,*user,uid,gid,t)) {
                /* add it to the list */
                id.uid=uid; id.gid=gid; id.t=t;
                job_state_t st = job_state_read_file(id.id,*user);
                if((st == JOB_STATE_FINISHED) || (st == JOB_STATE_DELETED)) {
                  iterator i;
                  AddJobNoCheck(id.id,i,id.uid,id.gid);
                  i->job_state = st;
                  --max_scan_jobs;
                };
              };
            };
          };
        };
      };
      if(((int)(time(NULL)-start)) >= max_scan_time) break;
      if(max_scan_jobs <= 0) break;
    };
  } catch(Glib::FileError& e) {
    logger.msg(Arc::ERROR,"Failed reading control directory: %s",cdir);
    if(old_dir) {
      old_dir->close();
      delete old_dir;
      old_dir=NULL;
    };
    return false;
  };
  return true;
}

bool JobsList::ScanNewMarks(void) {
  std::string cdir=user->ControlDir();
  std::string ndir=cdir+"/"+subdir_new;
  std::list<JobFDesc> ids;
  std::list<std::string> sfx;
  sfx.push_back(sfx_clean);
  sfx.push_back(sfx_restart);
  sfx.push_back(sfx_cancel);
  if(!ScanMarks(ndir,sfx,ids)) return false;
  ids.sort();
  std::string last_id;
  for(std::list<JobFDesc>::iterator id=ids.begin();id!=ids.end();++id) {
    if(id->id == last_id) continue; // already processed
    last_id = id->id;
    job_state_t st = job_state_read_file(id->id,*user);
    if((st == JOB_STATE_UNDEFINED) || (st == JOB_STATE_DELETED)) {
      // Job probably does not exist anymore
      job_clean_mark_remove(id->id,*user);
      job_restart_mark_remove(id->id,*user);
      job_cancel_mark_remove(id->id,*user);
    };
    // Check if such job finished and add it to list.
    if(st == JOB_STATE_FINISHED) {
      iterator i;
      AddJobNoCheck(id->id,i,id->uid,id->gid);
      // That will activate its processing at least for one step.
      i->job_state = st;
      //std::string fdir=cdir+"/"+subdir_old;
      //std::string odir=cdir+"/"+subdir_rew;
      //RestartJob(fdir,odir,*id);
    };
  };
  return true;
}

// For simply collecting all jobs. 
bool JobsList::ScanAllJobs(void) {
  std::list<std::string> subdirs;
  subdirs.push_back("/restarting"); // For picking up jobs after service restart
  subdirs.push_back("/accepting");  // For new jobs
  subdirs.push_back("/processing"); // For active jobs
  subdirs.push_back("/finished");   // For done jobs
  for(std::list<std::string>::iterator subdir = subdirs.begin();
                               subdir != subdirs.end();++subdir) {
    std::string cdir=user->ControlDir();
    std::list<JobFDesc> ids;
    std::string odir=cdir+(*subdir);
    if(!ScanJobs(odir,ids)) return false;
    // sorting by date
    ids.sort();
    for(std::list<JobFDesc>::iterator id=ids.begin();id!=ids.end();++id) {
      iterator i;
      AddJobNoCheck(id->id,i,id->uid,id->gid);
    }
  }
  return true;
}

