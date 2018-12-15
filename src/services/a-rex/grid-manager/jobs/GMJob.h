#ifndef GRID_MANAGER_JOB_H
#define GRID_MANAGER_JOB_H

#include <sys/types.h>
#include <string>

#include <arc/Run.h>
#include <arc/User.h>

namespace ARex {

class JobsList;
class JobLocalDescription;
class GMConfig;
class JobsMetrics;

/// Possible job states
enum job_state_t {
  JOB_STATE_ACCEPTED   = 0,
  JOB_STATE_PREPARING  = 1,
  JOB_STATE_SUBMITTING = 2,
  JOB_STATE_INLRMS     = 3,
  JOB_STATE_FINISHING  = 4,
  JOB_STATE_FINISHED   = 5,
  JOB_STATE_DELETED    = 6,
  JOB_STATE_CANCELING  = 7,
  JOB_STATE_UNDEFINED  = 8
};

/// Number of job states
#define JOB_STATE_NUM (JOB_STATE_UNDEFINED+1)

/// Jobs identifier. Stored as string. Normally is a random string of
/// numbers and letters.
typedef std::string JobId;

class GMJobRef;
class GMJobQueue;

/// Represents a job in memory as it passes through the JobsList state machine.
class GMJob {
 friend class JobsList;
 friend class GMJobRef;
 friend class GMJobQueue;
 friend class GMJobMock;
 friend class JobsMetrics;

 private:
  // State of the job (state machine)
  job_state_t job_state;
  // Flag to indicate job stays at this stage due to limits imposed.
  // Such jobs are not counted in counters
  bool job_pending; 
  // Job identifier
  JobId job_id;
  // Directory to run job in
  std::string session_dir;
  // Explanation of job's failure
  std::string failure_reason;
  // How long job is kept on cluster after it finished
  time_t keep_finished;
  time_t keep_deleted;
  // Pointer to object containing most important parameters of job,
  // loaded when needed.
  JobLocalDescription* local;
  // Job's owner
  Arc::User user;
  // Used to determine data transfer share (eg DN, VOMS VO)
  std::string transfer_share;
  // Start time of job i.e. when it first moves to PREPARING
  time_t start_time;

  struct job_state_rec_t {
    const char* name;
    char mail_flag;
  };
  /// Maps job state to state name and flag for email at that state
  static job_state_rec_t const states_all[JOB_STATE_NUM];


  // Job references handler

  Glib::RecMutex ref_lock;
  int ref_count;

  /// Inform job it has new GMJobRef associated
  void AddReference(void);

  /// Inform job that GMJobRef was destroyed
  void RemoveReference(void);

  /// Inform job that GMJobRef intends to destroy job
  void DestroyReference(void);

  /// Change queue to which job belongs. Queue switch is subject to queue's priority and
  /// happens atomically. Similar to GMJobQueue::Push(GMJobRef(this)).
  /// Returns true if queue was changed.
  bool SwitchQueue(GMJobQueue* new_queue, bool to_front = false);

  /// Queue to which job is currently associated
  GMJobQueue* queue;


 public:
  // external utility being run to perform tasks like stage-in/out,
  //   submit/cancel. (todo - move to private)
  Arc::Run* child;
  // Constructors and destructor.
  //  Accepts:
  //    job_id - identifier
  //    user - owner of job
  //    dir - session_dir of job
  //    state - initial state of job
  GMJob(const JobId &job_id,const Arc::User& user,const std::string &dir = "",job_state_t state = JOB_STATE_UNDEFINED);
  GMJob(void);
  GMJob(const GMJob &job);
  ~GMJob(void);
  job_state_t get_state() const { return job_state; };
  const char* get_state_name() const;
  char get_state_mail_flag() const;
  static const char* get_state_name(job_state_t st);
  static char get_state_mail_flag(job_state_t st);
  static job_state_t get_state(const char* state);
  const JobId& get_id() const { return job_id; };
  std::string SessionDir(void) const { return session_dir; };
  void AddFailure(const std::string &reason) { failure_reason+=reason; failure_reason+="\n"; };
  /// Retrieve current failure reason (both in memory and stored in control dir).
  /// For non-failed jobs returned string is empty.
  std::string GetFailure(const GMConfig& config) const;
  /// Check if job is marked as failed (slightly faster than GetFailure).
  /// For failed job returns true, non-failed - false.
  bool CheckFailure(const GMConfig& config) const;
  bool operator==(const GMJob& job) const { return (job_id == job.job_id); };
  bool operator==(const JobId &id) const { return (job_id == id); };
  bool operator!=(const JobId &id) const { return (job_id != id); };
  void set_user(const Arc::User& u) { user = u; }
  const Arc::User& get_user() const { return user;}
  void set_share(std::string share);
  // Force 'local' to be created and read from file if not already available
  JobLocalDescription* GetLocalDescription(const GMConfig& config);
  // Use only preloaded local
  JobLocalDescription* GetLocalDescription() const;
  void Start() { start_time = time(NULL); };
  time_t GetStartTime() const { return start_time; };
  void PrepareToDestroy(void);
};


class GMJobRef {
private:
  GMJob* job_;

public:
  GMJobRef() {
    job_ = NULL;
  }

