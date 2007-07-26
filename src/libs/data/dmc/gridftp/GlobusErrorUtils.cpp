#include "GlobusErrorUtils.h"

namespace Arc {

std::ostream& operator<<(std::ostream& o,GlobusResult res) {
  if(res == GlobusResult(GLOBUS_SUCCESS)) {
    o<<"<success>"; return o;
  };
  globus_object_t* err_top = globus_error_get(res);
  for(globus_object_t* err_=err_top;err_;) {
    char* tmp=globus_object_printable_to_string(err_);
    if(tmp) { if(err_ != err_top) o<<"/"; o<<tmp; free(tmp); };
    err_=globus_error_base_get_cause(err_);
  };
  if(err_top) globus_object_free(err_top);
  return o;
}

std::string GlobusResult::str() {
  if(r == GLOBUS_SUCCESS) { return "<success>"; };
  globus_object_t* err_top = globus_error_get(r);
  std::string s;
  for(globus_object_t* err_=err_top;err_;) {
    char* tmp=globus_object_printable_to_string(err_);
    if(tmp) { if(err_ != err_top) s+="/"; s+=tmp; free(tmp); };
    err_=globus_error_base_get_cause(err_);
  };
  if(err_top) globus_object_free(err_top);
  if(s.empty()) return "unknown error";
  return s;
}

std::ostream& operator<<(std::ostream& o,globus_object_t* err) {
  if(err == GLOBUS_NULL) {
    o<<"<success>"; return o;
  };
  for(globus_object_t* err_=err;err_;) {
    char* tmp=globus_object_printable_to_string(err_);
    if(tmp) { if(err_ != err) o<<"/"; o<<tmp; free(tmp); };
    err_=globus_error_base_get_cause(err_);
  };
  return o;
}

std::string globus_object_to_string(globus_object_t* err) {
  if(err == GLOBUS_NULL) { return "<success>"; };
  std::string s;
  for(globus_object_t* err_=err;err_;) {
    char* tmp=globus_object_printable_to_string(err_);
    if(tmp) { if(err_ != err) s+="/"; s+=tmp; free(tmp); };
    err_=globus_error_base_get_cause(err_);
  };
  if(s.empty()) return "unknown error";
  return s;
}

} // namespace Arc
