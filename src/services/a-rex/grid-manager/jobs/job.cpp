/*
  Filename: states.cc
  keeps list of states
  acts on states
*/

//@ #include "../std.h"

#include <string>

#include "../run/run.h"
#include "../files/info_types.h"
#include "../files/info_files.h"
// #include "users.h"
#include "job.h"


const char* state_names[JOB_STATE_NUM] = {
 "ACCEPTED",
 "PREPARING",
 "SUBMIT",
 "INLRMS",
 "FINISHING",
 "FINISHED",
 "DELETED",
 "CANCELING",
 "UNDEFINED"
};

job_state_rec_t states_all[JOB_STATE_UNDEFINED+1] = {
 { JOB_STATE_ACCEPTED,  state_names[JOB_STATE_ACCEPTED],   ' ' },
 { JOB_STATE_PREPARING, state_names[JOB_STATE_PREPARING],  'b' },
 { JOB_STATE_SUBMITING, state_names[JOB_STATE_SUBMITING],     ' ' },
 { JOB_STATE_INLRMS,    state_names[JOB_STATE_INLRMS],     'q' },
 { JOB_STATE_FINISHING, state_names[JOB_STATE_FINISHING],  'f' },
 { JOB_STATE_FINISHED,  state_names[JOB_STATE_FINISHED],   'e' },
 { JOB_STATE_DELETED,   state_names[JOB_STATE_DELETED],    'd' },
 { JOB_STATE_CANCELING, state_names[JOB_STATE_CANCELING],  'c' },
 { JOB_STATE_UNDEFINED, NULL, ' '}
};


const char* JobDescription::get_state_name() const {
  if((job_state<0) || (job_state>=JOB_STATE_NUM))
                       return state_names[JOB_STATE_UNDEFINED];
  return state_names[job_state];
}

const char* JobDescription::get_state_name(job_state_t st) {
  if((st<0) || (st>=JOB_STATE_NUM))
                       return state_names[JOB_STATE_UNDEFINED];
  return state_names[st];
}

job_state_t JobDescription::get_state(const char* state) {
  for(int i = 0;i<JOB_STATE_NUM;i++) {
    if(!strcmp(state_names[i],state)) return (job_state_t)i;
  };
  return JOB_STATE_UNDEFINED;
}

JobDescription::JobDescription(void) {
  job_state=JOB_STATE_UNDEFINED;
  job_pending=false;
  child=NULL;
  local=NULL;
  job_uid=0; job_gid=0;
}

JobDescription::JobDescription(const JobDescription &job) {
  job_state=job.job_state;
  job_pending=job.job_pending;
  job_id=job.job_id;
  session_dir=job.session_dir;
  failure_reason=job.failure_reason;
  keep_finished=job.keep_finished;
  keep_deleted=job.keep_deleted;
  child=NULL;
  local=job.local;
  job_uid=job.job_uid; job_gid=job.job_gid;
}

JobDescription::JobDescription(const JobId &id,const std::string &dir,job_state_t state) {
  job_state=state;
  job_pending=false;
  job_id=id;
  session_dir=dir;
  keep_finished=DEFAULT_KEEP_FINISHED;
  keep_deleted=DEFAULT_KEEP_DELETED;
  child=NULL;
  local=NULL;
  job_uid=0; job_gid=0;
}

JobDescription::~JobDescription(void){
 /* child is not destroyed here */
}

bool JobDescription::GetLocalDescription(const JobUser &user) {
  if(local) return true;
  JobLocalDescription* job_desc;
  job_desc=new JobLocalDescription;
  if(!job_local_read_file(job_id,user,*job_desc)) {
    delete job_desc;
    return false;
  };
  local=job_desc;
  return true;
}

