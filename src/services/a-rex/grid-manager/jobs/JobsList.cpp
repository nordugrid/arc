#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/stat.h>
#include <fcntl.h>

#include <arc/ArcLocation.h>
#include <arc/credential/VOMSUtil.h>

#include "../files/ControlFileHandling.h"
#include "../run/RunParallel.h"
#include "../mail/send_mail.h"
#include "../log/JobLog.h"
#include "../misc/proxy.h"
#include "../../delegation/DelegationStores.h"
#include "../../delegation/DelegationStore.h"
#include "../conf/GMConfig.h"

#include "ContinuationPlugins.h"
#include "DTRGenerator.h"
#include "JobsList.h"

namespace ARex {

/* max time to run submit-*-job/cancel-*-job before to
   start looking for alternative way to detect result.
   Only for protecting against lost child. */
#define CHILD_RUN_TIME_SUSPICIOUS (10*60)
/* max time to run submit-*-job/cancel-*-job before to
   decide that it is gone.
   Only for protecting against lost child. */
#define CHILD_RUN_TIME_TOO_LONG (60*60)

static Arc::Logger& logger = Arc::Logger::getRootLogger();

JobsList::JobsList(const GMConfig& config) :
    config(config), old_dir(NULL), dtr_generator(NULL), job_desc_handler(config), jobs_pending(0) {
  for(int n = 0;n<JOB_STATE_NUM;n++) jobs_num[n]=0;
  jobs.clear();
}
 
JobsList::iterator JobsList::FindJob(const JobId &id){
  iterator i;
  for(i=jobs.begin();i!=jobs.end();++i) {
    if((*i) == id) break;
  }
  return i;
}

bool JobsList::AddJobNoCheck(const JobId &id,uid_t uid,gid_t gid){
  iterator i;
  return AddJobNoCheck(id,i,uid,gid);
}

bool JobsList::AddJobNoCheck(const JobId &id,JobsList::iterator &i,uid_t uid,gid_t gid){
  i=jobs.insert(jobs.end(),GMJob(id, Arc::User(uid)));
  i->keep_finished=config.keep_finished;
  i->keep_deleted=config.keep_deleted;
  if (!GetLocalDescription(i)) {
    // safest thing to do is add failure and move to FINISHED
    i->AddFailure("Internal error");
    i->job_state = JOB_STATE_FINISHED;
    FailedJob(i, false);
    if(!job_state_write_file(*i,config,i->job_state)) {
      logger.msg(Arc::ERROR, "%s: Failed reading .local and changing state, job and "
                             "A-REX may be left in an inconsistent state", id);
    }
    return false;
  }
  i->session_dir = i->local->sessiondir;
  if (i->session_dir.empty()) i->session_dir = config.SessionRoot(id)+'/'+id;
  return true;
}

int JobsList::AcceptedJobs() const {
  return jobs_num[JOB_STATE_ACCEPTED] +
         jobs_num[JOB_STATE_PREPARING] +
         jobs_num[JOB_STATE_SUBMITTING] +
         jobs_num[JOB_STATE_INLRMS] +
         jobs_num[JOB_STATE_FINISHING] +
         jobs_pending;
}

int JobsList::RunningJobs() const {
  return jobs_num[JOB_STATE_SUBMITTING] +
         jobs_num[JOB_STATE_INLRMS];
}

int JobsList::ProcessingJobs() const {
  return jobs_num[JOB_STATE_PREPARING] +
         jobs_num[JOB_STATE_FINISHING];
}

int JobsList::PreparingJobs() const {
  return jobs_num[JOB_STATE_PREPARING];
}

int JobsList::FinishingJobs() const {
  return jobs_num[JOB_STATE_FINISHING];
}

void JobsList::ChooseShare(JobsList::iterator& i) {
  // only applies to old staging
  if (config.use_dtr || config.share_type.empty()) return;
  std::string user_proxy_file = job_proxy_filename(i->get_id(), config);
  std::string cert_dir = "/etc/grid-security/certificates";
  if (!config.cert_dir.empty()) cert_dir = config.cert_dir;
  Arc::Credential u(user_proxy_file,"",cert_dir,"");
  const std::string share = getCredentialProperty(u,config.share_type);
  i->set_share(share);
  logger.msg(Arc::INFO, "%s: adding to transfer share %s",i->get_id(),i->transfer_share);
  i->local->transfershare = i->transfer_share;
  job_local_write_file(*i,config,*i->local);
}

void JobsList::CalculateShares(){
  // transfer share calculation - look at current share for preparing and
  // finishing states, then look at jobs ready to go into preparing or finished
  // and set the share. Need to do it here because in the ActJob* methods we
  // don't have an overview of all jobs. In those methods we check the share to
  // see if each job can proceed. This method is only used for old
  // (down/uploader) data staging. With DTR it is handled internally.
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
      if ((job_lrms_mark_check(i->job_id,config) || i->job_pending) && i->next_retry <= time(NULL)) {
        pre_finishing_job_share[i->transfer_share]++;
      }
    }
  }
  
  // Now calculate how many of limited transfer shares are active
  // We need to try to preserve the maximum number of transfer threads 
  // for each active limited share. Jobs that belong to limited 
  // shares will be excluded from calculation of a share limit later
  int privileged_total_pre_preparing = 0;
  int privileged_total_pre_finishing = 0;
  int privileged_jobs_processing = 0;
  int privileged_preparing_job_share = 0;
  int privileged_finishing_job_share = 0;
  std::map<std::string, int> limited_shares = config.limited_share;
  for (std::map<std::string, int>::iterator i = limited_shares.begin(); i != limited_shares.end(); i++) {
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
  int unprivileged_jobs_processing = config.max_jobs_staging - privileged_jobs_processing;

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
  if (config.max_jobs_staging == -1 || unprivileged_preparing_job_share <= (unprivileged_jobs_processing / config.max_staging_share))
    unprivileged_preparing_limit = config.max_staging_share;
  else if (unprivileged_preparing_job_share > unprivileged_jobs_processing || unprivileged_preparing_job_share <= 0)
    unprivileged_preparing_limit = 1;
  else if (total_pre_preparing <= unprivileged_jobs_processing)
    unprivileged_preparing_limit = config.max_staging_share;
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
  if (config.max_jobs_staging == -1 || unprivileged_finishing_job_share <= (unprivileged_jobs_processing / config.max_staging_share))
    unprivileged_finishing_limit = config.max_staging_share;
  else if (unprivileged_finishing_job_share > unprivileged_jobs_processing || unprivileged_finishing_job_share <= 0)
    unprivileged_finishing_limit = 1;
  else if (total_pre_finishing <= unprivileged_jobs_processing)
    unprivileged_finishing_limit = config.max_staging_share;
  else
    unprivileged_finishing_limit = unprivileged_jobs_processing / unprivileged_finishing_job_share;

  // if there are queued jobs for both preparing and finishing, split the share between the two states
  if (config.max_jobs_staging > 0 && total_pre_preparing > unprivileged_jobs_processing/2 && total_pre_finishing > unprivileged_jobs_processing/2) {
    unprivileged_preparing_limit = unprivileged_preparing_limit < 2 ? 1 : unprivileged_preparing_limit/2;
    unprivileged_finishing_limit = unprivileged_finishing_limit < 2 ? 1 : unprivileged_finishing_limit/2;
  }

  if (config.max_jobs_staging > 0 && privileged_total_pre_preparing > privileged_jobs_processing/2 && privileged_total_pre_finishing > privileged_jobs_processing/2) {
    for (std::map<std::string, int>::iterator i = limited_shares.begin(); i != limited_shares.end(); i++) {
      i->second = i->second < 2 ? 1 : i->second/2;
    }
  }
      
  preparing_max_share = pre_preparing_job_share;
  finishing_max_share = pre_finishing_job_share;
  for (std::map<std::string, int>::iterator i = preparing_max_share.begin(); i != preparing_max_share.end(); i++){
    if (limited_shares.find(i->first) != limited_shares.end())
      i->second = limited_shares[i->first];
    else
      i->second = unprivileged_preparing_limit;
  }
  for (std::map<std::string, int>::iterator i = finishing_max_share.begin(); i != finishing_max_share.end(); i++){
    if (limited_shares.find(i->first) != limited_shares.end())
      i->second = limited_shares[i->first];
    else
      i->second = unprivileged_finishing_limit;
  }
}

void JobsList::PrepareToDestroy(void) {
  for(iterator i=jobs.begin();i!=jobs.end();++i) {
    i->PrepareToDestroy();
  }
}

