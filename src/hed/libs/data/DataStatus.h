// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_DATASTATUS_H__
#define __ARC_DATASTATUS_H__

#include <iostream>
#include <string>

namespace Arc {
  
#define DataStatusRetryableBase (100)
  /**
   * A class to be used for return types of all major data handling
   * methods. It describes the outcome of the method.
   */
  class DataStatus {

  public:

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
      ListErrorRetryable = DataStatusRetryableBase+ListError,

      /// Object initialization failed
      NotInitializedError = 26,

      /// Error in OS
      SystemError = 27,
    
      /// Staging error
      StageError = 28,
      StageErrorRetryable = DataStatusRetryableBase+StageError,
 
      /// Undefined
      UnknownError = 29
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

    bool operator!() {
      return status != Success;
    }
    operator bool() {
      return status == Success;
    }

    bool Passed(void) {
      return (status == Success) || (status == NotSupportedForDirectDataPointsError);
    }
  
    bool Retryable() {
      return status > 100;
    }
  
    void SetDesc(std::string d) {
      desc = d;
    }
    
    std::string GetDesc() {
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
