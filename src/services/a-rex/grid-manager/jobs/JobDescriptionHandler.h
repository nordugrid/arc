#ifndef __ARC_GM_JOB_REQUEST_H__
#define __ARC_GM_JOB_REQUEST_H__

#include <string>
#include <arc/compute/JobDescription.h>

#include "GMJob.h"

namespace ARex {

/// Return code of parsing operation
enum JobReqResultType {
  JobReqSuccess,
  JobReqInternalFailure,
  JobReqSyntaxFailure,
  JobReqMissingFailure,
  JobReqUnsupportedFailure,
  JobReqLogicalFailure
};

/// Return value of parsing operation
class JobReqResult {
public:
  JobReqResultType result_type;
  std::string acl;
  std::string failure;
  JobReqResult(JobReqResultType type, const std::string& acl="", const std::string& failure="")
    :result_type(type), acl(acl), failure(failure) {}
  bool operator==(const JobReqResultType& result) const { return result == result_type; }
  bool operator!=(const JobReqResultType& result) const { return result != result_type; }
};

/// Deals with parsing and converting job descriptions between Arc::JobDescription
/// and JobLocalDescription. Also deals with reading and writing .grami file.
class JobDescriptionHandler {

public:
  /// Create a new job description handler
  JobDescriptionHandler(const GMConfig& config): config(config) {}
  /// Parse the job description at the given file into job_desc and
  /// arc_job_desc. Optionally check acl file and put result into
  /// returned object
  JobReqResult parse_job_req_from_file(JobLocalDescription &job_desc,Arc::JobDescription& arc_job_desc,const std::string &fname,bool check_acl=false) const;
  /// Parse the job description from the given string into job_desc and
  /// arc_job_desc. Optionally check acl file and put result into
  /// returned object
  JobReqResult parse_job_req_from_mem(JobLocalDescription &job_desc,Arc::JobDescription& arc_job_desc,const std::string &desc_str,bool check_acl=false) const;
  /// Parse the job description for job_id into job_desc. Optionally check
  /// acl file and put result into returned object
  JobReqResult parse_job_req(const JobId &job_id,JobLocalDescription &job_desc,bool check_acl=false) const;
  /// Parse the job description for job_id into job_desc and arc_job_desc.
  /// Optionally check acl file and put result into returned object
  JobReqResult parse_job_req(const JobId &job_id,JobLocalDescription &job_desc,Arc::JobDescription& arc_job_desc,bool check_acl=false) const;
  /// Parse job description into job_desc and write .local, .input and .output files
  bool process_job_req(const GMJob &job,JobLocalDescription &job_desc) const;
  /// Write .grami file after parsing job description file
  bool write_grami(GMJob &job,const char *opt_add = NULL) const;
  /// Write .grami from information in arc_job_desc and job
  bool write_grami(const Arc::JobDescription& arc_job_desc, GMJob& job, const char* opt_add) const;
  /// Get the local LRMS job id corresponding to A-REX job_id
  std::string get_local_id(const JobId &job_id) const;
  /// Set executable bits on appropriate files for the given job
  bool set_execs(const GMJob &job) const;

private:
  JobReqResult parse_job_req_internal(JobLocalDescription &job_desc,Arc::JobDescription const& arc_job_desc,bool check_acl=false) const;

  /// Read and parse job description from file and update the job description reference.
  /** @param fname filename of the job description file.
   * @param desc a reference to a Arc::JobDescription which is filled on success,
   *   if the job description format is unknown the reference is not touched.
   * @return false if job description could not be read or parsed, true on success.
   */
  Arc::JobDescriptionResult get_arc_job_description(const std::string& fname, Arc::JobDescription& desc) const;
  /// Read ACLs from .acl file
  JobReqResult get_acl(const Arc::JobDescription& arc_job_desc) const;
  /// Write info to .grami for job executable
  bool write_grami_executable(std::ofstream& f, const std::string& name, const Arc::ExecutableType& exec) const;

  /// Class for handling escapes and quotes when writing to .grami
  class value_for_shell {
    friend std::ostream& operator<<(std::ostream&,const value_for_shell&);
   private:
    const char* str;
    bool quote;
   public:
    value_for_shell(const char *str_,bool quote_):str(str_),quote(quote_) { };
    value_for_shell(const std::string &str_,bool quote_):str(str_.c_str()),quote(quote_) { };
  };
  friend std::ostream& operator<<(std::ostream&,const value_for_shell&);

  const GMConfig& config;
  static Arc::Logger logger;
  static const std::string NG_RSL_DEFAULT_STDIN;
  static const std::string NG_RSL_DEFAULT_STDOUT;
  static const std::string NG_RSL_DEFAULT_STDERR;
};

std::ostream& operator<<(std::ostream&,const JobDescriptionHandler::value_for_shell&);

} // namespace ARex

#endif