bool JobsList::ActJobs(void) {

  // Need to calculate the shares here here because in the ActJob* methods we
  // don't have an overview of all jobs. In those methods we check the share
  // to see if each job can proceed.
  if (!config.share_type.empty() && config.max_staging_share > 0) {
    CalculateShares();
  }

  bool res = true;
  bool once_more = false;
  bool postpone_preparing = false;
  bool postpone_finishing = false;
  if (!(config.use_dtr && dtr_generator)) {
    if((config.max_jobs_staging != -1) &&
       (!config.use_local_transfer) &&
       ((ProcessingJobs()*3) > (config.max_jobs_staging*2))) {
      if(PreparingJobs() > FinishingJobs()) {
        postpone_preparing=true; 
      } else if(PreparingJobs() < FinishingJobs()) {
        postpone_finishing=true;
      }
    }
  }
  // first pass - optionally skipping some states
  for(iterator i=jobs.begin();i!=jobs.end();) {
    if(i->job_state == JOB_STATE_UNDEFINED) { once_more=true; }
    else if(((i->job_state == JOB_STATE_ACCEPTED) && postpone_preparing) ||
            ((i->job_state == JOB_STATE_INLRMS) && postpone_finishing)  ) {
      once_more=true;
      i++; continue;
    }
    res &= ActJob(i);
  }

  // Recalculation of the shares before the second pass to update the shares
  // that appeared as a result of moving some jobs to ACCEPTED during the first
  // pass.
  if (!config.share_type.empty() && config.max_staging_share > 0) {
    CalculateShares();
  }

  // second pass - process skipped states and new jobs
  if(once_more) for(iterator i=jobs.begin();i!=jobs.end();) {
    res &= ActJob(i);
  }

  // debug info on jobs per DN
  logger.msg(Arc::VERBOSE, "Current jobs in system (PREPARING to FINISHING) per-DN (%i entries)", jobs_dn.size());
  for (std::map<std::string, ZeroUInt>::iterator it = jobs_dn.begin(); it != jobs_dn.end(); ++it)
    logger.msg(Arc::VERBOSE, "%s: %i", it->first, (unsigned int)(it->second));

  return res;
}

bool JobsList::DestroyJob(JobsList::iterator &i,bool finished,bool active) {
  logger.msg(Arc::INFO,"%s: Destroying",i->job_id);
  job_state_t new_state=i->job_state;
  if(new_state == JOB_STATE_UNDEFINED) {
    if((new_state=job_state_read_file(i->job_id,config))==JOB_STATE_UNDEFINED) {
      logger.msg(Arc::ERROR,"%s: Can't read state - no comments, just cleaning",i->job_id);
      UnlockDelegation(i);
      job_clean_final(*i,config);
      if(i->local) delete i->local;
      i=jobs.erase(i);
      return true;
    }
  }
  i->job_state = new_state;
  if((new_state == JOB_STATE_FINISHED) && (!finished)) { ++i; return true; }
  if(!active) { ++i; return true; }
  if((new_state != JOB_STATE_INLRMS) || 
     (job_lrms_mark_check(i->job_id,config))) {
    logger.msg(Arc::INFO,"%s: Cleaning control and session directories",i->job_id);
    UnlockDelegation(i);
    job_clean_final(*i,config);
    if(i->local) delete i->local;
    i=jobs.erase(i);
    return true;
  }
  logger.msg(Arc::INFO,"%s: This job may be still running - canceling",i->job_id);
  bool state_changed = false;
  if(!state_submitting(i,state_changed,true)) {
    logger.msg(Arc::WARNING,"%s: Cancelation failed (probably job finished) - cleaning anyway",i->job_id);
    UnlockDelegation(i);
    job_clean_final(*i,config);
    if(i->local) delete i->local;
    i=jobs.erase(i);
    return true;
  }
  if(!state_changed) { ++i; return false; } // child still running
  logger.msg(Arc::INFO,"%s: Cancelation probably succeeded - cleaning",i->job_id);
  UnlockDelegation(i);
  job_clean_final(*i,config);
  if(i->local) delete i->local;
  i=jobs.erase(i);
  return true;
}

bool JobsList::FailedJob(const JobsList::iterator &i,bool cancel) {
  bool r = true;
  // add failure mark
  if(job_failed_mark_add(*i,config,i->failure_reason)) {
    i->failure_reason = "";
  } else {
    r = false;
  }
  if(GetLocalDescription(i)) {
    i->local->uploads=0;
  } else {
    r=false;
  }
  // If the job failed during FINISHING then uploader or DTR deals with .output
  // The exception is cancelling uploader since process is simply killed and
  // does not get the chance to change .output.
  if (i->get_state() == JOB_STATE_FINISHING && (!cancel || dtr_generator)) {
    if (i->local) job_local_write_file(*i,config,*(i->local));
    return r;
  }
  // adjust output files to failure state
  // Not good looking code
  JobLocalDescription job_desc;
  if(job_desc_handler.parse_job_req(i->get_id(),job_desc) != JobReqSuccess) {
    r = false;
  }
  // Convert delegation ids to credential paths.
  std::string default_cred = config.control_dir + "/job." + i->get_id() + ".proxy";
  for(std::list<FileData>::iterator f = job_desc.outputdata.begin();
                                   f != job_desc.outputdata.end(); ++f) {
    if(f->has_lfn()) {
      if(f->cred.empty()) {
        f->cred = default_cred;
      } else {
        std::string path;
        ARex::DelegationStores* delegs = config.delegations;
        if(delegs && i->local) path = (*delegs)[config.DelegationDir()].FindCred(f->cred,i->local->DN);
        f->cred = path;
      }
      if(i->local) ++(i->local->uploads);
    }
  }
  // Add user-uploaded input files so that they are not deleted during
  // FINISHING and so resume will work. Credentials are not necessary for
  // these files. The real output list will be recreated from the job
  // description if the job is restarted.
  if (!cancel && job_desc.reruns > 0) {
    for(std::list<FileData>::iterator f = job_desc.inputdata.begin();
                                      f != job_desc.inputdata.end(); ++f) {
      if (f->lfn.find(':') == std::string::npos) {
        FileData fd(f->pfn, "");
        fd.iffailure = true; // make sure to keep file
        job_desc.outputdata.push_back(fd);
      }
    }
  }
  if(!job_output_write_file(*i,config,job_desc.outputdata,cancel?job_output_cancel:job_output_failure)) {
    r=false;
    logger.msg(Arc::ERROR,"%s: Failed writing list of output files: %s",i->job_id,Arc::StrError(errno));
  }
  if(i->local) job_local_write_file(*i,config,*(i->local));
  return r;
}

bool JobsList::GetLocalDescription(const JobsList::iterator &i) {
  if(!i->GetLocalDescription(config)) {
    logger.msg(Arc::ERROR,"%s: Failed reading local information",i->job_id);
    return false;
  }
  return true;
}

