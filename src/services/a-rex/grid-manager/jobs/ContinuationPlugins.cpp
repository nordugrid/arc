#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cstring>

#include <arc/Run.h>
#include "../jobs/GMJob.h"
#include "../jobs/JobsList.h"
#include "../conf/GMConfig.h"

#include "ContinuationPlugins.h"

namespace ARex {

/*
  Substitution:
   %I - job id
*/


ContinuationPlugins::ContinuationPlugins(void) {
}

ContinuationPlugins::~ContinuationPlugins(void) {
}

bool ContinuationPlugins::add(job_state_t state,unsigned int timeout,const char* command) {
  if((state == JOB_STATE_ACCEPTED) ||
     (state == JOB_STATE_PREPARING) ||
     (state == JOB_STATE_SUBMITTING) ||
     (state == JOB_STATE_INLRMS) ||
     (state == JOB_STATE_FINISHING) ||
     (state == JOB_STATE_FINISHED) ||
     (state == JOB_STATE_DELETED)) {
    command_t cmd;
    cmd.cmd=command;
    cmd.to=timeout;
    cmd.onsuccess=act_pass;
    cmd.onfailure=act_fail;
    cmd.ontimeout=act_fail;
    commands[state].push_back(cmd);
  } else { return false; };
  return true;
}

bool ContinuationPlugins::add(const char* state,unsigned int timeout,const char* command) {
  job_state_t i = GMJob::get_state(state);
  if(i != JOB_STATE_UNDEFINED) {
    return add(i,timeout,command);
  };
  return false;
}

static ContinuationPlugins::action_t get_action(const char *s,unsigned int l) {
  if((l == 4) && (strncasecmp(s,"fail",4) == 0)) return ContinuationPlugins::act_fail;
  if((l == 4) && (strncasecmp(s,"pass",4) == 0)) return ContinuationPlugins::act_pass;
  if((l == 3) && (strncasecmp(s,"log",3) == 0)) return ContinuationPlugins::act_log;
  return ContinuationPlugins::act_undefined;
}

#define RES_ONSUCCESS 0
#define RES_ONFAILURE 1
#define RES_ONTIMEOUT 2
#define RES_TIMEOUT 3
#define RES_UNDEFINED -1
static int get_result(const char *s,unsigned int l) {
  if((l == 9) && (strncasecmp(s,"onsuccess",9) == 0)) return RES_ONSUCCESS;
  if((l == 9) && (strncasecmp(s,"onfailure",9) == 0)) return RES_ONFAILURE;
  if((l == 9) && (strncasecmp(s,"ontimeout",9) == 0)) return RES_ONTIMEOUT;
  if((l == 7) && (strncasecmp(s,"timeout",7) == 0)) return RES_TIMEOUT;
  return RES_UNDEFINED;
}

bool ContinuationPlugins::add(job_state_t state,const char* options,const char* command) {
  if((state == JOB_STATE_ACCEPTED) ||
     (state == JOB_STATE_PREPARING) ||
     (state == JOB_STATE_SUBMITTING) ||
     (state == JOB_STATE_INLRMS) ||
     (state == JOB_STATE_FINISHING) ||
     (state == JOB_STATE_FINISHED) ||
     (state == JOB_STATE_DELETED)) {
  } else { return false; };
  // go through options separated by ','
  action_t onsuccess = act_pass;
  action_t onfailure = act_fail;
  action_t ontimeout = act_fail;
  unsigned int to = 0;
  const char *opt_p = options;
  for(;*opt_p;) {
    const char *next_opt_p = strchr(opt_p,',');
    if(next_opt_p == NULL) next_opt_p=opt_p+strlen(opt_p);
    const char *val_p = strchr(opt_p,'=');
    unsigned int name_len;
    unsigned int val_len;
    if((val_p == NULL) || (val_p >= next_opt_p)) {
      name_len = next_opt_p-opt_p;
      val_p=next_opt_p;
      val_len=0;
    } else {
      name_len = val_p-opt_p;
      val_p++;
      val_len=next_opt_p-val_p;
    };
    action_t act = act_undefined;
    int res = get_result(opt_p,name_len);
    if(res == RES_UNDEFINED) { // can be timeout
      if(val_len != 0) return false;
      res=RES_TIMEOUT;
      val_p=opt_p;
      val_len=next_opt_p-val_p;
    };
    if(res != RES_TIMEOUT) {
      act=get_action(val_p,val_len);
      if(act == act_undefined) return false;
    };
    switch(res) {
      case RES_ONSUCCESS: onsuccess=act; break;
      case RES_ONFAILURE: onfailure=act; break;
      case RES_ONTIMEOUT: ontimeout=act; break;
      case RES_TIMEOUT: {
        if(val_len > 0) {
          char* e;
          to=strtoul(val_p,&e,0);
          if(e != next_opt_p) return false;
        } else { to=0; };
      }; break;
      default: return false;
    };
    opt_p=next_opt_p; if(!(*opt_p)) break;
    opt_p++;
  };
  command_t cmd;
  cmd.cmd=command;
  cmd.to=to;
  cmd.onsuccess=onsuccess;
  cmd.onfailure=onfailure;
  cmd.ontimeout=ontimeout;
  commands[state].push_back(cmd);
  return true;
}

bool ContinuationPlugins::add(const char* state,const char* options,const char* command) {
  job_state_t i = GMJob::get_state(state);
  if(i != JOB_STATE_UNDEFINED) {
    return add(i,options,command);
  };
  return false;
}

void ContinuationPlugins::run(const GMJob &job,const GMConfig& config,std::list<result_t>& results) {
  job_state_t state = job.get_state();
  for(std::list<command_t>::iterator command = commands[state].begin();
                     command != commands[state].end();++command) {
    action_t act = act_pass;
    if(command->cmd.length() == 0) {
      results.push_back(result_t(act_pass));
      continue;
    };
    std::string cmd = command->cmd;
    for(std::string::size_type p = 0;;) {
      p=cmd.find('%',p);
      if(p==std::string::npos) break;
      if(cmd[p+1]=='I') {
        cmd.replace(p,2,job.get_id().c_str());
        p+=job.get_id().length();
      } else if(cmd[p+1]=='S') {
        cmd.replace(p,2,job.get_state_name());
        p+=strlen(job.get_state_name());
      } else if(cmd[p+1]=='R') {
        // Get correct session root (without job subdir) for this job
        std::string sessionroot(job.SessionDir().substr(0, job.SessionDir().rfind('/')));
        cmd.replace(p,2,sessionroot);
        p+=sessionroot.length();
      } else {
        p+=2;
      };
    };
    if(!config.Substitute(cmd, job.get_user())) {
      results.push_back(result_t(act_undefined));
      continue; // or break ?
    };
    std::string res_out("");
    std::string res_err("");
    int to = command->to;
    int result = -1;

    Arc::Run re(cmd);
    re.AssignStdout(res_out);
    re.AssignStderr(res_err);
    re.KeepStdin();
    std::string response;
    if(re.Start()) {
      bool r = to?re.Wait(to):re.Wait();
      if(!r) {
        response="TIMEOUT";
        act=command->ontimeout;
      } else {
        result=re.Result();
        if(result == 0) {
          act=command->onsuccess;
        } else {
          response="FAILED";
          act=command->onfailure;
        };
      };
    } else {
      response="FAILED to start plugin";
      // act=command->onfailure; ?? 
      act=act_undefined;
    };
    if(!res_out.empty()) {
      if(!response.empty()) response+=" : ";
      response+=res_out;
    };
    if(!res_err.empty()) {
      if(!response.empty()) response+=" : ";
      response+=res_err;
    };
    results.push_back(result_t(act,result,response));
    if(act == act_fail) break;
  };
}

} // namespace ARex
