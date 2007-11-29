#ifndef GRID_MANAGER_RUN_FUNCTION_H
#define GRID_MANAGER_RUN_FUNCTION_H

#include <arc/Run.h>

#include "../jobs/users.h"

class RunFunction {
 private:
  RunFunction(const JobUser& user,const char* cmdname,int (*func)(void*),void* arg):user_(user),cmdname_(cmdname?cmdname:""),func_(func),arg_(arg) { };
  ~RunFunction(void) { };
  const JobUser& user_;
  std::string cmdname_;
  int (*func_)(void*);
  void* arg_;
  static void initializer(void* arg);
 public:
  operator bool(void) { return true; };
  bool operator!(void) { return false; };
  static int run(const JobUser& user,const char* cmdname,int (*func)(void*),void* arg,int timeout);

};

#endif
