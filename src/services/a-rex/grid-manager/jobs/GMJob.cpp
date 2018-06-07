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
  queue = NULL;
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
  queue = NULL;
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
  Glib::RecMutex::Lock lock(ref_lock);
  ++ref_count;
}

void GMJob::RemoveReference(void) {
  Glib::RecMutex::Lock lock(ref_lock);
  if(--ref_count == 0) {
    logger.msg(Arc::ERROR,"%s: Job monitoring is unintentionally lost",job_id);
    lock.release();
    delete this;
  };
}

void GMJob::DestroyReference(void) {
  Glib::RecMutex::Lock lock(ref_lock);
  if(--ref_count == 0) {
    logger.msg(Arc::VERBOSE,"%s: Job monitoring stop success",job_id);
    lock.release();
    delete this;
  } else {
    logger.msg(Arc::ERROR,"%s: Job monitoring stop requested with %u active references",job_id,ref_count);
  };
}

bool GMJob::SwitchQueue(GMJobQueue* new_queue, bool to_front) {
  // Lock job instance
  Glib::RecMutex::Lock lock(ref_lock);
  GMJobQueue* old_queue = queue;
  if (old_queue == new_queue) {
    // shortcut
    if(!to_front) return true;
    if(!old_queue) return true;
    // move to front
    Glib::RecMutex::Lock qlock(old_queue->lock_);
    old_queue->queue_.remove(this); // ineffective operation!
    old_queue->queue_.push_front(this);
    return true;
  };
  // Check priority
  if (old_queue && new_queue) {
    if(!to_front) {
      if(!(new_queue->priority_ > old_queue->priority_)) return false;
    } else {
      // If moving to first place in queue accept same priority 
      if(!(new_queue->priority_ >= old_queue->priority_)) return false;
    };
  };
  if (old_queue) {
    // Lock current queue
    Glib::RecMutex::Lock qlock(old_queue->lock_);
    // Remove from current queue
    old_queue->queue_.remove(this); // ineffective operation!
    queue = NULL;
    // Unlock current queue
  };
  if (new_queue) {
    // Lock new queue
    Glib::RecMutex::Lock qlock(new_queue->lock_);
    // Add to new queue
    if(!to_front) {
      new_queue->queue_.push_back(this);
    } else {
      new_queue->queue_.push_front(this);
    };
    queue = new_queue;
    // Unlock new queue
  };
  // Handle reference counter
  if(new_queue && !old_queue) {
    ++ref_count;
  } else if(!new_queue && old_queue) {
    if(--ref_count == 0) {
      logger.msg(Arc::ERROR,"%s: Job monitoring is lost due to removal from queue",job_id);
      lock.release(); // release before deleting referenced object
      delete this;
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
    return ref->SwitchQueue(this);
  };
  return false;
}

bool GMJobQueue::PushSorted(GMJobRef& ref, comparator_t compare) {
  if(ref) {
    if(ref->SwitchQueue(this)) {
      // Most of the cases job lands last in list
      std::list<GMJob*>::reverse_iterator opos = queue_.rbegin();
      while(opos != queue_.rend()) {
        if(ref == GMJobRef(*opos)) {
          // Can it be moved ?
          std::list<GMJob*>::reverse_iterator npos = opos;
          std::list<GMJob*>::reverse_iterator rpos = npos;
          ++npos;
          while(npos != queue_.rend()) {
            if(!compare(ref, *npos)) break;
            rpos = npos;
            ++npos;
          };
          if(rpos != opos) {  // no reason to move to itself
            queue_.insert(--(rpos.base()),*opos);
            queue_.erase(--(opos.base()));
          };
          break;
        };
        ++opos;
      };
      return true;
    };
  };
  return false;
}

GMJobRef GMJobQueue::Front() {
  Glib::RecMutex::Lock qlock(lock_);
  if(queue_.empty()) return GMJobRef();
  GMJobRef ref(queue_.front());
  return ref;
}

GMJobRef GMJobQueue::Pop() {
  Glib::RecMutex::Lock qlock(lock_);
  if(queue_.empty()) return GMJobRef();
  GMJobRef ref(queue_.front());
  ref->SwitchQueue(NULL); 
  return ref;
}

bool GMJobQueue::Unpop(GMJobRef& ref) {
  if(ref) {
    return ref->SwitchQueue(this, true);
  };
  return false;
}

bool GMJobQueue::Erase(GMJobRef& ref) {
  if(!ref) return false;
  Glib::RecMutex::Lock lock(lock_);
  if(ref->queue == this) {
    ref->SwitchQueue(NULL);
    return true;
  };
  return false;
}

bool GMJobQueue::Exists(const GMJobRef& ref) const {
  if(!ref) return false;
  Glib::RecMutex::Lock lock(const_cast<Glib::RecMutex&>(lock_));
  return (ref->queue == this);
}

void GMJobQueue::Sort(comparator_t compare) {
  queue_.sort(compare);
}

} // namespace ARex
