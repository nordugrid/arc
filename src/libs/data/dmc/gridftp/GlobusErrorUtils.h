#ifndef __ARC_GLOBUSERRORUTILS_H__
#define __ARC_GLOBUSERRORUTILS_H__

#include <iostream>
#include <string>
#include <globus_common.h>

namespace Arc {

std::ostream& operator<<(std::ostream& o,globus_object_t* err);
std::string globus_object_to_string(globus_object_t* err);

class GlobusResult {
 private:
  globus_result_t r;
 public:
  GlobusResult(void):r(0) { };
  GlobusResult(globus_result_t result):r(result) { };
  GlobusResult& operator=(globus_result_t result) { r=result; return *this; };
  bool operator==(GlobusResult result) { return (r==result.r); };
  bool operator!=(GlobusResult result) { return (r!=result.r); };
  operator bool(void) { return (r==GLOBUS_SUCCESS); };
  bool operator!(){ return (r!=GLOBUS_SUCCESS); };
  operator globus_result_t(void) { return r; };
  std::string str();
};

std::ostream& operator<<(std::ostream& o,GlobusResult res);

} // namespace Arc

#endif // __ARC_GLOBUSERRORUTILS_H__
