#ifndef GFALUTILS_H_
#define GFALUTILS_H_

#include <arc/Logger.h>
#include <arc/URL.h>

namespace ArcDMCGFAL {

  using namespace Arc;

  /// Utility functions for GFAL2
  class GFALUtils {
  public:
    /// Convert a URL into GFAL URL (using lfn: or guid: for LFC).
    static std::string GFALURL(const URL& u);
    /// Log GFAL message, clear internal GFAL error and return error number.
    static int HandleGFALError(Logger& logger);
  };

} // namespace ArcDMCGFAL


#endif /* GFALUTILS_H_ */
