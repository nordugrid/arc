#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cstring>

#include <arc/StringConv.h>

#include "JobsMetrics.h"

namespace ARex {

static Arc::Logger& logger = Arc::Logger::getRootLogger();

JobsMetrics::JobsMetrics():enabled(false),proc(NULL) {
  std::memset(jobs_processed, 0, sizeof(jobs_processed));
  std::memset(jobs_in_state, 0, sizeof(jobs_in_state));
  std::memset(jobs_processed_changed, 0, sizeof(jobs_processed_changed));
  std::memset(jobs_in_state_changed, 0, sizeof(jobs_in_state_changed));
}

JobsMetrics::~JobsMetrics() {
}

void JobsMetrics::SetEnabled(bool val) {
  enabled = val;
}

void JobsMetrics::SetConfig(const char* fname) {
  config_filename = fname;
}

void JobsMetrics::SetPath(const char* path) {
  tool_path = path;
}

static const char* gmetric_tool = "gmetric";

void JobsMetrics::ReportJobStateChange(job_state_t new_state, job_state_t old_state) {
  Glib::RecMutex::Lock lock_(lock);
  if(old_state < JOB_STATE_UNDEFINED) {
    ++(jobs_processed[old_state]);
    jobs_processed_changed[old_state] = true;
    --(jobs_in_state[old_state]);
    jobs_in_state_changed[old_state] = true;
  };
  if(new_state < JOB_STATE_UNDEFINED) {
    ++(jobs_in_state[new_state]);
    jobs_in_state_changed[new_state] = true;
  };
  Sync();
}

bool JobsMetrics::CheckRunMetrics(void) {
  if(!proc) return true;
  if(proc->Running()) return false;
  int run_result = proc->Result();
  if(run_result != 0) {
   logger.msg(Arc::ERROR,": Metrics tool returned error code %i: %s",run_result,proc_stderr);
  };
  proc = NULL;
  return true;
}

void JobsMetrics::Sync(void) {
  if(!enabled) return; // not configured
  Glib::RecMutex::Lock lock_(lock);
  if(!CheckRunMetrics()) return;
  // Run gmetric to report one change at a time
  std::list<std::string> cmd;
  for(int state = 0; state < JOB_STATE_UNDEFINED; ++state) {
    if(jobs_processed_changed[state]) {
      if(RunMetrics(
          std::string("AREX-JOBS-PROCESSED-") + GMJob::get_state_name(static_cast<job_state_t>(state)),
          Arc::tostring(jobs_processed[state])
         )) {
        jobs_processed_changed[state] = false;
        break;
      };
    };
    if(jobs_in_state_changed[state]) {
      if(RunMetrics(
          std::string("AREX-JOBS-IN_STATE-") + GMJob::get_state_name(static_cast<job_state_t>(state)),
          Arc::tostring(jobs_in_state[state])
         )) {
        jobs_in_state_changed[state] = false;
        break;
      };
    };
  };
}
 
bool JobsMetrics::RunMetrics(const std::string name, const std::string& value) {
  if(proc) return false;
  std::list<std::string> cmd;
  if(tool_path.empty()) {
    cmd.push_back(gmetric_tool);
  } else {
    cmd.push_back(tool_path+G_DIR_SEPARATOR_S+gmetric_tool);
  };
  if(!config_filename.empty()) {
    cmd.push_back("-c");
    cmd.push_back(config_filename);
  };
  cmd.push_back("-n");
  cmd.push_back(name);
  cmd.push_back("-v");
  cmd.push_back(value);
  cmd.push_back("-t");//unit-type
  cmd.push_back("int32");
  cmd.push_back("-u");//unit
  cmd.push_back("jobs");
  
  proc = new Arc::Run(cmd);
  proc->AssignStderr(proc_stderr);
  proc->AssignKicker(&RunMetricsKicker, this);
  if(!(proc->Start())) {
    delete proc;
    proc = NULL;
    return false;
  };
  return true;
}

void JobsMetrics::RunMetricsKicker(void* arg) {
  JobsMetrics& it = *reinterpret_cast<JobsMetrics*>(arg);
  if(&it) {
    Glib::RecMutex::Lock lock_(it.lock);
    if(it.proc) {
      // Continue only if no failure in previous call.
      // Otherwise it can cause storm of failed calls.
      if(it.proc->Result() == 0) {
        it.Sync();
      };
    };
  };
}

} // namespace ARex
