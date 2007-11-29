#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <unistd.h>

#include <arc/Logger.h>
#include <arc/Run.h>

#include "run_function.h"


Arc::Logger& logger = Arc::Logger::getRootLogger();


int RunFunction::run(const JobUser& user,const char* cmdname,int (*func)(void*),void* arg,int timeout) {
  if(func == NULL) return -1;
  std::string fake_cmd("/bin/true");
  Arc::Run re(fake_cmd);
  if(!re) {
    logger.msg(Arc::ERROR,"%s: Failure creating slot for child process.",cmdname?cmdname:"");
    return -1;
  };
  RunFunction* rf = new RunFunction(user,cmdname,func,arg);
  if(!rf) {
    logger.msg(Arc::ERROR,"%s: Failure creating data storage for child process.",cmdname?cmdname:"");
    return -1;
  };
  re.AssignInitializer(&initializer,rf);
  re.KeepStdin(true);
  re.KeepStdout(true);
  re.KeepStderr(true);
  if(!re.Start()) {
    delete rf;
    logger.msg(Arc::ERROR,"%s: Failure starting child process.",cmdname?cmdname:"");
    return -1;
  };
  delete rf;
  if(!re.Wait(timeout)) {
    logger.msg(Arc::ERROR,"%s: Failure waiting for child process to finish.",cmdname?cmdname:"");
    return -1;
  };
  return re.Result();
}

void RunFunction::initializer(void* arg) {
#ifdef WIN32
#error This functionality is not available in Windows environment
#else
  // child
  RunFunction* it = (RunFunction*)arg;
  /* change user */
  if(!(it->user_.SwitchUser(true))) {
    std::cerr<<it->cmdname_<<": Failed switching user"<<std::endl;
    _exit(-1);
  };
  int r = (*(it->func_))(it->arg_);
  _exit(r);
#endif
}

