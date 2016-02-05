#include <time.h>
#include <string>

namespace ARex {

class JobPerfLog {
 public:
  JobPerfLog();
  
  ~JobPerfLog();
  
  void SetOutput(const std::string& filename);

  void SetEnabled(bool enabled);

  bool GetEnabled(void) { return log_enabled; };

  /** Log one performance record. */
  void Log(const std::string& name, const std::string& id, const timespec& start, const timespec& end);

  /** Prepare to log one record by remembering start time of action being measured. */
  void LogStart();

  /** Log performance record started by previous LogStart(). */
  void LogEnd(const std::string& name, const std::string& id);

 private:
   std::string log_path;
   bool log_enabled;
   bool start_recorded;
   timespec start_time;

};

} // namespace ARex

