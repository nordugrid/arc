#ifndef __ARC_GM_PLUGINS_H__
#define __ARC_GM_PLUGINS_H__

#include <string>

class ContinuationPlugins {
 public:
  typedef enum {
    act_fail,
    act_pass,
    act_log,
    act_undefined
  } action_t;
  class result_t {
   public:
    action_t action;
    int result;
    std::string response;
    result_t(action_t act,int res,const std::string& resp):
                           action(act),result(res),response(resp) { };
    result_t(action_t act):
                           action(act),result(0) { };
  };
 private:
  class command_t {
   public:
    std::string cmd;
    unsigned int to;
    action_t onsuccess;
    action_t onfailure;
    action_t ontimeout;
  };
  std::list<command_t> commands[JOB_STATE_NUM];
 public:
  ContinuationPlugins(void); 
  ~ContinuationPlugins(void); 
  bool add(job_state_t state,unsigned int timeout,const char* command);
  bool add(const char* state,unsigned int timeout,const char* command);
  bool add(job_state_t state,const char* options,const char* command);
  bool add(const char* state,const char* options,const char* command);
  void run(const JobDescription &job,const JobUser& user,std::list<result_t>& results);
};

#endif
