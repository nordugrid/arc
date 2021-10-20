#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <fcntl.h>

#include <arc/Logger.h>

#include "RunRedirected.h"

namespace ARex {

static Arc::Logger& logger = Arc::Logger::getRootLogger();

int RunRedirected::run(const Arc::User& user,const char* cmdname,int in,int out,int err,char *const args[],int timeout) {
  std::list<std::string> args_;
  for(int n = 0;args[n];++n) args_.push_back(std::string(args[n]));
  Arc::Run re(args_);
  if(!re) {
    logger.msg(Arc::ERROR,"%s: Failure creating slot for child process",cmdname?cmdname:"");
    return -1;
  };
  RunRedirected* rr = new RunRedirected(in,out,err);
  if((!rr) || (!(*rr))) {
    if(rr) delete rr;
    logger.msg(Arc::ERROR,"%s: Failure creating data storage for child process",cmdname?cmdname:"");
    return -1;
  };
  re.AssignInitializer(&initializer,rr,false);
  re.AssignUserId(user.get_uid());
  re.AssignGroupId(user.get_gid());
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
    re.Kill(5);
    return -1;
  };
  return re.Result();
}

int RunRedirected::run(const Arc::User& user,const char* cmdname,int in,int out,int err,const char* cmd,int timeout) {
  Arc::Run re(cmd);
  if(!re) {
    logger.msg(Arc::ERROR,"%s: Failure creating slot for child process",cmdname?cmdname:"");
    return -1;
  };
  RunRedirected* rr = new RunRedirected(cmdname,in,out,err);
  if((!rr) || (!(*rr))) {
    if(rr) delete rr;
    logger.msg(Arc::ERROR,"%s: Failure creating data storage for child process",cmdname?cmdname:"");
    return -1;
  };
  re.AssignInitializer(&initializer,rr,false);
  re.AssignUserId(user.get_uid());
  re.AssignGroupId(user.get_gid());
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
    re.Kill(5);
    return -1;
  };
  return re.Result();
}

void RunRedirected::initializer(void* arg) {
  // There must be only async-safe calls here!
  // child
  RunRedirected* it = (RunRedirected*)arg;
  // set up stdin,stdout and stderr
  if(it->stdin_ != -1)  dup2(it->stdin_,0);
  if(it->stdout_ != -1) dup2(it->stdout_,1);
  if(it->stderr_ != -1) dup2(it->stderr_,2);
}

} // namespace ARex
