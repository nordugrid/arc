#ifndef GRID_MANAGER_RUN_PARALLEL_H
#define GRID_MANAGER_RUN_PARALLEL_H

#include <arc/Run.h>

#include "../jobs/JobsList.h"
#include "RunPlugin.h"

namespace ARex {

/// Run child process in parallel with stderr redirected to job.jobid.errors
/// This class to be used only for running external executables associated with
/// particular job.
class RunParallel {
 private:
  RunParallel(const GMConfig& config, const Arc::User& user,
              const char* procid, const char* errlog,
              const char* jobproxy, bool su,
              RunPlugin* cred, RunPlugin::substitute_t subst, void* subst_arg)
    :config_(config), user_(user), procid_(procid?procid:""), errlog_(errlog?errlog:""),
     su_(su), jobproxy_(jobproxy?jobproxy:""),
     cred_(cred),
     subst_(subst), subst_arg_(subst_arg) { };
  ~RunParallel(void) { };
  const GMConfig& config_;
  const Arc::User& user_;
  std::string procid_;
  std::string errlog_;
  bool su_;
  std::string jobproxy_;
  RunPlugin* cred_;
  RunPlugin::substitute_t subst_;
  void* subst_arg_;
  static void initializer(void* arg);
  operator bool(void) { return true; };
  bool operator!(void) { return false; };
  static bool run(const GMConfig& config, const Arc::User& user,
                  const char* procid, const char* errlog,
                  const std::string& args, Arc::Run**,
                  const char* job_proxy, bool su = true,
                  RunPlugin* cred = NULL,
                  RunPlugin::substitute_t subst = NULL, void* subst_arg = NULL,
                  void (*kicker_func)(void*) = NULL, void* kicker_arg = NULL);
 public:
  static bool run(const GMConfig& config, const GMJob& job, JobsList& list,
                  const std::string& args, Arc::Run**,
                  bool su = true);
 public:
  static bool run(const GMConfig& config, const GMJob& job,
                  const std::string& args, Arc::Run**,
                  bool su = true);
};

} // namespace ARex

#endif