bool JobsList::state_submitting(const JobsList::iterator &i,bool &state_changed,bool cancel) {
  if(i->child == NULL) {
    // no child was running yet, or recovering from fault 
    // write grami file for submit-X-job
    // TODO: read existing grami file to check if job is already submitted
    JobLocalDescription* job_desc;
    if(i->local) { job_desc=i->local; }
    else {
      job_desc=new JobLocalDescription;
      if(!job_local_read_file(i->job_id,config,*job_desc)) {
        logger.msg(Arc::ERROR,"%s: Failed reading local information",i->job_id);
        if(!cancel) i->AddFailure("Internal error: can't read local file");
        delete job_desc;
        return false;
      }
      i->local=job_desc;
    }
    if(!cancel) {  // in case of cancel all preparations are already done
      const char *local_transfer_s = NULL;
      if(config.use_local_transfer) {
        local_transfer_s="joboption_localtransfer=yes";
      }
      if(!job_desc_handler.write_grami(*i,local_transfer_s)) {
        logger.msg(Arc::ERROR,"%s: Failed creating grami file",i->job_id);
        return false;
      }
      if(!job_desc_handler.set_execs(*i)) {
        logger.msg(Arc::ERROR,"%s: Failed setting executable permissions",i->job_id);
        return false;
      }
      // precreate file to store diagnostics from lrms
      job_diagnostics_mark_put(*i,config);
      job_lrmsoutput_mark_put(*i,config);
    }
    // submit/cancel job to LRMS using submit/cancel-X-job
    std::string cmd;
    if(cancel) { cmd=Arc::ArcLocation::GetDataDir()+"/cancel-"+job_desc->lrms+"-job"; }
    else { cmd=Arc::ArcLocation::GetDataDir()+"/submit-"+job_desc->lrms+"-job"; }
    if(!cancel) {
      logger.msg(Arc::INFO,"%s: state SUBMIT: starting child: %s",i->job_id,cmd);
    } else {
      if(!job_lrms_mark_check(i->job_id,config)) {
        logger.msg(Arc::INFO,"%s: state CANCELING: starting child: %s",i->job_id,cmd);
      } else {
        logger.msg(Arc::INFO,"%s: Job has completed already. No action taken to cancel",i->job_id);
        state_changed=true;
        return true;
      }
    }
    std::string grami = config.control_dir+"/job."+(*i).job_id+".grami";
    cmd += " --config " + config.conffile + " " + grami;
    job_errors_mark_put(*i,config);
    if(!RunParallel::run(config,*i,cmd,&(i->child))) {
      if(!cancel) {
        i->AddFailure("Failed initiating job submission to LRMS");
        logger.msg(Arc::ERROR,"%s: Failed running submission process",i->job_id);
      } else {
        logger.msg(Arc::ERROR,"%s: Failed running cancellation process",i->job_id);
      }
      return false;
    }
    return true;
  }
  // child was run - check if exited and then exit code
  bool simulate_success = false;
  if(i->child->Running()) {
    // child is running - come later
    // Due to unknown reason sometimes child exit event is lost.
    // As workaround check if child is running for too long. If
    // it does then check in grami file for generated local id
    // or in case of cancel just assume child exited.
    if((Arc::Time() - i->child->RunTime()) > Arc::Period(CHILD_RUN_TIME_SUSPICIOUS)) {
      if(!cancel) {
        // Check if local id is already obtained
        std::string local_id=job_desc_handler.get_local_id(i->job_id);
        if(local_id.length() > 0) {
          simulate_success = true;
          logger.msg(Arc::ERROR,"%s: Job submission to LRMS takes too long, but ID is already obtained. Pretending submission is done.",i->job_id);
        }
      } else {
        // Check if diagnostics collection is done
        if(job_lrms_mark_check(i->job_id,config)) {
          simulate_success = true;
          logger.msg(Arc::ERROR,"%s: Job cancellation takes too long, but diagnostic collection seems to be done. Pretending cancellation succeeded.",i->job_id);
        }
      }
    }
    if((!simulate_success) && (Arc::Time() - i->child->RunTime()) > Arc::Period(CHILD_RUN_TIME_TOO_LONG)) {
      // In any case it is way too long. Job must fail. Otherwise it will hang forever.
      delete i->child; i->child=NULL;
      if(!cancel) {
        logger.msg(Arc::ERROR,"%s: Job submission to LRMS takes too long. Failing.",i->job_id);
        JobFailStateRemember(i,JOB_STATE_SUBMITTING);
        i->AddFailure("Job submission to LRMS failed");
        // It would be nice to cancel if job finally submits. But we do not know id.
        return false;
      } else {
        logger.msg(Arc::ERROR,"%s: Job cancellation takes too long. Failing.",i->job_id);
        delete i->child; i->child=NULL;
        return false;
      }
    }
    if(!simulate_success) return true;
  }
  if(!simulate_success) {
    // real processing
    if(!cancel) {
      logger.msg(Arc::INFO,"%s: state SUBMIT: child exited with code %i",i->job_id,i->child->Result());
    } else {
      if((i->child->ExitTime() != Arc::Time::UNDEFINED) &&
         ((Arc::Time() - i->child->ExitTime()) < (config.wakeup_period*2))) {
        // not ideal solution
        logger.msg(Arc::INFO,"%s: state CANCELING: child exited with code %i",i->job_id,i->child->Result());
      }
    }
    // Another workaround in Run class may also detect lost child.
    // It then sets exit code to -1. This value is also set in
    // case child was killed. So it is worth to check grami anyway.
    if((i->child->Result() != 0) && (i->child->Result() != -1)) {
      if(!cancel) {
        logger.msg(Arc::ERROR,"%s: Job submission to LRMS failed",i->job_id);
        JobFailStateRemember(i,JOB_STATE_SUBMITTING);
      } else {
        logger.msg(Arc::ERROR,"%s: Failed to cancel running job",i->job_id);
      }
      delete i->child; i->child=NULL;
      if(!cancel) i->AddFailure("Job submission to LRMS failed");
      return false;
    }
  } else {
    // Just pretend everything is alright
  }
  if(!cancel) {
    delete i->child; i->child=NULL;
    // success code - get LRMS job id
    std::string local_id=job_desc_handler.get_local_id(i->job_id);
    if(local_id.length() == 0) {
      logger.msg(Arc::ERROR,"%s: Failed obtaining lrms id",i->job_id);
      i->AddFailure("Failed extracting LRMS ID due to some internal error");
      JobFailStateRemember(i,JOB_STATE_SUBMITTING);
      return false;
    }
    // put id into local information file
    if(!GetLocalDescription(i)) {
      i->AddFailure("Internal error");
      return false;
    }
    i->local->localid=local_id;
    if(!job_local_write_file(*i,config,*(i->local))) {
      i->AddFailure("Internal error");
      logger.msg(Arc::ERROR,"%s: Failed writing local information: %s",i->job_id,Arc::StrError(errno));
      return false;
    }
  } else {
    // job diagnostics collection done in background (scan-*-job script)
    if(!job_lrms_mark_check(i->job_id,config)) {
      // job diag not yet collected - come later
      if((i->child->ExitTime() != Arc::Time::UNDEFINED) &&
         ((Arc::Time() - i->child->ExitTime()) > Arc::Period(Arc::Time::HOUR))) {
        // it takes too long
        logger.msg(Arc::ERROR,"%s: state CANCELING: timeout waiting for cancellation",i->job_id);
        delete i->child; i->child=NULL;
        return false;
      }
      return true;
    } else {
      logger.msg(Arc::INFO,"%s: state CANCELING: job diagnostics collected",i->job_id);
      delete i->child; i->child=NULL;
      job_diagnostics_mark_move(*i,config);
    }
  }
  // move to next state
  state_changed=true;
  return true;
}

