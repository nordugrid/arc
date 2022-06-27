#ifndef GRID_MANAGER_RUN_PARALLEL_H
#define GRID_MANAGER_RUN_PARALLEL_H

#include <arc/Run.h>

#include "../jobs/JobsList.h"

namespace ARex {

/// Run child process in parallel with stderr redirected to job.jobid.errors
class RunParallel {
 private:
  RunParallel() { };
  ~RunParallel(void) { };
  static void initializer(void* arg);
  operator bool(void) { return true; };
  bool operator!(void) { return false; };
  static bool run(const GMConfig& config, const Arc::User& user,
                  const char* procid, const char* errlog, std::string* errstr,
                  const std::string& args, Arc::Run**,
                  const char* job_proxy, bool su = true,
                  void (*kicker_func)(void*) = NULL, void* kicker_arg = NULL);
 public:
  static bool run(const GMConfig& config, const GMJob& job, JobsList& list, std::string* errstr,
                  const std::string& args, Arc::Run**,
                  bool su = true);
  static bool run(const GMConfig& config, const GMJob& job, std::string* errstr,
                  const std::string& args, Arc::Run**,
                  bool su = true);
};

} // namespace ARex

#endif
