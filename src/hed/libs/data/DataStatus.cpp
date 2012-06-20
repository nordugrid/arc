// -*- indent-tabs-mode: nil -*-
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif


#include <arc/data/DataStatus.h>
#include <arc/IString.h>

namespace Arc {


  static const char *status_string[] = {
    istring("Operation completed successfully"), // 0
    istring("Source is invalid URL"), // 1
    istring("Destination is invalid URL"), // 2
    istring("Resolving of index service for source failed"), // 3
    istring("Resolving of index service for destination failed"), // 4
    istring("Can't read from source"), // 5
    istring("Can't write to destination"), // 6
    istring("Failed while reading from source"), // 7
    istring("Failed while writing to destination"), // 8
    istring("Failed while transferring data (mostly timeout)"), // 9
    istring("Failed while finishing reading from source"), // 10
    istring("Failed while finishing writing to destination"), // 11
    istring("First stage of registration to index service failed"), // 12
    istring("Last stage of registration to index service failed"), // 13
    istring("Unregistering from index service failed"), // 14
    istring("Error in caching procedure"), // 15
    istring("Error due to expiration of provided credentials"), // 16
    istring("Delete error"), // 17
    istring("No valid location available"), // 18
    istring("Location already exists"), // 19
    istring("Operation makes no sense for this kind of URL"), // 20
    istring("Feature is not implemented"), // 21
    istring("Already reading from source"), // 22
    istring("Already writing to destination"), // 23
    istring("Read access check failed"), // 24
    istring("Directory listing failed"), // 25
    istring("Object is not suitable for listing"), // 26
    istring("Failed to obtain information about file"), // 27
    istring("Object not initialized (internal error)"), // 28
    istring("Operating System error"), // 29
    istring("Failed to stage file(s)"), // 30
    istring("Inconsistent metadata"), // 31
    istring("Failed to prepare source"), // 32
    istring("Should wait for source to be prepared"), // 33
    istring("Failed to prepare destination"), // 34
    istring("Should wait for destination to be prepared"), // 35
    istring("Failed to finalize reading from source"), // 36
    istring("Failed to finalize writing to destination"), // 37
    istring("Failed to create directory"), // 38
    istring("Failed to rename URL"), // 39
    istring("Data was already cached"), // 40
    istring("Generic error"), // 41
    istring("Unknown error") // 42
  };

  DataStatus::operator std::string() const {
    unsigned int status_ = status;
    if (status_ >= DataStatusRetryableBase) status_-=DataStatusRetryableBase;
    if (status_ > UnknownError) status_=UnknownError;
    return status_string[status_];
  }

} // namespace Arc