bool JobsList::state_loading(const JobsList::iterator &i,bool &state_changed,bool up,bool &retry) {

  if (config.use_dtr && dtr_generator) {  /***** new data staging ******/

    if (config.use_local_transfer) {
      // just check user-uploaded files for PREPARING jobs
      if (up) {
        state_changed = true;
        return true;
      }
      int res = dtr_generator->checkUploadedFiles(*i);
      if (res == 2) { // still going
        return true;
      }
      if (res == 0) { // finished successfully
        state_changed=true;
        return true;
      }
      // error
      return false;
    }

    // first check if job is already in the system
    if (!dtr_generator->hasJob(*i)) {
      dtr_generator->receiveJob(*i);
      return true;
    }
    // if job has already failed then do not set failed state again if DTR failed
    bool already_failed = !i->GetFailure(config).empty();
    // queryJobFinished() calls i->AddFailure() if any DTR failed
    if (dtr_generator->queryJobFinished(*i)) {

      bool done = true;
      bool result = true;

      // check for failure
      if (!i->GetFailure(config).empty()) {
        if (!already_failed) JobFailStateRemember(i, (up ? JOB_STATE_FINISHING : JOB_STATE_PREPARING));
        result = false;
      }
      else if (!up) { // check for user-uploadable files if downloading
        int res = dtr_generator->checkUploadedFiles(*i);
        if (res == 2) { // still going
          done = false;
        }
        else if (res == 0) { // finished successfully
          state_changed=true;
        }
        else { // error
          result = false;
        }
      }
      else { // if uploading we are done
        state_changed = true;
      }
      if (done) dtr_generator->removeJob(*i);
      return result;
    }
    else {
      // not finished yet
      logger.msg(Arc::VERBOSE, "%s: State: %s: still in data staging", i->job_id, (up ? "FINISHING" : "PREPARING"));
      return true;
    }
  }
  else {  /*************** old downloader/uploader *********************/

    // Job description was analyzed/parsed - now run child process downloader
    // to download job input files and to wait for user uploaded ones
    if(i->child == NULL) { // no child started
      logger.msg(Arc::INFO,"%s: state: %s: starting new child",i->job_id,up?"FINISHING":"PREPARING");
      // no child was running yet, or recovering from fault
      // run it anyway and exit code will give more inforamtion
      std::string cmd_name;
      if(up) { cmd_name=Arc::ArcLocation::GetToolsDir()+"/uploader"; }
      else { cmd_name=Arc::ArcLocation::GetToolsDir()+"/downloader"; }
      std::string user_id = Arc::tostring(i->get_user().get_uid());
      std::string cmd = cmd_name + " -U " + user_id + " -f";
      if(config.max_downloads > 0) {
        cmd += " -n " + Arc::tostring(config.max_downloads);
      }
      if(!config.use_secure_transfer) {
        cmd += " -c";
      }
      if(config.use_passive_transfer) {
        cmd += " -p";
      }
      if(config.use_local_transfer) {
        cmd += " -l";
      }
      if(config.min_speed) {
        cmd += " -s " + Arc::tostring(config.min_speed);
        cmd += " -S " + Arc::tostring(config.min_speed_time);
      }
      if(config.min_average_speed) {
        cmd += " -a " + Arc::tostring(config.min_average_speed);
      }
      if(config.max_inactivity_time) {
        cmd += " -i " + Arc::tostring(config.max_inactivity_time);
      }
      std::string debug_level = Arc::level_to_string(Arc::Logger::getRootLogger().getThreshold());
      if (!debug_level.empty()) {
        cmd += " -d " + debug_level;
      }
      if (!config.conffile.empty()) {
        cmd += " -C " + config.conffile;
      }
      if(!config.preferred_pattern.empty()) {
        cmd += " -r " + config.preferred_pattern;
      }
      cmd += " " + i->job_id;
      cmd += " " + config.control_dir;
      cmd += " " + i->session_dir;

      logger.msg(Arc::INFO,"%s: State %s: starting child: %s",i->job_id,up?"FINISHING":"PREPARING",cmd_name);
      job_errors_mark_put(*i,config);
      // Remove restart mark because restart point may change. Keep it if we are
      // already processing failed job.
      if(!job_failed_mark_check(i->job_id,config)) job_restart_mark_remove(i->job_id,config);
      if(!RunParallel::run(config,*i,cmd,&(i->child),config.strict_session)) {
        if(up) {
          logger.msg(Arc::ERROR,"%s: Failed to run uploader process",i->job_id);
          i->AddFailure("Failed to run uploader (post-processing)");
        } else {
          logger.msg(Arc::ERROR,"%s: Failed to run downloader process",i->job_id);
          i->AddFailure("Failed to run downloader (pre-processing)");
        }
        return false;
      }
    } else {
      if(i->child->Running()) {
        logger.msg(Arc::VERBOSE,"%s: State: PREPARING/FINISHING: child is running",i->job_id);
        // child is running - come later
        return true;
      }
      // child was run - check exit code
      if(!up) { logger.msg(Arc::INFO,"%s: State: PREPARING: child exited with code: %i",i->job_id,i->child->Result()); }
      else { logger.msg(Arc::INFO,"%s: State: FINISHING: child exited with code: %i",i->job_id,i->child->Result()); }
      if(i->child->Result() != 0) {
        if(i->child->Result() == 1) {
          // unrecoverable failure detected - all we can do is to kill the job
          if(up) {
            logger.msg(Arc::ERROR,"%s: State: FINISHING: unrecoverable error detected (exit code 1)",i->job_id);
            i->AddFailure("Failed in files upload (post-processing)");
          } else {
            logger.msg(Arc::ERROR,"%s: State: PREPARING: unrecoverable error detected (exit code 1)",i->job_id);
            i->AddFailure("Failed in files download (pre-processing)");
          }
        } else if(i->child->Result() == 4) { // retryable error
          logger.msg(Arc::DEBUG, "%s: State: PREPARING/FINISHING: retryable error", i->job_id);
          delete i->child; i->child=NULL;
          retry = true;
          return true;
        }
        else {
          // Because we have no information (anymore) if failure happened
          // due to expired credentials let's just check them
          std::string old_proxy_file =
                      config.control_dir+"/job."+i->job_id+".proxy";
          Arc::Credential cred(old_proxy_file,"","","");
          // TODO: check if cred is at all there
          if(cred.GetEndTime() < Arc::Time()) {
            // Credential is expired
            logger.msg(Arc::ERROR,"%s: State: %s: credentials probably expired (exit code %i)",i->job_id,up?"FINISHING":"PREPARING",i->child->Result());
            // in case of expired credentials there is a chance to get them
            // from credentials server - so far myproxy only
            if(GetLocalDescription(i)) {
              if(i->local->credentialserver.length()) {
                std::string new_proxy_file = config.control_dir+"/job."+i->job_id+".proxy.tmp";
                remove(new_proxy_file.c_str());
                int h = ::open(new_proxy_file.c_str(),
                        O_WRONLY | O_CREAT | O_EXCL,S_IRUSR | S_IWUSR);
                if(h!=-1) {
                  close(h);
                  logger.msg(Arc::INFO,"%s: State: %s: trying to renew credentials",i->job_id,up?"FINISHING":"PREPARING");
                  if(myproxy_renew(old_proxy_file.c_str(),new_proxy_file.c_str(),
                          i->local->credentialserver.c_str())) {
                    renew_proxy(old_proxy_file.c_str(),new_proxy_file.c_str());
                    // imitate rerun request
                    job_restart_mark_put(*i,config);
                  } else {
                    logger.msg(Arc::ERROR,"%s: State: %s: failed to renew credentials",i->job_id,up?"FINISHING":"PREPARING");
                  }
                } else {
                  logger.msg(Arc::ERROR,"%s: State: %s: failed to create temporary proxy for renew: %s",i->job_id,up?"FINISHING":"PREPARING",new_proxy_file);
                }
              }
            } else {
              i->AddFailure("Internal error");
            }
            if(up) {
              i->AddFailure("Failed in files upload probably due to expired credentials - try to renew");
            } else {
              i->AddFailure("Failed in files download probably due to expired credentials - try to renew");
            }
          } else {
            // Credentials were alright
            logger.msg(Arc::ERROR,"%s: State: %s: some error detected (exit code %i). Recover from such type of errors is not supported yet.",i->job_id,up?"FINISHING":"PREPARING",i->child->Result());
            if(up) {
              i->AddFailure("Failed in files upload (post-processing)");
            } else {
              i->AddFailure("Failed in files download (pre-processing)");
            }
          }
        }
        delete i->child; i->child=NULL;
        JobFailStateRemember(i, (up ? JOB_STATE_FINISHING : JOB_STATE_PREPARING));
        return false;
      }
      // success code - move to next state
      state_changed=true;
      delete i->child; i->child=NULL;
    }
  }
  return true;
}

bool JobsList::CanStage(const JobsList::iterator &i, bool up) {

  // new data staging - all jobs can proceed immediately
  if (config.use_dtr && dtr_generator) return true;
  // transfer is done on worker nodes
  if (config.use_local_transfer) return true;
  // nothing to stage
  if (!up && (i->local->downloads == 0)) return true;
  if (up && (i->local->uploads == 0)) return true;
  // before checking limits, check the retry time
  if (i->next_retry > time(NULL)) return false;
  // no limit on staging jobs
  if (config.max_jobs_staging == -1) return true;
  if (!up) {
    // limits on downloads
    if (((ProcessingJobs() < config.max_jobs_staging) ||
        ((FinishingJobs() >= config.max_jobs_staging) &&
         (PreparingJobs() < config.max_jobs_staging_emergency))) &&
        (config.share_type.empty() ||
         preparing_job_share[i->transfer_share] < preparing_max_share[i->transfer_share])) return true;
  } else {
    // limits on uploads
    if (((ProcessingJobs() < config.max_jobs_staging) ||
        ((PreparingJobs() >= config.max_jobs_staging) &&
         (FinishingJobs() < config.max_jobs_staging_emergency))) &&
        (config.share_type.empty() ||
         finishing_job_share[i->transfer_share] < finishing_max_share[i->transfer_share])) return true;
  }
  return false;
}

bool JobsList::JobPending(JobsList::iterator &i) {
  if(i->job_pending) return true;
  i->job_pending=true; 
  return job_state_write_file(*i,config,i->job_state,true);
}

job_state_t JobsList::JobFailStateGet(const JobsList::iterator &i) {
  if(!GetLocalDescription(i)) {
    return JOB_STATE_UNDEFINED;
  }
  if(i->local->failedstate.empty()) { return JOB_STATE_UNDEFINED; }
  for(int n = 0;states_all[n].name != NULL;n++) {
    if(i->local->failedstate == states_all[n].name) {
      if(i->local->reruns <= 0) {
        logger.msg(Arc::ERROR,"%s: Job is not allowed to be rerun anymore",i->job_id);
        job_local_write_file(*i,config,*(i->local));
        return JOB_STATE_UNDEFINED;
      }
      i->local->failedstate="";
      i->local->failedcause="";
      i->local->reruns--;
      job_local_write_file(*i,config,*(i->local));
      return states_all[n].id;
    }
  }
  logger.msg(Arc::ERROR,"%s: Job failed in unknown state. Won't rerun.",i->job_id);
  i->local->failedstate="";
  i->local->failedcause="";
  job_local_write_file(*i,config,*(i->local));
  return JOB_STATE_UNDEFINED;
}

