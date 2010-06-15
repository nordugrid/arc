// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_GUID_H__
#define __ARC_GUID_H__

#include <string>

namespace Arc {

  /// Utilities for generating unique identifiers in the form 12345678-90ab-cdef-1234-567890abcdef

  /// Generates a unique identifier using information such as IP address, current time etc.
  void GUID(std::string& guid);
  /// Generates a unique identifier using the system uuid libraries
  std::string UUID(void);

} // namespace Arc

#endif // __ARC_GUID_H__
