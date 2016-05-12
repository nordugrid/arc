#ifndef __ARC_JOB_PERF_LOGGER__
#define __ARC_JOB_PERF_LOGGER__

#include <time.h>
#include <string>

namespace Arc {

class JobPerfLog {
 public:
  JobPerfLog();
  
  ~JobPerfLog();
  
  void SetOutput(const std::string& filename);

  void SetEnabled(bool enabled);

  const std::string& GetOutput() const { return log_path; };

  bool GetEnabled() const { return log_enabled; };

  /** Log one performance record. */
  void Log(const std::string& name, const std::string& id, const timespec& start, const timespec& end);


 private:
   std::string log_path;
   bool log_enabled;

};

class JobPerfRecord {
 public:
  /** Creates object */
  JobPerfRecord(JobPerfLog& log);

  /** Creates object and calls Start() */
  JobPerfRecord(JobPerfLog& log, const std::string& id);

  /** Prepare to log one record by remembering start time of action being measured. */
  void Start(const std::string& id);

  /** Log performance record started by previous LogStart(). */
  void End(const std::string& name);

  bool Started() { return start_recorded; };

 private:
   JobPerfLog& perf_log;
   bool start_recorded;
   timespec start_time;
   std::string start_id;

};

} // namespace Arc

#endif // __ARC_JOB_PERF_LOGGER__