bool JobsList::RecreateTransferLists(const JobsList::iterator &i) {
  // Recreate list of output and input files, excluding those already
  // transferred. For input files this is done by looking at the session dir,
  // for output files by excluding files in .output_status
  std::list<FileData> output_files;
  std::list<FileData> output_files_done;
  std::list<FileData> input_files;
  // keep local info
  if(!GetLocalDescription(i)) return false;
  // get output files already done
  job_output_status_read_file(i->job_id,config,output_files_done);
  // recreate lists by reprocessing job description
  JobLocalDescription job_desc; // placeholder
  if(!job_desc_handler.process_job_req(*i,job_desc)) {
    logger.msg(Arc::ERROR,"%s: Reprocessing job description failed",i->job_id);
    return false;
  }
  // Restore 'local'
  if(!job_local_write_file(*i,config,*(i->local))) return false;
  // Read new lists
  if(!job_output_read_file(i->job_id,config,output_files)) {
    logger.msg(Arc::ERROR,"%s: Failed to read reprocessed list of output files",i->job_id);
    return false;
  }
  if(!job_input_read_file(i->job_id,config,input_files)) {
    logger.msg(Arc::ERROR,"%s: Failed to read reprocessed list of input files",i->job_id);
    return false;
  }
  // remove already uploaded files
  i->local->uploads=0;
  for(std::list<FileData>::iterator i_new = output_files.begin();
                                    i_new!=output_files.end();) {
    if(!(i_new->has_lfn())) { // user file - keep
      ++i_new;
      continue;
    }
    std::list<FileData>::iterator i_done = output_files_done.begin();
    for(;i_done!=output_files_done.end();++i_done) {
      if((i_new->pfn == i_done->pfn) && (i_new->lfn == i_done->lfn)) break;
    }
    if(i_done == output_files_done.end()) {
      ++i_new;
      i->local->uploads++;
      continue;
    }
    i_new=output_files.erase(i_new);
  }
  if(!job_output_write_file(*i,config,output_files)) return false;
  // remove already downloaded files
  i->local->downloads=0;
  for(std::list<FileData>::iterator i_new = input_files.begin();
                                    i_new!=input_files.end();) {
    std::string path = i->session_dir+"/"+i_new->pfn;
    struct stat st;
    if(::stat(path.c_str(),&st) == -1) {
      ++i_new;
      i->local->downloads++;
    } else {
      i_new=input_files.erase(i_new);
    }
  }
  if(!job_input_write_file(*i,config,input_files)) return false;
  return true;
}

bool JobsList::JobFailStateRemember(const JobsList::iterator &i,job_state_t state,bool internal) {
  if(!(i->local)) {
    JobLocalDescription *job_desc = new JobLocalDescription;
    if(!job_local_read_file(i->job_id,config,*job_desc)) {
      logger.msg(Arc::ERROR,"%s: Failed reading local information",i->job_id);
      delete job_desc; return false;
    }
    else {
      i->local=job_desc;
    }
  }
  if(state == JOB_STATE_UNDEFINED) {
    i->local->failedstate="";
    i->local->failedcause=internal?"internal":"client";
    return job_local_write_file(*i,config,*(i->local));
  }
  if(i->local->failedstate.empty()) {
    i->local->failedstate=states_all[state].name;
    i->local->failedcause=internal?"internal":"client";
    return job_local_write_file(*i,config,*(i->local));
  }
  return true;
}

time_t JobsList::PrepareCleanupTime(JobsList::iterator &i,time_t& keep_finished) {
  JobLocalDescription job_desc;
  time_t t = -1;
  // read lifetime - if empty it wont be overwritten
  job_local_read_file(i->job_id,config,job_desc);
  if(!Arc::stringto(job_desc.lifetime,t)) t = keep_finished;
  if(t > keep_finished) t = keep_finished;
  time_t last_changed=job_state_time(i->job_id,config);
  t=last_changed+t; job_desc.cleanuptime=t;
  job_local_write_file(*i,config,job_desc);
  return t;
}

void JobsList::UnlockDelegation(JobsList::iterator &i) {
  ARex::DelegationStores* delegs = config.delegations;
  if(delegs) (*delegs)[config.DelegationDir()].ReleaseCred(i->job_id,true,false);
}

void JobsList::ActJobUndefined(JobsList::iterator &i,
                               bool& once_more,bool& /*delete_job*/,
                               bool& job_error,bool& state_changed) {
        // new job - read its status from status file, but first check if it is
        // under the limit of maximum jobs allowed in the system
        if((AcceptedJobs() < config.max_jobs) || (config.max_jobs == -1)) {
          job_state_t new_state=job_state_read_file(i->job_id,config);
          if(new_state == JOB_STATE_UNDEFINED) { // something failed
            logger.msg(Arc::ERROR,"%s: Reading status of new job failed",i->job_id);
            job_error=true; i->AddFailure("Failed reading status of the job");
            return;
          }
          // By keeping once_more==false job does not cycle here but
          // goes out and registers its state in counters. This allows
          // to maintain limits properly after restart. Except FINISHED
          // jobs because they are not kept in memory and should be 
          // processed immediately.
          i->job_state = new_state; // this can be any state, after A-REX restart
          if(new_state == JOB_STATE_ACCEPTED) {
            state_changed = true; // to trigger email notification, etc.
            // first phase of job - just  accepted - parse request
            logger.msg(Arc::INFO,"%s: State: ACCEPTED: parsing job description",i->job_id);
            if(!job_desc_handler.process_job_req(*i,*i->local)) {
              logger.msg(Arc::ERROR,"%s: Processing job description failed",i->job_id);
              job_error=true;
              i->AddFailure("Could not process job description");
              return; // go to next job
            }
            // set transfer share
            ChooseShare(i);
            job_state_write_file(*i,config,i->job_state);
            // prepare information for logger
            // This call is not needed here because at higher level make_file()
            // is called for every state change
            //config.job_log->make_file(*i,config);
          } else if(new_state == JOB_STATE_FINISHED) {
            once_more=true;
            job_state_write_file(*i,config,i->job_state);
          } else if(new_state == JOB_STATE_DELETED) {
            once_more=true;
            job_state_write_file(*i,config,i->job_state);
          } else {
            logger.msg(Arc::INFO,"%s: %s: New job belongs to %i/%i",i->job_id.c_str(),
                GMJob::get_state_name(new_state),i->get_user().get_uid(),i->get_user().get_gid());
            // Make it clean state after restart
            job_state_write_file(*i,config,i->job_state);
            i->retries = config.max_retries;
            // set transfer share and counters
            ChooseShare(i);
            if (new_state == JOB_STATE_PREPARING) preparing_job_share[i->transfer_share]++;
            if (new_state == JOB_STATE_FINISHING) finishing_job_share[i->transfer_share]++;
            i->Start();

            // add to DN map
            // here we don't enforce the per-DN limit since the jobs are
            // already in the system
            if (i->local->DN.empty()) {
              logger.msg(Arc::WARNING, "Failed to get DN information from .local file for job %s", i->job_id);
            }
            jobs_dn[i->local->DN]++;
          }
        } // Not doing JobPending here because that job kind of does not exist.
        return;
}

void JobsList::ActJobAccepted(JobsList::iterator &i,
                              bool& once_more,bool& /*delete_job*/,
                              bool& job_error,bool& state_changed) {
        // accepted state - job was just accepted by A-REX and we already
        // know that it is accepted - now we are analyzing/parsing request,
        // or it can also happen we are waiting for user specified time
        logger.msg(Arc::VERBOSE,"%s: State: ACCEPTED",i->job_id);
        if(!GetLocalDescription(i)) {
          job_error=true;
          i->AddFailure("Internal error");
          return; // go to next job
        }
        if(i->local->dryrun) {
          logger.msg(Arc::INFO,"%s: State: ACCEPTED: dryrun",i->job_id);
          i->AddFailure("User requested dryrun. Job skipped.");
          job_error=true; 
          return; // go to next job
        }
        // check per-DN limit on processing jobs
        if (config.max_jobs_per_dn > 0 && jobs_dn[i->local->DN] >= config.max_jobs_per_dn) {
          JobPending(i);
          return;
        }
        // check other limits on staging
        if (!CanStage(i, false)) {
          JobPending(i);
          return;
        }
        // check for user specified time
        if(i->retries == 0 && i->local->processtime != -1 && (i->local->processtime) > time(NULL)) {
          logger.msg(Arc::INFO,"%s: State: ACCEPTED: has process time %s",i->job_id.c_str(),
              i->local->processtime.str(Arc::UserTime));
          return;
        }
        // job can progress to PREPARING - add to per-DN job list
        jobs_dn[i->local->DN]++;
        logger.msg(Arc::INFO,"%s: State: ACCEPTED: moving to PREPARING",i->job_id);
        state_changed=true; once_more=true;
        i->job_state = JOB_STATE_PREPARING;
        // if first pass then reset retries
        if (i->retries == 0) i->retries = config.max_retries;
        preparing_job_share[i->transfer_share]++;
        i->Start();

        // gather some frontend specific information for user, do it only once
        // Runs user-supplied executable placed at "frontend-info-collector"
        if(state_changed && i->retries == config.max_retries) {
          std::string cmd = Arc::ArcLocation::GetToolsDir()+"/frontend-info-collector";
          char const * const args[2] = { cmd.c_str(), NULL };
          job_controldiag_mark_put(*i,config,args);
        }
}

