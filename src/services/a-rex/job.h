#include "../../libs/common/XMLNode.h"
#include "grid-manager/jobs/users.h"
#include "grid-manager/files/info_types.h"

namespace ARex {

class ARexGMConfig {
 private:
  JobUser *user_;
  bool readonly_;
  std::list<std::string> queues_;
  std::string grid_name_; // temporary solution
 public:
  ARexGMConfig(const std::string& config_file,const std::string& uname,const std::string& grid_name);
  ~ARexGMConfig(void);
  operator bool(void) const { return (user_ != NULL); };
  bool operator!(void) const { return (user_ == NULL); };
  JobUser* User(void) { return user_; };
  bool ReadOnly(void) const { return readonly_; };
  const std::string& GridName(void) const { return grid_name_; };
  const std::list<std::string>& Queues(void) const { return queues_; };
};


/** This class represents convenience interface to manage jobs 
  handled by Grid Manager. It works mostly through corresponding
  classes and functions of Grid Manager. */
class ARexJob {
 private:
  std::string id_;
  std::string failure_;
  bool allowed_to_see_;
  bool allowed_to_maintain_;
  /** Returns true if job exists and authorization was checked 
    without errors. Fills information about authorization in 
    this instance. */ 
  bool is_allowed(void);
  ARexGMConfig& config_;
  JobLocalDescription job_;
  bool make_job_id(void);
  bool delete_job_id(void);
 public:
  /** Create instance which is an interface to existing job */
  ARexJob(const std::string& id,ARexGMConfig& config);
  /** Create new job with provided JSDL description */
  ARexJob(Arc::XMLNode jsdl,ARexGMConfig& config);
  operator bool(void) { return !id_.empty(); };
  bool operator!(void) { return !id_.empty(); };
  /** Returns textual description of failure of last operation */
  std::string Failure(void) { return failure_; };
  /** Return ID assigned to job */
  std::string ID(void) { return id_; };
  /** Fills provided jsdl with job description */
  void GetDescription(Arc::XMLNode& jsdl);
  /** Cancel processing/execution of job */
  bool Cancel(void);
  /** Resume execution of job after error */
  bool Resume(void);
  /** Returns current state of job */
  std::string State(void);
};

}; // namespace ARex














