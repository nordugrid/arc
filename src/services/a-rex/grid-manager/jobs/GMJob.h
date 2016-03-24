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

/// Represents a job in memory as it passes through the JobsList state machine.
class GMJob {
 friend class JobsList;
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
  GMJob& operator=(const GMJob &job);
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
  JobLocalDescription* get_local(void) const { return local; };
  void set_local(JobLocalDescription* desc) { local=desc; };
  bool operator==(const GMJob& job) const { return (job_id == job.job_id); };
  bool operator==(const JobId &id) const { return (job_id == id); };
  bool operator!=(const JobId &id) const { return (job_id != id); };
  void set_user(const Arc::User& u) { user = u; }
  const Arc::User& get_user() const { return user;}
  void set_share(std::string share);
  // Force 'local' to be created and read from file if not already available
  bool GetLocalDescription(const GMConfig& config);
  void Start() { start_time = time(NULL); };
  time_t GetStartTime() const { return start_time; };
  void PrepareToDestroy(void);
};

} // namespace ARex

#endif
