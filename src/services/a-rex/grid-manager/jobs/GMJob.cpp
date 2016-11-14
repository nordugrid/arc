#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <cstring>

#include "../files/ControlFileContent.h"
#include "../files/ControlFileHandling.h"
#include "GMJob.h"

namespace ARex {

static Arc::Logger& logger = Arc::Logger::getRootLogger();

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
  ref_count = 0;
}

// Deprecated
GMJob::GMJob(const GMJob &job) {
  operator=(job);
}

// Deprecated
GMJob& GMJob::operator=(const GMJob &job) {
  job_state=job.job_state;
  job_pending=job.job_pending;
  job_id=job.job_id;
  session_dir=job.session_dir;
  failure_reason=job.failure_reason;
  keep_finished=job.keep_finished;
  keep_deleted=job.keep_deleted;
  child=NULL;
  local=NULL;
  if(job.local) local=new JobLocalDescription(*job.local);
  user=job.user;
  transfer_share=job.transfer_share;
  start_time=job.start_time;
  ref_count = 0;
  return *this;
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
  ref_count = 0;
}

GMJob::~GMJob(void){
  if(child) {
    // Wait for downloader/uploader/script to finish
    child->Wait();
    delete child;
    child=NULL;
  }
  delete local;
}


void GMJob::AddReference(void) {
  Glib::Mutex::Lock lock(ref_lock);
  ++ref_count;
}

void GMJob::RemoveReference(void) {
  Glib::Mutex::Lock lock(ref_lock);
  if(--ref_count == 0) {
    logger.msg(Arc::ERROR,"%s: Job monitoring is unintentionally lost",job_id);
    lock.release();
    delete this;
  };
}

void GMJob::DestroyReference(void) {
  Glib::Mutex::Lock lock(ref_lock);
  if(--ref_count == 0) {
    logger.msg(Arc::ERROR,"%s: Job monitoring stop success",job_id);
    lock.release();
    delete this;
  } else {
    logger.msg(Arc::ERROR,"%s: Job monitoring stop requested with %u active references",job_id,ref_count);
  };
}

bool GMJob::SwitchQueue(GMJobQueue* new_queue, bool no_lock) {
  // Lock job instance
  Glib::Mutex::Lock lock(ref_lock, Glib::NOT_LOCK);
  if (!no_lock) lock.acquire();
  GMJobQueue* old_queue = queue;
  if (old_queue == new_queue) return true; // shortcut
  // Check priority
  if (old_queue && new_queue && (new_queue->priority_ > old_queue->priority_)) return false;
  if (old_queue) {
    // Lock current queue
    Glib::Mutex::Lock qlock(queue->lock_);
    // Remove from current queue
    old_queue->queue_.remove(this); // ineffective operation!
    queue = NULL;
    // Unlock current queue
  };
  if (new_queue) {
    // Lock new queue
    Glib::Mutex::Lock qlock(new_queue->lock_);
    // Add to new queue
    new_queue->queue_.push_back(this);
    queue = new_queue;
    // Unlock new queue
  };
  // Handle reference counter
  if(new_queue && !old_queue) {
    ++ref_count;
  } else if(!new_queue && old_queue) {
    if(--ref_count == 0) {
      if(!no_lock) {
        logger.msg(Arc::ERROR,"%s: Job monitoring is lost due to removal from queue",job_id);
        lock.release();
        delete this;
      } else {
        logger.msg(Arc::ERROR,"%s: Job has no reference due to removal from queue",job_id);
      };
    };
  };
  // Unlock job instance
  return true;
}


JobLocalDescription* GMJob::GetLocalDescription(const GMConfig& config) {
  if(local) return local;
  JobLocalDescription* job_desc;
  job_desc=new JobLocalDescription;
  if(!job_local_read_file(job_id,config,*job_desc)) {
    delete job_desc;
    return NULL;
  };
  local=job_desc;
  return local;
}

JobLocalDescription* GMJob::GetLocalDescription(void) const {
  return local;
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

// ----------------------------------------------------------

GMJobQueue::GMJobQueue(int priority):priority_(priority) {
}

bool GMJobQueue::Push(GMJobRef& ref) {
  if(ref) {
    return ref->SwitchQueue(this, false);
  };
  return false;
}

GMJobRef GMJobQueue::Pop() {
  Glib::Mutex::Lock qlock(lock_);
  if(queue_.empty()) return GMJobRef();
  GMJobRef ref(queue_.front());
  ref->SwitchQueue(NULL, true); 
  return ref;
}



} // namespace ARex
