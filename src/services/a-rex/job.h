
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
 public:
  /** Create instance which is an interface to existing job */
  ARexJob(const std::string& id);
  /** Create new job with provided JSDL description */
  ARexJob(XMLNode jsdl);
  operator bool(void) { return !id_.empty(); };
  bool operator!(void) { return !id_.empty(); };
  /** Returns textual description of failure of last operation */
  std::string Failure(void) { return failure_; };
  /** Return ID assigned to job */
  std::string ID(void) { return id_; };
  /** Fills provided jsdl with job description */
  void GetDescription(XMLNode& jsdl);
  /** Cancel processing/execution of job */
  bool Cancel(void);
  /** Resume execution of job after error */
  bool Resume(void);
  /** Returns current state of job */
  std::string State(void);
};
















