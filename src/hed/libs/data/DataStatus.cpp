#include <arc/data/DataStatus.h>
#include <arc/IString.h>

namespace Arc {

  static const char *status_string[] = {
    istring("Operation completed successfully"),
    istring("Source is bad URL or can't be used due to some reason"),
    istring("Destination is bad URL or can't be used due to some reason"),
    istring("Resolving of index service URL for source failed"),
    istring("Resolving of index service URL for destination failed"),
    istring("Can't read from source"),
    istring("Can't write to destination"),
    istring("Failed while reading from source"),
    istring("Failed while writing to destination"),
    istring("Failed while transfering data (mostly timeout)"),
    istring("Failed while finishing reading from source"),
    istring("Failed while finishing writing to destination"),
    istring("First stage of registration of index service URL failed"),
    istring("Last stage of registration of index service URL failed"),
    istring("Unregistration of index service URL failed"),
    istring("Error in caching procedure"),
    istring("Error in caching procedure (retryable)"),
    istring("Error due to provided credentials are expired"),
    istring("Error deleting location or URL"),
    istring("No valid location available"),
    istring("Location already exists"),
    istring("Operation has no sense for this kind of URL"),
    istring("Feature is not implemented"),
    istring("DataPoint is already reading"),
    istring("DataPoint is already writing"),
    istring("Access check failed"),
    istring("File listing failed"),
    istring("Object not initialized (internal error)"),
    istring("Unknown error")
  };

  DataStatus::operator std::string() const {
    return status_string[status];
  }

} // namespace Arc