void JobsList::ActJobPreparing(JobsList::iterator &i,
                               bool& once_more,bool& /*delete_job*/,
                               bool& job_error,bool& state_changed) {
        // preparing state - job is in data staging system, so check if it has
        // finished and whether all user uploadable files have been uploaded.
        logger.msg(Arc::VERBOSE,"%s: State: PREPARING",i->job_id);
        bool retry = false;
        if(i->job_pending || state_loading(i,state_changed,false,retry)) {
          if(i->job_pending || state_changed) {
            if (state_changed) preparing_job_share[i->transfer_share]--;
            if(!GetLocalDescription(i)) {
              logger.msg(Arc::ERROR,"%s: Failed obtaining local job information.",i->job_id);
              i->AddFailure("Internal error");
              job_error=true;
              return;
            }
            // For jobs with free stage in check if user reported complete stage in.
            bool stagein_complete = true;
            if(i->local->freestagein) {
              stagein_complete = false;
              std::list<std::string> ifiles;
              if(job_input_status_read_file(i->job_id,config,ifiles)) {
                for(std::list<std::string>::iterator ifile = ifiles.begin();
                                   ifile != ifiles.end(); ++ifile) {
                  if(*ifile == "/") {
                    stagein_complete = true;
                    break;
                  }
                }
              }
            }
            // Here we have branch. Either job is ordinary one and goes to SUBMIT
            // or it has no executable and hence goes to FINISHING
            if(!stagein_complete) {
              state_changed=false;
              JobPending(i);
            } else if(i->local->exec.size() > 0) {
              if((config.max_jobs_running==-1) || (RunningJobs()<config.max_jobs_running)) {
                i->job_state = JOB_STATE_SUBMITTING;
                state_changed=true; once_more=true;
                i->retries = config.max_retries;
              } else {
                state_changed=false;
                JobPending(i);
              }
            } else if(CanStage(i, true)) {
              i->job_state = JOB_STATE_FINISHING;
              state_changed=true; once_more=true;
              i->retries = config.max_retries;
              finishing_job_share[i->transfer_share]++;
            } else {
              JobPending(i);
            }
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
            // set next retry time
            //  exponential back-off algorithm - wait 10s, 40s, 90s, 160s,...
            // with a bit of randomness thrown in - vary by up to 50% of wait_time
            int wait_time = 10 * (config.max_retries - i->retries) * (config.max_retries - i->retries);
            int randomness = (rand() % wait_time) - (wait_time/2);
            wait_time += randomness;
            i->next_retry = time(NULL) + wait_time;
            logger.msg(Arc::ERROR,"%s: Download failed. %d retries left. Will wait for %ds before retrying",i->job_id,i->retries,wait_time);
            // set back to ACCEPTED
            i->job_state = JOB_STATE_ACCEPTED;
            if (--(jobs_dn[i->local->DN]) <= 0) jobs_dn.erase(i->local->DN);
            state_changed = true;
          }
        } 
        else {
          if(i->GetFailure(config).empty())
            i->AddFailure("Data staging failed (pre-processing)");
          job_error=true;
          preparing_job_share[i->transfer_share]--;
        }
}

void JobsList::ActJobSubmitting(JobsList::iterator &i,
                                bool& once_more,bool& /*delete_job*/,
                                bool& job_error,bool& state_changed) {
        // everything is ready for submission to batch system or currently submitting
        logger.msg(Arc::VERBOSE,"%s: State: SUBMIT",i->job_id);
        if(state_submitting(i,state_changed)) {
          if(state_changed) {
            i->job_state = JOB_STATE_INLRMS;
            once_more=true;
          }
        } else {
          job_error=true;
        }
}

void JobsList::ActJobCanceling(JobsList::iterator &i,
                               bool& once_more,bool& /*delete_job*/,
                               bool& job_error,bool& state_changed) {
        // This state is like submitting, only -cancel instead of -submit
        logger.msg(Arc::VERBOSE,"%s: State: CANCELING",i->job_id);
        if(state_submitting(i,state_changed,true)) {
          if(state_changed) {
            i->job_state = JOB_STATE_FINISHING;
            finishing_job_share[i->transfer_share]++;
            once_more=true;
          }
        }
        else {
          job_error=true;
        }
}

void JobsList::ActJobInlrms(JobsList::iterator &i,
                            bool& once_more,bool& /*delete_job*/,
                            bool& job_error,bool& state_changed) {
        // Job is currently running in LRMS, check if it has finished
        logger.msg(Arc::VERBOSE,"%s: State: INLRMS",i->job_id);
        if(!GetLocalDescription(i)) {
          i->AddFailure("Failed reading local job information");
          job_error=true;
          return; // go to next job
        }
        // only check lrms job status on first pass
        if(i->retries == 0 || i->retries == config.max_retries) {
          if(i->job_pending || job_lrms_mark_check(i->job_id,config)) {
            if(!i->job_pending) {
              logger.msg(Arc::INFO,"%s: Job finished",i->job_id);
              job_diagnostics_mark_move(*i,config);
              LRMSResult ec = job_lrms_mark_read(i->job_id,config);
              if(ec.code() != i->local->exec.successcode) {
                logger.msg(Arc::INFO,"%s: State: INLRMS: exit message is %i %s",i->job_id,ec.code(),ec.description());
                i->AddFailure("LRMS error: ("+
                      Arc::tostring(ec.code())+") "+ec.description());
                job_error=true;
                JobFailStateRemember(i,JOB_STATE_INLRMS);
                // This does not require any special postprocessing and
                // can go to next state directly
                state_changed=true; once_more=true;
                return;
              }
            }
            if (CanStage(i, true)) {
              state_changed=true; once_more=true;
              i->job_state = JOB_STATE_FINISHING;
              // if first pass then reset retries
              if (i->retries == 0) i->retries = config.max_retries;
              finishing_job_share[i->transfer_share]++;
            } else JobPending(i);
          }
        } else if (CanStage(i, true)) {
          state_changed=true; once_more=true;
          i->job_state = JOB_STATE_FINISHING;
          finishing_job_share[i->transfer_share]++;
        } else {
          JobPending(i);
        }
}

void JobsList::ActJobFinishing(JobsList::iterator &i,
                               bool& once_more,bool& /*delete_job*/,
                               bool& job_error,bool& state_changed) {
        // Batch job has finished and now ready to upload output files, or
        // upload is already on-going
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
            }
            // set next retry time
            // exponential back-off algorithm - wait 10s, 40s, 90s, 160s,...
            // with a bit of randomness thrown in - vary by up to 50% of wait_time
            int wait_time = 10 * (config.max_retries - i->retries) * (config.max_retries - i->retries);
            int randomness = (rand() % wait_time) - (wait_time/2);
            wait_time += randomness;
            i->next_retry = time(NULL) + wait_time;
            logger.msg(Arc::ERROR,"%s: Upload failed. %d retries left. Will wait for %ds before retrying.",i->job_id,i->retries,wait_time);
            // set back to INLRMS
            i->job_state = JOB_STATE_INLRMS;
            state_changed = true;
            return;
          }
          else if(state_changed) {
            finishing_job_share[i->transfer_share]--;
            i->job_state = JOB_STATE_FINISHED;
            if(GetLocalDescription(i)) {
              if (--(jobs_dn[i->local->DN]) <= 0) jobs_dn.erase(i->local->DN);
            }
            once_more=true;
          }
          else {
            return; // still in data staging
          }
        } else {
          state_changed=true; // to send mail
          once_more=true;
          if(i->GetFailure(config).empty())
            i->AddFailure("uploader failed (post-processing)");
          job_error=true;
          finishing_job_share[i->transfer_share]--;
        }
}

