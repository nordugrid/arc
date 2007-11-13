#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/Run.h>
#include "../jobs/job.h"
#include "../jobs/states.h"
#include "../jobs/users.h"

#include "plugins.h"

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
     (state == JOB_STATE_SUBMITING) ||
     (state == JOB_STATE_FINISHING) ||
     (state == JOB_STATE_FINISHED) ||
     (state == JOB_STATE_DELETED)) {
    commands[state].cmd=command;
    commands[state].to=timeout;
    commands[state].onsuccess=act_pass;
    commands[state].onfailure=act_fail;
    commands[state].ontimeout=act_fail;
  } else { return false; };
  return true;
}

bool ContinuationPlugins::add(const char* state,unsigned int timeout,const char* command) {
  job_state_t i = JobDescription::get_state(state);
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
     (state == JOB_STATE_SUBMITING) ||
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
  commands[state].cmd=command;
  commands[state].to=to;
  commands[state].onsuccess=onsuccess;
  commands[state].onfailure=onfailure;
  commands[state].ontimeout=ontimeout;
  return true;
}

bool ContinuationPlugins::add(const char* state,const char* options,const char* command) {
  job_state_t i = JobDescription::get_state(state);
  if(i != JOB_STATE_UNDEFINED) {
    return add(i,options,command);
  };
  return false;
}

ContinuationPlugins::action_t ContinuationPlugins::run(const JobDescription &job,const JobUser& user,int& result,std::string& response) {
  job_state_t state = job.get_state();
  response.resize(0);
  if(commands[state].cmd.length() == 0) {
    result=0; return act_pass;
  };
  std::string cmd = commands[state].cmd;
  for(std::string::size_type p = 0;;) {
    p=cmd.find('%',p);
    if(p==std::string::npos) break;
    if(cmd[p+1]=='I') {
      cmd.replace(p,2,job.get_id().c_str());
      p+=job.get_id().length();
    } else if(cmd[p+1]=='S') {
      cmd.replace(p,2,job.get_state_name());
      p+=strlen(job.get_state_name());
    } else {
      p+=2;
    };
  };
  if(!user.substitute(cmd)) {
    return act_undefined;
  };
  std::string res_out("");
  std::string res_err("");
  int to = commands[state].to;
  bool r = false;
  bool t = false;
  {
    Arc::Run re(cmd);
    re.AssignStdout(res_out);
    re.AssignStderr(res_err);
    re.KeepStdin();
    if(re.Start()) {
      if(re.Wait(to)) {
        r=true;
      } else {
        t=true;
      };
    };
  };
  response=res_out;
  if(res_err.length()) {
    if(response.length()) response+=" : ";
    response+=res_err;
  };
  if(!r) { // failure
    if(t) { // timeout occured
      if(response.length()) { response="TIMEOUT : "+response; }
      else { response="TIMEOUT"; };
      return commands[state].ontimeout;
    };
    return act_undefined;
  };
  if(result == 0) return commands[state].onsuccess;
  if(response.length()) { response="FAILED : "+response; }
  else { response="FAILED"; };
  return commands[state].onfailure; 
}
