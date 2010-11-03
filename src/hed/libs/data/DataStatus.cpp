// -*- indent-tabs-mode: nil -*-

#include <arc/data/DataStatus.h>
#include <arc/IString.h>

namespace Arc {

  static const char *status_string[] = {
    istring("Operation completed successfully"), // 0
    istring("Source is bad URL or can't be used due to some reason"), // 1
    istring("Destination is bad URL or can't be used due to some reason"), // 2
    istring("Resolving of index service URL for source failed"), // 3
    istring("Resolving of index service URL for destination failed"), // 4
    istring("Can't read from source"), // 5
    istring("Can't write to destination"), // 6
    istring("Failed while reading from source"), // 7
    istring("Failed while writing to destination"), // 8
    istring("Failed while transfering data (mostly timeout)"), // 9
    istring("Failed while finishing reading from source"), // 10
    istring("Failed while finishing writing to destination"), // 11
    istring("First stage of registration of index service URL failed"), // 12
    istring("Last stage of registration of index service URL failed"), // 13
    istring("Unregistration of index service URL failed"), // 14
    istring("Error in caching procedure"), // 15
    istring("Error due to provided credentials are expired"), // 16
    istring("Error deleting location or URL"), // 17
    istring("No valid location available"), // 18
    istring("Location already exists"), // 19
    istring("Operation has no sense for this kind of URL"), // 20
    istring("Feature is not implemented"), // 21
    istring("DataPoint is already reading"), // 22
    istring("DataPoint is already writing"), // 23
    istring("Read access check failed"), // 24
    istring("Directory listing failed"), // 25
    istring("Object is not suitable for listing"), // 26
    istring("File stating failed"), // 27
    istring("Object not initialized (internal error)"), // 28
    istring("System error"), // 29
    istring("Failed to stage file(s)"), // 30
    istring("Inconsistent metadata"), // 31
    istring("Data was already cached"), // 32
    istring("Unknown error") // 33
  };

  DataStatus::operator std::string() const {
    unsigned int status_ = status;
    if (status_ >= DataStatusRetryableBase) status_-=DataStatusRetryableBase;
    if (status_ > UnknownError) status_=UnknownError;
    return status_string[status_];
  }

} // namespace Arc
