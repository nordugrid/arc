#ifndef GRID_MANAGER_RUN_REDIRECTED_H
#define GRID_MANAGER_RUN_REDIRECTED_H

#include <arc/Run.h>

#include "../jobs/users.h"
#include "../jobs/states.h"
#include "run_plugin.h"

class RunRedirected {
 private:
  RunRedirected(JobUser& user,const char* cmdname,int stdin,int stdout,int stderr):user_(user),cmdname_(cmdname?cmdname:""),stdin_(stdin),stdout_(stdout),stderr_(stderr) { };
  ~RunRedirected(void) { };
  JobUser& user_;
  std::string cmdname_;
  int stdin_;
  int stdout_;
  int stderr_;
  static void initializer(void* arg);
 public:
  operator bool(void) { return true; };
  bool operator!(void) { return false; };
  static int run(JobUser& user,const char* cmdname,int stdin,int stdout,int stderr,char *const args[],int timeout);
  static int run(JobUser& user,const char* cmdname,int stdin,int stdout,int stderr,const char* cmd,int timeoutd);
};

#endif
