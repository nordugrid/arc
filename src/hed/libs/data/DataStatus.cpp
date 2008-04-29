#include "DataStatus.h"
#include <arc/StringConv.h>

namespace Arc {

  static const char *status_string[] = {
      "Operation completed successfully",
      "Source is bad URL or can't be used due to some reason",
      "Destination is bad URL or can't be used due to some reason",
      "Resolving of index service URL for source failed",
      "Resolving of index service URL for destination failed",
      "Can't read from source",
      "Can't write to destination",
      "Failed while reading from source",
      "Failed while writing to destination",
      "Failed while transfering data (mostly timeout)",
      "Failed while finishing reading from source",
      "Failed while finishing writing to destination",
      "First stage of registration of index service URL failed",
      "Last stage of registration of index service URL failed",
      "Unregistration of index service URL failed",
      "Error in caching procedure",
      "Error due to provided credentials are expired",
      "Error deleting location or URL",
      "No valid location available",
      "Location already exists",
      "Operation has no sense for this kind of URL",
      "Feature is not implemented",
      "DataPoint is already reading",
      "DataPoint is already writing",
      "Access check failed",
      "File listing failed",
      "Unknown error"
  };

  DataStatus::operator std::string(void) const {
    if(status >= UnknownError) {
      return status_string[UnknownError] + (" " + Arc::tostring(status));
    };
    return status_string[status];
  }

} // namespace Arc

