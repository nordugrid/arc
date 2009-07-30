// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_DATASTATUS_H__
#define __ARC_DATASTATUS_H__

#include <iostream>
#include <string>

namespace Arc {
  
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
      ReadAcquireErrorRetryable = 101,

      /// Destination is bad URL or can't be used due to some reason
      WriteAcquireError = 2,
      WriteAcquireErrorRetryable = 102,

      /// Resolving of index service URL for source failed
      ReadResolveError = 3,
      ReadResolveErrorRetryable = 103,

      /// Resolving of index service URL for destination failed
      WriteResolveError = 4,
      WriteResolveErrorRetryable = 104,

      /// Can't read from source
      ReadStartError = 5,
      ReadStartErrorRetryable = 105,

      /// Can't write to destination
      WriteStartError = 6,
      WriteStartErrorRetryable = 106,

      /// Failed while reading from source
      ReadError = 7,
      ReadErrorRetryable = 107,

      /// Failed while writing to destination
      WriteError = 8,
      WriteErrorRetryable = 108,

      /// Failed while transfering data (mostly timeout)
      TransferError = 9,
      TransferErrorRetryable = 109,

      /// Failed while finishing reading from source
      ReadStopError = 10,
      ReadStopErrorRetryable = 110,

      /// Failed while finishing writing to destination
      WriteStopError = 11,
      WriteStopErrorRetryable = 111,

      /// First stage of registration of index service URL failed
      PreRegisterError = 12,
      PreRegisterErrorRetryable = 112,

      /// Last stage of registration of index service URL failed
      PostRegisterError = 13,
      PostRegisterErrorRetryable = 113,

      /// Unregistration of index service URL failed
      UnregisterError = 14,
      UnregisterErrorRetryable = 114,

      /// Error in caching procedure
      CacheError = 15,
      CacheErrorRetryable = 115,

      /// Error due to provided credentials are expired
      CredentialsExpiredError = 16,

      /// Error deleting location or URL
      DeleteError = 17,
      DeleteErrorRetryable = 117,

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
      CheckErrorRetryable = 124,

      /// File listing failed
      ListError = 25,
      ListErrorRetryable = 125,

      /// Object initialization failed
      NotInitializedError = 26,

      /// Error in OS
      SystemError = 27,
    
      /// Staging error
      StageError = 28,
      StageErrorRetryable = 128,
 
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
