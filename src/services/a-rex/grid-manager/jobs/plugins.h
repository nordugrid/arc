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
 private:
  typedef struct {
    std::string cmd;
    unsigned int to;
    action_t onsuccess;
    action_t onfailure;
    action_t ontimeout;
  } command_t;
  command_t commands[JOB_STATE_NUM];
 public:
  ContinuationPlugins(void); 
  ~ContinuationPlugins(void); 
  bool add(job_state_t state,unsigned int timeout,const char* command);
  bool add(const char* state,unsigned int timeout,const char* command);
  bool add(job_state_t state,const char* options,const char* command);
  bool add(const char* state,const char* options,const char* command);
  action_t run(const JobDescription &job,const JobUser& user,int& result,std::string& response);
};

#endif
