#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <arc/Logger.h>

#include "../conf/environment.h"
#include "run_redirected.h"

static Arc::Logger& logger = Arc::Logger::getRootLogger();

int RunRedirected::run(JobUser& user,const char* cmdname,int in,int out,int err,char *const args[],int timeout) {
  std::list<std::string> args_;
  for(int n = 0;args[n];++n) args_.push_back(std::string(args[n]));
  Arc::Run re(args_);
  if(!re) {
    logger.msg(Arc::ERROR,"%s: Failure creating slot for child process",cmdname?cmdname:"");
    return -1;
  };
  RunRedirected* rr = new RunRedirected(user,cmdname,in,out,err);
  if((!rr) || (!(*rr))) {
    if(rr) delete rr;
    logger.msg(Arc::ERROR,"%s: Failure creating data storage for child process",cmdname?cmdname:"");
    return -1;
  };
  re.AssignInitializer(&initializer,rr);
  re.KeepStdin(true);
  re.KeepStdout(true);
  re.KeepStderr(true);
  if(!re.Start()) {
    delete rr;
    logger.msg(Arc::ERROR,"%s: Failure starting child process",cmdname?cmdname:"");
    return -1;
  };
  delete rr;
  if(!re.Wait(timeout)) {
    logger.msg(Arc::ERROR,"%s: Failure waiting for child process to finish",cmdname?cmdname:"");
    return -1;
  };
  return re.Result();
}

int RunRedirected::run(JobUser& user,const char* cmdname,int in,int out,int err,const char* cmd,int timeout) {
  Arc::Run re(cmd);
  if(!re) {
    logger.msg(Arc::ERROR,"%s: Failure creating slot for child process",cmdname?cmdname:"");
    return -1;
  };
  RunRedirected* rr = new RunRedirected(user,cmdname,in,out,err);
  if((!rr) || (!(*rr))) {
    if(rr) delete rr;
    logger.msg(Arc::ERROR,"%s: Failure creating data storage for child process",cmdname?cmdname:"");
    return -1;
  };
  re.AssignInitializer(&initializer,rr);
  re.KeepStdin(true);
  re.KeepStdout(true);
  re.KeepStderr(true);
  if(!re.Start()) {
    delete rr;
    logger.msg(Arc::ERROR,"%s: Failure starting child process",cmdname?cmdname:"");
    return -1;
  };
  delete rr;
  if(!re.Wait(timeout)) {
    logger.msg(Arc::ERROR,"%s: Failure waiting for child process to finish",cmdname?cmdname:"");
    return -1;
  };
  return re.Result();
}

void RunRedirected::initializer(void* arg) {
#ifdef WIN32
#error This functionality is not available in Windows environement
#else
  // child
  RunRedirected* it = (RunRedirected*)arg;
  struct rlimit lim;
  int max_files;
  if(getrlimit(RLIMIT_NOFILE,&lim) == 0) { max_files=lim.rlim_cur; }
  else { max_files=4096; };
  // change user
  if(!(it->user_.SwitchUser(true))) {
    logger.msg(Arc::ERROR,"%s: Failed switching user",it->cmdname_); sleep(10); exit(1);
  };
  // set up stdin,stdout and stderr
  if(it->stdin_ != -1)  dup2(it->stdin_,0);
  if(it->stdout_ != -1) dup2(it->stdout_,1);
  if(it->stderr_ != -1) dup2(it->stderr_,2);
  // close all handles inherited from parent
  if(max_files == RLIM_INFINITY) max_files=4096;
  for(int i=3;i<max_files;i++) { close(i); };
#endif
}

