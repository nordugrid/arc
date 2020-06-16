#ifndef __ARC_GLOBUSERRORUTILS_H__
#define __ARC_GLOBUSERRORUTILS_H__

#include <iostream>
#include <string>

#include <globus_common.h>

namespace Arc {

  std::string globus_object_to_string(globus_object_t *err);
  /// Parse error message, set errorno if possible and return it.
  int globus_error_to_errno(const std::string& msg, int errorno);
  std::ostream& operator<<(std::ostream& o, globus_object_t *err);

  class GlobusResult {
  public:
    GlobusResult();
    ~GlobusResult();
    explicit GlobusResult(const globus_result_t result);
    GlobusResult& operator=(const globus_result_t result);
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
    operator globus_object_t*() const {
      return o;
    }
    std::string str() const;
    static void wipe();
  private:
    globus_result_t r;
    globus_object_t* o;
    GlobusResult(const GlobusResult&);
    GlobusResult& operator=(const GlobusResult&);
  };

  std::ostream& operator<<(std::ostream& o, const GlobusResult& res);

} // namespace Arc

#endif // __ARC_GLOBUSERRORUTILS_H__
