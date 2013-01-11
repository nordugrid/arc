#include <posix/gfal_posix_api.h>

#include "GFALUtils.h"

namespace ArcDMCGFAL {

  using namespace Arc;

  std::string GFALUtils::GFALURL(const URL& u) {
    // LFC URLs must be converted to lfn:/path or guid:abcd...
    std::string gfalurl;
    if (u.Protocol() != "lfc") gfalurl = u.plainstr();
    else if (u.MetaDataOption("guid").empty()) gfalurl = "lfn:" + u.Path();
    else gfalurl = "guid:" + u.MetaDataOption("guid");
    return gfalurl;
  }

  //  return error number
  int GFALUtils::HandleGFALError(Logger& logger) {
    // Set errno before error is cleared from gfal
    int error_no = gfal_posix_code_error();
    char errbuf[2048];
    gfal_posix_strerror_r(errbuf, sizeof(errbuf));
    logger.msg(ERROR, errbuf);
    gfal_posix_clear_error();
    return error_no;
  }

} // namespace ArcDMCGFAL
