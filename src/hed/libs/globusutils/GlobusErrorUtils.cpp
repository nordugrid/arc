#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/StringConv.h>

#include "GlobusErrorUtils.h"

namespace Arc {

static Logger logger(Logger::getRootLogger(), "GLOBUS");

  // When Globus code fails it allocates error object, assigns it to cache
  // and returns cache index as error code. If object is not retrived
  // from cache it is effectiviely leaked. So each error code must be passed
  // through though globus_error_get() (or even better GlobusResult class
  // implemented here). Unfortunately Globus code itself does not always 
  // follow this pattern and hence is effectively leaking memory. 
  // Here we have adjust_last_processed() trying to pick up such lost 
  // error object and free them.

  static Glib::StaticMutex last_processed_lock = GLIBMM_STATIC_MUTEX_INIT;
  static globus_result_t last_processed = GLOBUS_SUCCESS+1;
  static const long int safety_gap = 10; // leave 10 last errors in cache in case something still needs them
  static const long int processing_limit = 100; // process no more than 100 lost error object at once

  static void adjust_last_processed(globus_result_t new_result) {
    last_processed_lock.lock();
    long int diff = (long int)(new_result-last_processed);
    if(diff > processing_limit) diff = processing_limit;
    while(diff > safety_gap) {
      globus_object_t* o = globus_error_get(last_processed);
      if((o != NULL) && (o != GLOBUS_ERROR_NO_INFO)) {
        globus_object_free(o);
      }
      ++last_processed;
      --diff;
    }
    last_processed_lock.unlock();
  }

  void GlobusResult::wipe() {
    GlobusResult(globus_error_put(globus_error_construct_error(GLOBUS_COMMON_MODULE, GLOBUS_NULL, 0, "", "", 0, "")));
  }

  GlobusResult::GlobusResult() : r(GLOBUS_SUCCESS), o(NULL) {
  }

  GlobusResult::GlobusResult(const globus_result_t result) : r(result), o(NULL) {
    if((r != GLOBUS_SUCCESS) && (r != GLOBUS_FAILURE)) {
      o = globus_error_get(r);
      if(o == GLOBUS_ERROR_NO_INFO) o = NULL;
      adjust_last_processed(r);
    }
  }
  
  GlobusResult::~GlobusResult() {
    if(o)
      globus_object_free(o);
  }
 
  GlobusResult& GlobusResult::operator=(const globus_result_t result) {
    if(o)
      globus_object_free(o);
    o = NULL;
    r = result;
    if((r != GLOBUS_SUCCESS) && (r != GLOBUS_FAILURE)) {
      o = globus_error_get(r);
      if(o == GLOBUS_ERROR_NO_INFO) o = NULL;
      adjust_last_processed(r);
    }
    return *this;
  }

  std::string GlobusResult::str() const {
    if (r == GLOBUS_SUCCESS)
      return "<success>";
    std::string s;
    for (globus_object_t *err_ = o; err_;
	 err_ = globus_error_base_get_cause(err_)) {
      if (err_ != o)
	s += "/";
      char *tmp = globus_object_printable_to_string(err_);
      if (tmp) {
	s += tmp;
	free(tmp);
      }
      else
	s += "unknown error";
    }
    return trim(s);
  }

  std::ostream& operator<<(std::ostream& o, const GlobusResult& res) {
    if (res)
      return (o << "<success>");
    globus_object_t *err = static_cast<globus_object_t*>(res);
    for (globus_object_t *err_ = err; err_;
	 err_ = globus_error_base_get_cause(err_)) {
      if (err_ != err)
	o << "/";
      char *tmp = globus_object_printable_to_string(err_);
      if (tmp) {
	o << tmp;
	free(tmp);
      }
      else
	o << "unknown error";
    }
    return o;
  }

  std::ostream& operator<<(std::ostream& o, globus_object_t *err) {
    if (err == GLOBUS_NULL)
      return (o << "<success>");
    for (globus_object_t *err_ = err; err_;
	 err_ = globus_error_base_get_cause(err_)) {
      if (err_ != err)
	o << "/";
      char *tmp = globus_object_printable_to_string(err_);
      if (tmp) {
	o << tmp;
	free(tmp);
      }
      else
	o << "unknown error";
    }
    return o;
  }

  std::string globus_object_to_string(globus_object_t *err) {
    if (err == GLOBUS_NULL)
      return "<success>";
    std::string s;
    for (globus_object_t *err_ = err; err_;
	 err_ = globus_error_base_get_cause(err_)) {
      if (err_ != err)
	s += "/";
      char *tmp = globus_object_printable_to_string(err_);
      if (tmp) {
	s += tmp;
	free(tmp);
      }
      else
	s += "unknown error";
    }
    return s;
  }

  int globus_error_to_errno(const std::string& msg, int errorno) {
    // parse the message and try to detect certain errors. If none found leave
    // errorno unchanged. There is no guarantee that Globus won't change error
    // messages but there is no other way to determine the reason for errors.
    if (lower(msg).find("no such file")         != std::string::npos) return ENOENT;
    if (lower(msg).find("object unavailable")   != std::string::npos) return ENOENT;
    if (lower(msg).find("object not available") != std::string::npos) return ENOENT;
    if (lower(msg).find("no such job")          != std::string::npos) return ENOENT;
    if (lower(msg).find("file unavailable")     != std::string::npos) return ENOENT;
    if (lower(msg).find("file exists")          != std::string::npos) return EEXIST;
    if (lower(msg).find("file not allowed")     != std::string::npos) return EACCES;
    if (lower(msg).find("permission denied")    != std::string::npos) return EACCES;
    if (lower(msg).find("failed authenticating")!= std::string::npos) return EACCES;
    if (lower(msg).find("can't make")           != std::string::npos) return EACCES;
    if (lower(msg).find("directory not empty")  != std::string::npos) return ENOTEMPTY;
    if (lower(msg).find("do not understand")    != std::string::npos) return EOPNOTSUPP;

    return errorno;
  }

} // namespace Arc
