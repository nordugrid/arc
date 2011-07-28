// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_DATASTATUS_H__
#define __ARC_DATASTATUS_H__

#include <iostream>
#include <string>

#include <arc/StringConv.h>

namespace Arc {
  
#define DataStatusRetryableBase (100)

  /// Status code returned by many DataPoint methods.
  /**
   * A class to be used for return types of all major data handling
   * methods. It describes the outcome of the method.
   */
  class DataStatus {

  public:

    /// Status codes
    enum DataStatusType {

      /// Operation completed successfully
      Success = 0,

      /// Source is bad URL or can't be used due to some reason
      ReadAcquireError = 1,
      ReadAcquireErrorRetryable = DataStatusRetryableBase+ReadAcquireError,

      /// Destination is bad URL or can't be used due to some reason
      WriteAcquireError = 2,
      WriteAcquireErrorRetryable = DataStatusRetryableBase+WriteAcquireError,

      /// Resolving of index service URL for source failed
      ReadResolveError = 3,
      ReadResolveErrorRetryable = DataStatusRetryableBase+ReadResolveError,

      /// Resolving of index service URL for destination failed
      WriteResolveError = 4,
      WriteResolveErrorRetryable = DataStatusRetryableBase+WriteResolveError,

      /// Can't read from source
      ReadStartError = 5,
      ReadStartErrorRetryable = DataStatusRetryableBase+ReadStartError,

      /// Can't write to destination
      WriteStartError = 6,
      WriteStartErrorRetryable = DataStatusRetryableBase+WriteStartError,

      /// Failed while reading from source
      ReadError = 7,
      ReadErrorRetryable = DataStatusRetryableBase+ReadError,

      /// Failed while writing to destination
      WriteError = 8,
      WriteErrorRetryable = DataStatusRetryableBase+WriteError,

      /// Failed while transfering data (mostly timeout)
      TransferError = 9,
      TransferErrorRetryable = DataStatusRetryableBase+TransferError,

      /// Failed while finishing reading from source
      ReadStopError = 10,
      ReadStopErrorRetryable = DataStatusRetryableBase+ReadStopError,

      /// Failed while finishing writing to destination
      WriteStopError = 11,
      WriteStopErrorRetryable = DataStatusRetryableBase+WriteStopError,

      /// First stage of registration of index service URL failed
      PreRegisterError = 12,
      PreRegisterErrorRetryable = DataStatusRetryableBase+PreRegisterError,

      /// Last stage of registration of index service URL failed
      PostRegisterError = 13,
      PostRegisterErrorRetryable = DataStatusRetryableBase+PostRegisterError,

      /// Unregistration of index service URL failed
      UnregisterError = 14,
      UnregisterErrorRetryable = DataStatusRetryableBase+UnregisterError,

      /// Error in caching procedure
      CacheError = 15,
      CacheErrorRetryable = DataStatusRetryableBase+CacheError,

      /// Error due to provided credentials are expired
      CredentialsExpiredError = 16,

      /// Error deleting location or URL
      DeleteError = 17,
      DeleteErrorRetryable = DataStatusRetryableBase+DeleteError,

      /// No valid location available
      NoLocationError = 18,

      /// No valid location available
      LocationAlreadyExistsError = 19,

      /// Operation has no sense for this kind of URL
      NotSupportedForDirectDataPointsError = 20,

      /// Feature is unimplemented
      UnimplementedError = 21,

      /// DataPoint is already reading
      IsReadingError = 22,

      /// DataPoint is already writing
      IsWritingError = 23,

      /// Access check failed
      CheckError = 24,
      CheckErrorRetryable = DataStatusRetryableBase+CheckError,

      /// File listing failed
      ListError = 25,
      ListNonDirError = 26,
      ListErrorRetryable = DataStatusRetryableBase+ListError,

      /// File/dir stating failed
      StatError = 27,
      StatNotPresentError = 28,
      StatErrorRetryable = DataStatusRetryableBase+StatError,

      /// Object initialization failed
      NotInitializedError = 29,

      /// Error in OS
      SystemError = 30,
    
      /// Staging error
      StageError = 31,
      StageErrorRetryable = DataStatusRetryableBase+StageError,
      
      /// Inconsistent metadata
      InconsistentMetadataError = 32,
 
      /// Can't prepare source
      ReadPrepareError = 32,
      ReadPrepareErrorRetryable = DataStatusRetryableBase+ReadPrepareError,

      /// Wait for source to be prepared
      ReadPrepareWait = 33,

      /// Can't prepare destination
      WritePrepareError = 34,
      WritePrepareErrorRetryable = DataStatusRetryableBase+WritePrepareError,

      /// Wait for destination to be prepared
      WritePrepareWait = 35,

      /// Can't finish source
      ReadFinishError = 36,
      ReadFinishErrorRetryable = DataStatusRetryableBase+ReadFinishError,

      /// Can't finish destination
      WriteFinishError = 37,
      WriteFinishErrorRetryable = DataStatusRetryableBase+WriteFinishError,

      /// Data was already cached
      SuccessCached = 38,
      
      /// General error which doesn't fit any other error
      GenericError = 39,
      GenericErrorRetryable = DataStatusRetryableBase+GenericError,

      /// Undefined
      UnknownError = 40
    };

    DataStatus(const DataStatusType& status, std::string desc="")
      : status(status), desc(desc) {}
    DataStatus()
      : status(Success), desc("") {}
    ~DataStatus() {}

    bool operator==(const DataStatusType& s) {
      return status == s;
    }
    bool operator==(const DataStatus& s) {
      return status == s.status;
    }
  
    bool operator!=(const DataStatusType& s) {
      return status != s;
    }
    bool operator!=(const DataStatus& s) {
      return status != s.status;
    }
  
    DataStatus operator=(const DataStatusType& s) {
      status = s;
      return *this;
    }

    bool operator!() const {
      return (status != Success) && (status != SuccessCached);
    }
    operator bool() const {
      return (status == Success) || (status == SuccessCached);
    }

    /// Returns true if no error occurred
    bool Passed() const {
      return ((status == Success) || (status == NotSupportedForDirectDataPointsError) ||
              (status == ReadPrepareWait) || (status == WritePrepareWait) ||
              (status == SuccessCached));
    }
  
    /// Returns true if the error was temporary and could be retried
    bool Retryable() const {
      return status > 100;
    }
  
    /// Set a text description of the status, removing trailing new line if present
    void SetDesc(const std::string& d) {
      desc = trim(d);
    }
    
    /// Get a text description of the status
    std::string GetDesc() const {
      return desc;
    }

    operator std::string(void) const;

  private:
  
    /// status code
    DataStatusType status;
    /// description of failure
    std::string desc;

  };

  inline std::ostream& operator<<(std::ostream& o, const DataStatus& d) {
    return (o << ((std::string)d));
  }
} // namespace Arc


#endif // __ARC_DATASTATUS_H__
