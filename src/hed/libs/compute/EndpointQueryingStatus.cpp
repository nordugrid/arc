// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "EndpointQueryingStatus.h"

namespace Arc {

  std::string EndpointQueryingStatus::str(EndpointQueryingStatusType s) {
    if      (s == UNKNOWN)     return "UNKNOWN";
    else if (s == STARTED)     return "STARTED";
    else if (s == FAILED)      return "FAILED";
    else if (s == NOPLUGIN)    return "NOPLUGIN";
    else if (s == SUCCESSFUL)  return "SUCCESSFUL";
    else                       return ""; // There should be no other alternative!
  }

} // namespace Arc
