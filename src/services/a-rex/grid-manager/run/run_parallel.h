#ifndef GRID_MANAGER_RUN_PARALLEL_H
#define GRID_MANAGER_RUN_PARALLEL_H

//@ #include "../std.h"
#include "../jobs/users.h"
#include "../jobs/states.h"
#include <sys/resource.h>
#include <sys/wait.h>
#include <pthread.h>

#include "run.h"
#include "run_plugin.h"

extern char** environ;

class RunParallel:public Run {
 public:
  static bool run(JobUser& user,const char* jobid,char *const args[],RunElement**,bool su = true,bool job_proxy = true, RunPlugin* cred = NULL, RunPlugin::substitute_t subst = NULL, void* subst_arg = NULL);
  static bool run(JobUser& user,const JobDescription& desc,char *const args[],RunElement**,bool su = true);
  RunParallel(pthread_cond_t *cond = NULL):Run(cond) { };
  ~RunParallel() { };
};

#endif
