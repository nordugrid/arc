#ifndef __ARC_GM_JOB_DESC_H__
#define __ARC_GM_JOB_DESC_H__
#include <string>
#include <iostream>

#include <arc/client/JobDescription.h>

#include "users.h"
#include "job.h"

typedef enum {
  JobReqSuccess,
  JobReqInternalFailure,
  JobReqSyntaxFailure,
  JobReqMissingFailure,
  JobReqUnsupportedFailure,
  JobReqLogicalFailure
} JobReqResult;

/**
 * Read and parse job description from file and update the job description
 * reference.
 *
 * @param fname filename of the job description file.
 * @param desc a reference to a Arc::JobDescription which is filled on success,
 *   if the job description format is unknown the reference is not touched.
 * @return false if job description could not be read or parsed, true on success.
 */
bool get_arc_job_description(const std::string& fname, Arc::JobDescription& desc);
bool write_grami(const Arc::JobDescription& arc_job_desc, const JobDescription& job_desc, const JobUser& user, const char* opt_add);
JobReqResult get_acl(const Arc::JobDescription& arc_job_desc, std::string& acl);
bool check(const Arc::JobDescription& arc_job_desc);
bool set_execs(const Arc::JobDescription& desc, const std::string& session_dir);

class value_for_shell {
 friend std::ostream& operator<<(std::ostream&,const value_for_shell&);
 private:
  const char* str;
  bool quote;
 public:
  value_for_shell(const char *str_,bool quote_):str(str_),quote(quote_) { };
  value_for_shell(const std::string &str_,bool quote_):str(str_.c_str()),quote(quote_) { };
};
std::ostream& operator<<(std::ostream &o,const value_for_shell &s);

class numvalue_for_shell {
 friend std::ostream& operator<<(std::ostream&,const numvalue_for_shell&);
 private:
  long int n;
 public:
  numvalue_for_shell(const char *str) { n=0; sscanf(str,"%li",&n); };
  numvalue_for_shell(int n_) { n=n_; };
  numvalue_for_shell operator/(int d) { return numvalue_for_shell(n/d); };
  numvalue_for_shell operator*(int d) { return numvalue_for_shell(n*d); };
};
std::ostream& operator<<(std::ostream &o,const numvalue_for_shell &s);

#define NG_RSL_DEFAULT_STDIN      const_cast<char*>("/dev/null")
#define NG_RSL_DEFAULT_STDOUT     const_cast<char*>("/dev/null")
#define NG_RSL_DEFAULT_STDERR     const_cast<char*>("/dev/null")

#endif
