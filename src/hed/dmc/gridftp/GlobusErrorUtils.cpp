#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "GlobusErrorUtils.h"

namespace Arc {

  std::ostream& operator<<(std::ostream& o, const GlobusResult& res) {
    if(res)
      return (o << "<success>");
    globus_object_t *err = globus_error_get(res);
    for(globus_object_t *err_ = err; err_;
        err_ = globus_error_base_get_cause(err_)) {
      if(err_ != err)
        o << "/";
      char *tmp = globus_object_printable_to_string(err_);
      if(tmp) {
        o << tmp;
        free(tmp);
      }
      else
        o << "unknown error";
    }
    if(err)
      globus_object_free(err);
    return o;
  }

  std::string GlobusResult::str() const {
    if(r == GLOBUS_SUCCESS)
      return "<success>";
    globus_object_t *err = globus_error_get(r);
    std::string s;
    for(globus_object_t *err_ = err; err_;
        err_ = globus_error_base_get_cause(err_)) {
      if(err_ != err)
        s += "/";
      char *tmp = globus_object_printable_to_string(err_);
      if(tmp) {
        s += tmp;
        free(tmp);
      }
      else
        s += "unknown error";
    }
    if(err)
      globus_object_free(err);
    return s;
  }

  std::ostream& operator<<(std::ostream& o, globus_object_t *err) {
    if(err == GLOBUS_NULL)
      return (o << "<success>");
    for(globus_object_t *err_ = err; err_;
        err_ = globus_error_base_get_cause(err_)) {
      if(err_ != err)
        o << "/";
      char *tmp = globus_object_printable_to_string(err_);
      if(tmp) {
        o << tmp;
        free(tmp);
      }
      else
        o << "unknown error";
    }
    return o;
  }

  std::string globus_object_to_string(globus_object_t *err) {
    if(err == GLOBUS_NULL)
      return "<success>";
    std::string s;
    for(globus_object_t *err_ = err; err_;
        err_ = globus_error_base_get_cause(err_)) {
      if(err_ != err)
        s += "/";
      char *tmp = globus_object_printable_to_string(err_);
      if(tmp) {
        s += tmp;
        free(tmp);
      }
      else
        s += "unknown error";
    }
    return s;
  }

} // namespace Arc