  GMJobRef(GMJob* job) {
    job_ = job;
    if(job_) job_->AddReference();
  }

  GMJobRef(GMJobRef const& other) {
    job_ = other.job_;
    if(job_) job_->AddReference();
  }

  ~GMJobRef() {
    if (job_) job_->RemoveReference();
  }

  GMJobRef& operator=(GMJobRef const& other) {
    if (job_) job_->RemoveReference();
    job_ = other.job_;
    if(job_) job_->AddReference();
    return *this;
  }

  bool operator==(GMJobRef const& other) const {
    return (job_ == other.job_);
  }

  bool operator==(GMJob const* job) const {
    return (job_ == job);
  }

  operator bool() const {
    return job_ != NULL;
  }

  bool operator!() const {
    return job_ == NULL;
  }

  GMJob& operator*() const {
    return *job_;
  }

  GMJob* operator->() const {
    return job_;
  }

  void Destroy() {
    if (job_) job_->DestroyReference();
    job_ = NULL;
  }
};


class GMJobQueue {
 friend class GMJob;
 private:
  // Using global lock intentionally.
  // It would be possible to have per-queue lock but rules to avoid 
  // deadlocks between 2 queues and queue+job locks would be too complex
  // and too easy to break. So as long as we have not so many queues 
  // global lock is acceptable.
  static Glib::RecMutex lock_;
  int const priority_;
  std::list<GMJob*> queue_;
  std::string name_;
  GMJobQueue();
  GMJobQueue(GMJobQueue const& it);
 public:
  //! Construct jobs queue with specified priority.
  GMJobQueue(int priority, char const * name);

  //! Comparison function type definition.
  typedef bool (*comparator_t)(GMJobRef const& first, GMJobRef const& second);

  //! Insert job at end of the queue. Subject to queue priority.
  bool Push(GMJobRef& ref);

  //! Insert job into queue at position defined by sorting. Subject to queue priority.
  bool PushSorted(GMJobRef& ref, comparator_t compare);

  //! Returns reference to first job in the queue.
  GMJobRef Front();

  //! Removes first job n the queue and returns its reference.
  GMJobRef Pop();

  //! Insert job at beginnign of the queue. Subject to queue priority.
  bool Unpop(GMJobRef& ref);

  //! Removes job from the queue
  bool Erase(GMJobRef& ref);

  //! Returns true if job is in queue
  bool Exists(const GMJobRef& ref) const;

  //! Sort jobs in queue
  void Sort(comparator_t compare);

  //! Removes job from queue identified by key
  template<typename KEY> bool Erase(KEY const& key) {
    Glib::RecMutex::Lock lock(lock_);
    for(std::list<GMJob*>::iterator i = queue_.begin();
                       i != queue_.end(); ++i) {
      if((*i) && (**i == key)) {
        (*i)->SwitchQueue(NULL);
        return true;
      };
    };
    return false;
  };

  //! Gets reference to job identified by key and stored in this queue
  template<typename KEY> GMJobRef Find(KEY const& key) const {
    Glib::RecMutex::Lock lock(lock_);
    for(std::list<GMJob*>::const_iterator i = queue_.begin();
                       i != queue_.end(); ++i) {
      if((*i) && (**i == key)) {
        return GMJobRef(*i);
      };
    };
    return GMJobRef();
  };
};


} // namespace ARex

#endif
