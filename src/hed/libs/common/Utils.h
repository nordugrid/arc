#ifndef __ARC_UTILS_H__
#define __ARC_UTILS_H__

#include <cstdlib>
// NOTE: On Solaris errno is not working properly if cerrno is included first
#include <cerrno>
#include <string>

namespace Arc {

  std::string GetEnv(const std::string& var);

  void SetEnv(const std::string& var, const std::string& value);

  std::string StrError(int errnum = errno);

} // namespace Arc

# endif // __ARC_UTILS_H__
