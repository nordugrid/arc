#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <cstring>

#include "../files/ControlFileContent.h"
#include "../files/ControlFileHandling.h"
#include "GMJob.h"

namespace ARex {

GMJob::job_state_rec_t const GMJob::states_all[JOB_STATE_NUM] = {
  { "ACCEPTED",   ' ' }, // JOB_STATE_ACCEPTED
  { "PREPARING",  'b' }, // JOB_STATE_PREPARING
  { "SUBMIT",     ' ' }, // JOB_STATE_SUBMITING
  { "INLRMS",     'q' }, // JOB_STATE_INLRMS,
  { "FINISHING",  'f' }, // JOB_STATE_FINISHING
  { "FINISHED",   'e' }, // JOB_STATE_FINISHED
  { "DELETED",    'd' }, // JOB_STATE_DELETED
  { "CANCELING",  'c' }, // JOB_STATE_CANCELING
  { "UNDEFINED",  ' ' }  // JOB_STATE_UNDEFINED
};


const char* GMJob::get_state_name() const {
  if((job_state<0) || (job_state>=JOB_STATE_NUM))
                       return states_all[JOB_STATE_UNDEFINED].name;
  return states_all[job_state].name;
}

char GMJob::get_state_mail_flag() const {
  if((job_state<0) || (job_state>=JOB_STATE_NUM))
                       return states_all[JOB_STATE_UNDEFINED].mail_flag;
  return states_all[job_state].mail_flag;
}

const char* GMJob::get_state_name(job_state_t st) {
  if((st<0) || (st>=JOB_STATE_NUM))
                       return states_all[JOB_STATE_UNDEFINED].name;
  return states_all[st].name;
}

char GMJob::get_state_mail_flag(job_state_t st) {
  if((st<0) || (st>=JOB_STATE_NUM))
                       return states_all[JOB_STATE_UNDEFINED].mail_flag;
  return states_all[st].mail_flag;
}

job_state_t GMJob::get_state(const char* state) {
  for(int i = 0;i<JOB_STATE_NUM;i++) {
    if(!strcmp(states_all[i].name,state)) return (job_state_t)i;
  };
  return JOB_STATE_UNDEFINED;
}

void GMJob::set_share(std::string share) {
  transfer_share = share.empty() ? JobLocalDescription::transfersharedefault : share;
}

GMJob::GMJob(void) {
  job_state=JOB_STATE_UNDEFINED;
  job_pending=false;
  keep_finished=-1;
  keep_deleted=-1;
  child=NULL;
  local=NULL;
  start_time=time(NULL);
}

GMJob::GMJob(const GMJob &job) {
  job_state=job.job_state;
  job_pending=job.job_pending;
  job_id=job.job_id;
  session_dir=job.session_dir;
  failure_reason=job.failure_reason;
  keep_finished=job.keep_finished;
  keep_deleted=job.keep_deleted;
  child=NULL;
  local=job.local;
  user=job.user;
  transfer_share=job.transfer_share;
  start_time=job.start_time;
}

GMJob::GMJob(const JobId &id,const Arc::User& u,const std::string &dir,job_state_t state) {
  job_state=state;
  job_pending=false;
  job_id=id;
  session_dir=dir;
  keep_finished=-1;
  keep_deleted=-1;
  child=NULL;
  local=NULL;
  user=u;
  transfer_share=JobLocalDescription::transfersharedefault;
  start_time=time(NULL);
}

GMJob::~GMJob(void){
  if(child) {
    // Wait for downloader/uploader/script to finish
    child->Wait();
    delete child;
    child=NULL;
  }
}

bool GMJob::GetLocalDescription(const GMConfig& config) {
  if(local) return true;
  JobLocalDescription* job_desc;
  job_desc=new JobLocalDescription;
  if(!job_local_read_file(job_id,config,*job_desc)) {
    delete job_desc;
    return false;
  };
  local=job_desc;
  return true;
}

std::string GMJob::GetFailure(const GMConfig& config) const {
  std::string reason = job_failed_mark_read(job_id,config);
  if(!failure_reason.empty()) {
    reason+=failure_reason; reason+="\n";
  };
  return reason;
}

bool GMJob::CheckFailure(const GMConfig& config) const {
  if(!failure_reason.empty()) return true;
  return job_failed_mark_check(job_id,config);
}

void GMJob::PrepareToDestroy(void) {
  // We could send signals to downloaders and uploaders.
  // But currently those do not implement safe shutdown.
  // So we will simply wait for them to finish in destructor.
}

} // namespace ARex
