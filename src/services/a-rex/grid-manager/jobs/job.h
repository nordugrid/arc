#ifndef GRID_MANAGER_JOB_H
#define GRID_MANAGER_JOB_H

#include <sys/types.h>
#include <string>

#include <arc/Run.h>
#include <arc/User.h>

class JobsList;
class JobLocalDescription;
class GMConfig;

typedef enum {
  JOB_STATE_ACCEPTED   = 0,
  JOB_STATE_PREPARING  = 1,
  JOB_STATE_SUBMITTING = 2,
  JOB_STATE_INLRMS     = 3,
  JOB_STATE_FINISHING  = 4,
  JOB_STATE_FINISHED   = 5,
  JOB_STATE_DELETED    = 6,
  JOB_STATE_CANCELING  = 7,
  JOB_STATE_UNDEFINED  = 8
} job_state_t;

typedef struct {
  job_state_t id;
  const char* name;
  char mail_flag;
} job_state_rec_t;

extern job_state_rec_t states_all[JOB_STATE_UNDEFINED+1];

#define JOB_STATE_NUM (JOB_STATE_UNDEFINED+1)

/*
  Jobs identifier. Stored as string. Normally is a big number.
*/
typedef std::string JobId;

/*
  Object to represent job in memory.
*/
class JobDescription {
 friend class JobsList;
 private:
  /* state of the job (state machine) */
  job_state_t job_state;
  /* flag to indicate job stays at this stage due to limits imposed
     such jobs are not counted in couters */
  bool job_pending; 
  /* identifier */
  JobId job_id;
  /* directory to run job in */
  std::string session_dir;
  /* explanation of job's failure */
  std::string failure_reason;
  /* how long job is kept on cluster after it finished */
  time_t keep_finished;
  time_t keep_deleted;
  /* pointer to object containing most important parameters of job,
     loaded when needed. */
  JobLocalDescription* local;
  /* job's owner */
  Arc::User user;
  /* reties left (should be reset for each state change) */
  int retries;
  /* time to perform the next retry */
  time_t next_retry;
  /* used to determine data transfer share (eg DN, VOMS VO) */
  std::string transfer_share;
  /* start time of job i.e. when it first moves to PREPARING */
  time_t start_time;
 public:
  /* external utility beeing run to perform tasks like stage-in/our, 
     submit/cancel. (todo - move to private) */
  Arc::Run* child;
  /* 
    Constructors and destructor.
    Accepts:
      job_id - identifier
      dir - session_dir of job
      state - initial state of job
  */
  JobDescription(void);
  JobDescription(const JobDescription &job);
  JobDescription(const JobId &job_id,const Arc::User& user,const std::string &dir = "",job_state_t state = JOB_STATE_UNDEFINED);
  ~JobDescription(void);
  job_state_t get_state() const { return job_state; };
  const char* get_state_name() const;
  static const char* get_state_name(job_state_t st);
  static job_state_t get_state(const char* state);
  const JobId& get_id() const { return job_id; };
  std::string SessionDir(void) const { return session_dir; };
  void AddFailure(const std::string &reason) {
    //if(failure_reason.length()) failure_reason+="\n";
    failure_reason+=reason; failure_reason+="\n";
  };
  std::string GetFailure(const GMConfig& config) const;
  JobLocalDescription* get_local(void) const { return local; };
  void set_local(JobLocalDescription* desc) { local=desc; };
//  void set_state(job_state_t state) { job_state=state; };
  bool operator==(const JobDescription& job) const { return (job_id == job.job_id); };
  bool operator==(const JobId &id) const { return (job_id == id); };
  bool operator!=(const JobId &id) const { return (job_id != id); };
  void set_user(const Arc::User& u) { user = u; }
  const Arc::User& get_user() const { return user;}
  void set_share(std::string share);
  /* force 'local' to be created and read from file if not already available */
  bool GetLocalDescription(const GMConfig& config);
  void Start() { start_time = time(NULL); };
  time_t GetStartTime() const { return start_time; };
  void PrepareToDestroy(void);
};
#endif