void JobsList::ActJobFinished(JobsList::iterator &i,
                              bool& /*once_more*/,bool& /*delete_job*/,
                              bool& /*job_error*/,bool& state_changed) {
        // Job has completely finished, check for user requests to restart or
        // clean up job, and if it is time to move to DELETED
        if(job_clean_mark_check(i->job_id,config)) {
          // request to clean job
          logger.msg(Arc::INFO,"%s: Job is requested to clean - deleting",i->job_id);
          UnlockDelegation(i);
          // delete everything
          job_clean_final(*i,config);
          return;
        }
        if(job_restart_mark_check(i->job_id,config)) {
           job_restart_mark_remove(i->job_id,config);
          // request to rerun job - check if we can
          // Get information about failed state and forget it
          job_state_t state_ = JobFailStateGet(i);
          if(state_ == JOB_STATE_PREPARING) {
            if(RecreateTransferLists(i)) {
              job_failed_mark_remove(i->job_id,config);
              i->job_state = JOB_STATE_ACCEPTED;
              JobPending(i); // make it go to end of state immediately
              return;
            }
          } else if((state_ == JOB_STATE_SUBMITTING) ||
                    (state_ == JOB_STATE_INLRMS)) {
            if(RecreateTransferLists(i)) {
              job_failed_mark_remove(i->job_id,config);
              if(i->local->downloads > 0) {
                // missing input files has to be re-downloaded
                i->job_state = JOB_STATE_ACCEPTED;
              } else {
                i->job_state = JOB_STATE_PREPARING;
              }
              JobPending(i); // make it go to end of state immediately
              return;
            }
          } else if(state_ == JOB_STATE_FINISHING) {
            if(RecreateTransferLists(i)) {
              job_failed_mark_remove(i->job_id,config);
              i->job_state = JOB_STATE_INLRMS;
              JobPending(i); // make it go to end of state immediately
              return;
            }
          } else if(state_ == JOB_STATE_UNDEFINED) {
            logger.msg(Arc::ERROR,"%s: Can't rerun on request",i->job_id);
          } else {
            logger.msg(Arc::ERROR,"%s: Can't rerun on request - not a suitable state",i->job_id);
          }
        }
        time_t t = -1;
        if(!job_local_read_cleanuptime(i->job_id,config,t)) {
          // must be first time - create cleanuptime
          t=PrepareCleanupTime(i,i->keep_finished);
        }
        // check if it is time to move job to DELETED
        if(((int)(time(NULL)-t)) >= 0) {
          logger.msg(Arc::INFO,"%s: Job is too old - deleting",i->job_id);
          UnlockDelegation(i);
          if(i->keep_deleted) {
            // here we have to get the cache per-job dirs to be deleted
            std::list<std::string> cache_per_job_dirs;
            CacheConfig cache_config(config.cache_params);
            cache_config.substitute(config, i->user);
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
            job_clean_deleted(*i,config,cache_per_job_dirs);
            i->job_state = JOB_STATE_DELETED;
            state_changed=true;
          } else {
            // delete everything
            job_clean_final(*i,config);
          }
        }
}

void JobsList::ActJobDeleted(JobsList::iterator &i,
                             bool& /*once_more*/,bool& /*delete_job*/,
                             bool& /*job_error*/,bool& /*state_changed*/) {
        // Job only has a few control files left, check if is it time to
        // remove all traces
        time_t t = -1;
        if(!job_local_read_cleanuptime(i->job_id,config,t) || ((time(NULL)-(t+i->keep_deleted)) >= 0)) {
          logger.msg(Arc::INFO,"%s: Job is ancient - delete rest of information",i->job_id);
          // delete everything
          job_clean_final(*i,config);
        }
}

bool JobsList::ActJob(JobsList::iterator &i) {
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
    // some states can not be canceled (or there is no sense to do that)
    if((i->job_state != JOB_STATE_CANCELING) &&
       (i->job_state != JOB_STATE_FINISHED) &&
       (i->job_state != JOB_STATE_DELETED) &&
       (i->job_state != JOB_STATE_SUBMITTING)) {
      if(job_cancel_mark_check(i->job_id,config)) {
        logger.msg(Arc::INFO,"%s: Canceling job because of user request",i->job_id);
        if (config.use_dtr && dtr_generator &&
            (i->job_state == JOB_STATE_PREPARING || i->job_state == JOB_STATE_FINISHING)) {
          dtr_generator->cancelJob(*i);
        }
        // kill running child
        if(i->child) { 
          i->child->Kill(0);
          delete i->child; i->child=NULL;
        }
        // update transfer share counters
        if (i->job_state == JOB_STATE_PREPARING && !i->job_pending) preparing_job_share[i->transfer_share]--;
        else if (i->job_state == JOB_STATE_FINISHING) finishing_job_share[i->transfer_share]--;
        // put some explanation
        i->AddFailure("User requested to cancel the job");
        JobFailStateRemember(i,i->job_state,false);
        // behave like if job failed
        if(!FailedJob(i,true)) {
          // DO NOT KNOW WHAT TO DO HERE !!!!!!!!!!
        }
        // special processing for INLRMS case
        if(i->job_state == JOB_STATE_INLRMS) {
          i->job_state = JOB_STATE_CANCELING;
        }
        // if new data staging we wait to get back all DTRs
        else if(i->job_state == JOB_STATE_FINISHING) {
          if (!config.use_dtr) {
            i->job_state = JOB_STATE_FINISHED;
            if(GetLocalDescription(i)) {
              if (--(jobs_dn[i->local->DN]) <= 0) jobs_dn.erase(i->local->DN);
            }
          }
        }
        else if (!config.use_dtr || i->job_state != JOB_STATE_PREPARING) {
          i->job_state = JOB_STATE_FINISHING;
          finishing_job_share[i->transfer_share]++;
        }
        job_cancel_mark_remove(i->job_id,config);
        state_changed=true;
        once_more=true;
      }
    }
    if(!state_changed) switch(i->job_state) {
      case JOB_STATE_UNDEFINED: {
       ActJobUndefined(i,once_more,delete_job,job_error,state_changed);
      } break;
      case JOB_STATE_ACCEPTED: {
       ActJobAccepted(i,once_more,delete_job,job_error,state_changed);
      } break;
      case JOB_STATE_PREPARING: {
       ActJobPreparing(i,once_more,delete_job,job_error,state_changed);
      } break;
      case JOB_STATE_SUBMITTING: {
       ActJobSubmitting(i,once_more,delete_job,job_error,state_changed);
      } break;
      case JOB_STATE_CANCELING: {
       ActJobCanceling(i,once_more,delete_job,job_error,state_changed);
      } break;
      case JOB_STATE_INLRMS: {
       ActJobInlrms(i,once_more,delete_job,job_error,state_changed);
      } break;
      case JOB_STATE_FINISHING: {
       ActJobFinishing(i,once_more,delete_job,job_error,state_changed);
      } break;
      case JOB_STATE_FINISHED: {
       ActJobFinished(i,once_more,delete_job,job_error,state_changed);
      } break;
      case JOB_STATE_DELETED: {
       ActJobDeleted(i,once_more,delete_job,job_error,state_changed);
      } break;
      default: { // should destroy job with unknown state ?!
      } break;
    }
    do {
      // Process errors which happened during processing this job
      if(job_error) {
        job_error=false;
        // always cause rerun - in order not to lose state change
        // Failed job - move it to proper state
        logger.msg(Arc::ERROR,"%s: Job failure detected",i->job_id);
        if(!FailedJob(i,false)) { // something is really wrong
          i->AddFailure("Failed during processing failure");
          delete_job=true;
        } else { // just move job to proper state
          if((i->job_state == JOB_STATE_FINISHED) ||
             (i->job_state == JOB_STATE_DELETED)) {
            // Normally these stages should not generate errors
            // so ignore them
          } else if(i->job_state == JOB_STATE_FINISHING) {
            // No matter if FINISHING fails - it still goes to FINISHED
            i->job_state = JOB_STATE_FINISHED;
            if(GetLocalDescription(i)) {
              if (--(jobs_dn[i->local->DN]) <= 0) jobs_dn.erase(i->local->DN);
            }
            state_changed=true;
            once_more=true;
          } else {
            i->job_state = JOB_STATE_FINISHING;
            finishing_job_share[i->transfer_share]++;
            state_changed=true;
            once_more=true;
          }
          i->job_pending=false;
        }
      }
      // Process state changes, also those generated by error processing
      if(old_reported_state != i->job_state) {
        if(old_reported_state != JOB_STATE_UNDEFINED) {
          // Report state change into log
          logger.msg(Arc::INFO,"%s: State: %s from %s",
                i->job_id.c_str(),GMJob::get_state_name(i->job_state),
                GMJob::get_state_name(old_reported_state));
        }
        old_reported_state=i->job_state;
      }
      if(state_changed) {
        state_changed=false;
        i->job_pending=false;
        if(!job_state_write_file(*i,config,i->job_state)) {
          i->AddFailure("Failed writing job status: "+Arc::StrError(errno));
          job_error=true;
        } else {
          // Talk to external plugin to ask if we can proceed
          // Jobs with ACCEPTED state or UNDEFINED previous state
          // could be ignored here. But there is tiny possibility
          // that service failed while processing ContinuationPlugins.
          // Hence here we have duplicate call for ACCEPTED state.
          // TODO: maybe introducing job state prefix VALIDATING:
          // could be used to resolve this situation.
          if(config.cont_plugins) {
            std::list<ContinuationPlugins::result_t> results;
            config.cont_plugins->run(*i,config,results);
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
              }
              ++result;
            }
          }
          // Processing to be done on state changes 
          config.job_log->make_file(*i,config);
          if(i->job_state == JOB_STATE_FINISHED) {
            job_clean_finished(i->job_id,config);
            config.job_log->finish_info(*i,config);
            PrepareCleanupTime(i,i->keep_finished);
          } else if(i->job_state == JOB_STATE_PREPARING) {
            config.job_log->start_info(*i,config);
          }
        }
        // send mail after error and change are processed
        // do not send if something really wrong happened to avoid email DoS
        if(!delete_job) send_mail(*i,config);
      }
      // Keep repeating till error goes out
    } while(job_error);
    if(delete_job) { 
      logger.msg(Arc::ERROR,"%s: Delete request due to internal problems",i->job_id);
      i->job_state = JOB_STATE_FINISHED; // move to finished in order to remove from list
      if(i->GetLocalDescription(config)) {
        if (--(jobs_dn[i->local->DN]) == 0) jobs_dn.erase(i->local->DN);
      }
      i->job_pending=false;
      job_state_write_file(*i,config,i->job_state);
      i->AddFailure("Serious troubles (problems during processing problems)");
      FailedJob(i,false);  // put some marks
      job_clean_finished(i->job_id,config);  // clean status files
      once_more=true; // to process some things in local
    }
  }
  // FINISHED+DELETED jobs are not kept in list - only in files
  // if job managed to get here with state UNDEFINED -
  // means we are overloaded with jobs - do not keep them in list
  if((i->job_state == JOB_STATE_FINISHED) ||
     (i->job_state == JOB_STATE_DELETED) ||
     (i->job_state == JOB_STATE_UNDEFINED)) {
    // this is the ONLY place where jobs are removed from memory
    // update counters
    if(!old_pending) {
      jobs_num[old_state]--;
    } else {
      jobs_pending--;
    }
    if(i->local) { delete i->local; }
    i=jobs.erase(i);
  }
  else {
    // update counters
    if(!old_pending) {
      jobs_num[old_state]--;
    } else {
      jobs_pending--;
    }
    if(!i->job_pending) {
      jobs_num[i->job_state]++;
    } else {
      jobs_pending++;
    }
    ++i;
  }
  return true;
}

