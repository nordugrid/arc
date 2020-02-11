#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdint.h>
#include <fstream>

#ifdef _MACOSX
#include <mach/clock.h>
#include <mach/mach.h>
#endif

#include <arc/DateTime.h>

#include "JobPerfLog.h"

namespace Arc {


JobPerfLog::JobPerfLog(): log_enabled(false) {
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
#ifdef _MACOSX // OS X does not have clock_gettime, use clock_get_time
  clock_serv_t cclock;
  mach_timespec_t mts;
  host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock);
  clock_get_time(cclock, &mts);
  mach_port_deallocate(mach_task_self(), cclock);
  start_time.tv_sec = mts.tv_sec;
  start_time.tv_nsec = mts.tv_nsec;
  {
#else
  if((clock_gettime(CLOCK_MONOTONIC, &start_time) == 0) || (clock_gettime(CLOCK_REALTIME, &start_time) == 0)) {
#endif
    start_recorded = true;
    start_id = id;
  };
  start_id = id;
}

void JobPerfRecord::End(const std::string& name) {
  if(start_recorded) {
    timespec end_time;
#ifdef _MACOSX // OS X does not have clock_gettime, use clock_get_time
    clock_serv_t cclock;
    mach_timespec_t mts;
    host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock);
    clock_get_time(cclock, &mts);
    mach_port_deallocate(mach_task_self(), cclock);
    end_time.tv_sec = mts.tv_sec;
    end_time.tv_nsec = mts.tv_nsec;
    {
#else
    if((clock_gettime(CLOCK_MONOTONIC, &end_time) == 0) || (clock_gettime(CLOCK_REALTIME, &end_time) == 0)) {
#endif
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
    logout << Arc::Time().str(Arc::UTCTime) << "\t" << name << "\t" << id << "\t" << delta << " ns" << std::endl;
  };
}

} // namespace Arc

