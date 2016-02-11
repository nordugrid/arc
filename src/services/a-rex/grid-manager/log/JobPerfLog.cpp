#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdint.h>
#include <fstream>

#include <arc/DateTime.h>

#include "JobPerfLog.h"

namespace ARex {


JobPerfLog::JobPerfLog() {
}

JobPerfLog::~JobPerfLog() {
}

void JobPerfLog::SetOutput(const std::string& filename) {
  log_path = filename;
}

void JobPerfLog::SetEnabled(bool enabled) {
  if(enabled != log_enabled) {
    log_enabled = enabled;
  };
}

JobPerfRecord::JobPerfRecord(JobPerfLog& log):perf_log(log) {
  start_recorded = false;
}

JobPerfRecord::JobPerfRecord(JobPerfLog& log, const std::string& id):perf_log(log) {
  Start(id);
}

void JobPerfRecord::Start(const std::string& id) {
  start_recorded = false;
  if(&perf_log == NULL) return;
  if(!perf_log.GetEnabled()) return;
  if((clock_gettime(CLOCK_MONOTONIC, &start_time) == 0) || (clock_gettime(CLOCK_REALTIME, &start_time) == 0)) {
    start_recorded = true;
    start_id = id;
  };
  start_id = id;
}

void JobPerfRecord::End(const std::string& name) {
  if(start_recorded) {
    timespec end_time;
    if((clock_gettime(CLOCK_MONOTONIC, &end_time) == 0) || (clock_gettime(CLOCK_REALTIME, &end_time) == 0)) {
      perf_log.Log(name, start_id, start_time, end_time);
    };
    start_recorded = false;
  };
}

void JobPerfLog::Log(const std::string& name, const std::string& id, const timespec& start, const timespec& end) {
  if(!log_enabled) return;
  if(log_path.empty()) return;
  std::ofstream logout(log_path.c_str(), std::ofstream::app);
  if(logout.is_open()) {
    uint64_t delta = ((uint64_t)(end.tv_sec-start.tv_sec))*1000000000 + end.tv_nsec - start.tv_nsec;
    logout << Arc::Time().str(Arc::UTCTime) << "\t" << name << "\t" << id << "\t" << delta << " nS" << std::endl;
  };
}

} // namespace ARex