class JobFDesc {
 public:
  JobId id;
  uid_t uid;
  gid_t gid;
  time_t t;
  JobFDesc(const std::string& s):id(s),uid(0),gid(0),t(-1) { }
  bool operator<(const JobFDesc &right) const { return (t < right.t); }
};

bool JobsList::RestartJobs(const std::string& cdir,const std::string& odir) {
  bool res = true;
  try {
    Glib::Dir dir(cdir);
    for(;;) {
      std::string file=dir.read_name();
      if(file.empty()) break;
      int l=file.length();
      // job id contains at least 1 character
      if(l>(4+7) && file.substr(0,4) == "job." && file.substr(l-7) == ".status") {
        uid_t uid;
        gid_t gid;
        time_t t;
        std::string fname=cdir+'/'+file.c_str();
        std::string oname=odir+'/'+file.c_str();
        if(check_file_owner(fname,uid,gid,t)) {
          if(::rename(fname.c_str(),oname.c_str()) != 0) {
            logger.msg(Arc::ERROR,"Failed to move file %s to %s",fname,oname);
            res=false;
          }
        }
      }
    }
    dir.close();
  } catch(Glib::FileError& e) {
    logger.msg(Arc::ERROR,"Failed reading control directory: %s",cdir);
    return false;
  }
  return res;
}

// This code is run at service restart
bool JobsList::RestartJobs(void) {
  std::string cdir=config.control_dir;
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
      // job id contains at least 1 character
      if(l>(4+7) && file.substr(0,4) == "job." && file.substr(l-7) == ".status") {
        JobFDesc id(file.substr(4,l-7-4));
        if(FindJob(id.id) == jobs.end()) {
          std::string fname=cdir+'/'+file.c_str();
          uid_t uid;
          gid_t gid;
          time_t t;
          if(check_file_owner(fname,uid,gid,t)) {
            // add it to the list
            id.uid=uid; id.gid=gid; id.t=t;
            ids.push_back(id);
          }
        }
      }
    }
  } catch(Glib::FileError& e) {
    logger.msg(Arc::ERROR,"Failed reading control directory: %s: %s",config.control_dir, e.what());
    return false;
  }
  return true;
}

bool JobsList::ScanMarks(const std::string& cdir,const std::list<std::string>& suffices,std::list<JobFDesc>& ids) {
  try {
    Glib::Dir dir(cdir);
    for(;;) {
      std::string file=dir.read_name();
      if(file.empty()) break;
      int l=file.length();
      // job id contains at least 1 character
      if(l>(4+7) && file.substr(0,4) == "job.") {
        for(std::list<std::string>::const_iterator sfx = suffices.begin();
            sfx != suffices.end();++sfx) {
          int ll = sfx->length();
          if(l > (ll+4) && file.substr(l-ll) == *sfx) {
            JobFDesc id(file.substr(4,l-ll-4));
            if(FindJob(id.id) == jobs.end()) {
              std::string fname=cdir+'/'+file.c_str();
              uid_t uid;
              gid_t gid;
              time_t t;
              if(check_file_owner(fname,uid,gid,t)) {
                // add it to the list
                id.uid=uid; id.gid=gid; id.t=t;
                ids.push_back(id);
              }
            }
            break;
          }
        }
      }
    }
  } catch(Glib::FileError& e) {
    logger.msg(Arc::ERROR,"Failed reading control directory: %s",config.control_dir);
    return false;
  }
  return true;
}

// find new jobs - sort by date to implement FIFO
bool JobsList::ScanNewJobs(void) {
  std::string cdir=config.control_dir;
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
    // adding job with file's uid/gid
    AddJobNoCheck(id->id,i,id->uid,id->gid);
  }
  return true;
}

bool JobsList::ScanOldJobs(int max_scan_time,int max_scan_jobs) {
  // We are going to scan a dir with a lot of files here. So we scan it in
  // parts and limit scanning time. A finished job is added to the job list
  // and acted on straight away. If it remains finished or is deleted then it
  // will be removed again from the list. If it is restarted it will be kept in
  // the list and processed as normal in the next processing loop. Restarts
  // are normally processed in ScanNewMarks but can also happen here.
  time_t start = time(NULL);
  if(max_scan_time < 10) max_scan_time=10; // some sane number - 10s
  std::string cdir=config.control_dir+"/finished";
  try {
    if(!old_dir) {
      old_dir = new Glib::Dir(cdir);
    }
    for(;;) {
      std::string file=old_dir->read_name();
      if(file.empty()) {
        old_dir->close();
        delete old_dir;
        old_dir=NULL;
        return false;
      }
      int l=file.length();
      // job id must contain at least one character
      if(l>(4+7) && file.substr(0,4) == "job." && file.substr(l-7) == ".status") {
        JobFDesc id(file.substr(4, l-7-4));
        if(FindJob(id.id) == jobs.end()) {
          std::string fname=cdir+'/'+file;
          uid_t uid;
          gid_t gid;
          time_t t;
          if(check_file_owner(fname,uid,gid,t)) {
            job_state_t st = job_state_read_file(id.id,config);
            if(st == JOB_STATE_FINISHED || st == JOB_STATE_DELETED) {
              JobsList::iterator i;
              AddJobNoCheck(id.id, i, uid, gid);
              ActJob(i);
              --max_scan_jobs;
            }
          }
        }
      }
      if(((int)(time(NULL)-start)) >= max_scan_time) break;
      if(max_scan_jobs <= 0) break;
    }
  } catch(Glib::FileError& e) {
    logger.msg(Arc::ERROR,"Failed reading control directory: %s",cdir);
    if(old_dir) {
      old_dir->close();
      delete old_dir;
      old_dir=NULL;
    }
    return false;
  }
  return true;
}

bool JobsList::ScanNewMarks(void) {
  std::string cdir=config.control_dir;
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
    job_state_t st = job_state_read_file(id->id,config);
    if((st == JOB_STATE_UNDEFINED) || (st == JOB_STATE_DELETED)) {
      // Job probably does not exist anymore
      job_clean_mark_remove(id->id,config);
      job_restart_mark_remove(id->id,config);
      job_cancel_mark_remove(id->id,config);
    }
    // Check if such job finished and add it to list.
    if(st == JOB_STATE_FINISHED) {
      iterator i;
      AddJobNoCheck(id->id,i,id->uid,id->gid);
      // That will activate its processing at least for one step.
      i->job_state = st;
    }
  }
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
    std::string cdir=config.control_dir;
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

bool JobsList::AddJob(const JobId& id) {
  if(FindJob(id) != jobs.end()) return true;
  std::list<std::string> subdirs;
  subdirs.push_back("/restarting"); // For picking up jobs after service restart
  subdirs.push_back("/accepting");  // For new jobs
  subdirs.push_back("/processing"); // For active jobs
  subdirs.push_back("/finished");   // For done jobs
  for(std::list<std::string>::iterator subdir = subdirs.begin();
                               subdir != subdirs.end();++subdir) {
    std::string cdir=config.control_dir;
    std::string odir=cdir+(*subdir);
    std::string fname=odir+'/'+"job."+id+".status";
    uid_t uid;
    gid_t gid;
    time_t t;
    if(check_file_owner(fname,uid,gid,t)) {
      // add it to the list
      AddJobNoCheck(id,uid,gid);
      return true;
    }
  }
  return false;
}

} // namespace ARex
