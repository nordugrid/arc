// -*- indent-tabs-mode: nil -*-
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/data/DataStatus.h>
#include <arc/IString.h>

namespace Arc {

  static const char *status_string[] = {
    istring("Operation completed successfully"),              // Success
    istring("Source is invalid URL"),                         // ReadAcquireError
    istring("Destination is invalid URL"),                    // WriteAcquireError
    istring("Resolving of index service for source failed"),  // ReadResolveError
    istring("Resolving of index service for destination failed"), // WriteResolveError
    istring("Can't read from source"),                        // ReadStartError
    istring("Can't write to destination"),                    // WriteStartError
    istring("Failed while reading from source"),              // ReadError
    istring("Failed while writing to destination"),           // WriteError
    istring("Failed while transferring data (mostly timeout)"), // TransferError
    istring("Failed while finishing reading from source"),    // ReadStopError
    istring("Failed while finishing writing to destination"), // WriteStopError
    istring("First stage of registration to index service failed"), // PreRegisterError
    istring("Last stage of registration to index service failed"), // PostRegisterError
    istring("Unregistering from index service failed"),       // UnregisterError
    istring("Error in caching procedure"),                    // CacheError
    istring("Error due to expiration of provided credentials"), // CredentialsExpiredError
    istring("Delete error"),                                  // DeleteError
    istring("No valid location available"),                   // NoLocationError
    istring("Location already exists"),                       // LocationAlreadyExistsError
    istring("Operation not supported for this kind of URL"),  // NotSupportedForDirectDataPointsError
    istring("Feature is not implemented"),                    // UnimplementedError
    istring("Already reading from source"),                   // IsReadingError
    istring("Already writing to destination"),                // IsWritingError
    istring("Read access check failed"),                      // CheckError
    istring("Directory listing failed"),                      // ListError
    istring("Object is not suitable for listing"),            // ListNonDirError
    istring("Failed to obtain information about file"),       // StatError
    istring("No such file or directory"),                     // StatNotPresentError
    istring("Object not initialized (internal error)"),       // NotInitializedError
    istring("Operating System error"),                        // SystemError
    istring("Failed to stage file(s)"),                       // StageError
    istring("Inconsistent metadata"),                         // InconsistentMetadataError
    istring("Failed to prepare source"),                      // ReadPrepareError
    istring("Should wait for source to be prepared"),         // ReadPrepareWait
    istring("Failed to prepare destination"),                 // WritePrepareError
    istring("Should wait for destination to be prepared"),    // WritePrepareWait
    istring("Failed to finalize reading from source"),        // ReadFinishError
    istring("Failed to finalize writing to destination"),     // WriteFinishError
    istring("Failed to create directory"),                    // CreateDirectoryError
    istring("Failed to rename URL"),                          // RenameError
    istring("Data was already cached"),                       // SuccessCached
    istring("Operation cancelled successfully"),              // SuccessCancelled
    istring("Generic error"),                                 // GenericError
    istring("Unknown error")                                  // UnknownError
  };

  static const char* errnodesc[] = {
    istring("No error"),                         // DataStatusErrnoBase
    istring("Transfer timed out"),               // EARCTRANSFERTIMEOUT
    istring("Checksum mismatch"),                // EARCCHECKSUM
    istring("Bad logic"),                        // EARCLOGIC
    istring("All results obtained are invalid"), // EARCRESINVAL
    istring("Temporary service error"),          // EARCSVCTMP
    istring("Permanent service error"),          // EARCSVCPERM
    istring("Error switching uid"),              // EARCUIDSWITCH
    istring("Unknown error")                     // EARCOTHER
  };

  DataStatus::operator std::string() const {
    unsigned int status_ = status;
    if (status_ >= DataStatusRetryableBase) status_ -= DataStatusRetryableBase;
    if (status_ > UnknownError) status_ = UnknownError;
    std::string s(status_string[status_]);
    if (Errno > 0 && Errno < DataStatusErrnoMax) s += ": " + GetStrErrno();
    if (!desc.empty()) s += ": " + desc;
    return s;
  }

  std::string DataStatus::GetStrErrno() const {
    if (Errno > DataStatusErrnoMax) return "Unknown error";
    if (Errno > DataStatusErrnoBase) return errnodesc[Errno - DataStatusErrnoBase];
    return StrError(Errno);
  }

  bool DataStatus::Retryable() const {
    return (status > DataStatusRetryableBase
         || Errno == EAGAIN
         || Errno == EBUSY
         || Errno == ETIMEDOUT
         || Errno == EARCSVCTMP
         || Errno == EARCTRANSFERTIMEOUT
         || Errno == EARCCHECKSUM
         || Errno == EARCOTHER);
  }

} // namespace Arc
