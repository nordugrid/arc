#ifndef __ARC_GLOBUSERRORUTILS_H__
#define __ARC_GLOBUSERRORUTILS_H__

#include <iostream>
#include <string>

#include <globus_common.h>

namespace Arc {

  std::string globus_object_to_string(globus_object_t *err);
  std::ostream& operator<<(std::ostream& o, globus_object_t *err);

  class GlobusResult {
  public:
    GlobusResult()
      : r(GLOBUS_SUCCESS) {}
    GlobusResult(const globus_result_t result)
      : r(result) {}
    GlobusResult& operator=(const globus_result_t result) {
      r = result;
      return *this;
    }
    bool operator==(const GlobusResult& result) const {
      return (r == result.r);
    }
    bool operator!=(const GlobusResult& result) const {
      return (r != result.r);
    }
    operator bool() const {
      return (r == GLOBUS_SUCCESS);
    }
    bool operator!() const {
      return (r != GLOBUS_SUCCESS);
    }
    operator globus_result_t() const {
      return r;
    }
    std::string str() const;
  private:
    globus_result_t r;
  };

  std::ostream& operator<<(std::ostream& o, const GlobusResult& res);

} // namespace Arc

#endif // __ARC_GLOBUSERRORUTILS_H__
