#ifndef __ARC_DATASTATUS__
#define __ARC_DATASTATUS__

#include <string>
#include <iostream>

namespace Arc {

  class DataStatus {

  public:

    enum DataStatusType {

      /// Operation completed successfully
      Success = 0,

      /// Source is bad URL or can't be used due to some reason
      ReadAcquireError = 1,

      /// Destination is bad URL or can't be used due to some reason
      WriteAcquireError = 2,

      /// Resolving of index service URL for source failed
      ReadResolveError = 3,

      /// Resolving of index service URL for destination failed
      WriteResolveError = 4,

      /// Can't read from source
      ReadStartError = 5,

      /// Can't write to destination
      WriteStartError = 6,

      /// Failed while reading from source
      ReadError = 7,

      /// Failed while writing to destination
      WriteError = 8,

      /// Failed while transfering data (mostly timeout)
      TransferError = 9,

      /// Failed while finishing reading from source
      ReadStopError = 10,

      /// Failed while finishing writing to destination
      WriteStopError = 11,

      /// First stage of registration of index service URL failed
      PreRegisterError = 12,

      /// Last stage of registration of index service URL failed
      PostRegisterError = 13,

      /// Unregistration of index service URL failed
      UnregisterError = 14,

      /// Error in caching procedure
      CacheError = 15,

      /// Error due to provided credentials are expired
      CredentialsExpiredError = 16,

      /// Error deleting location or URL
      DeleteError = 17,

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

      /// File listing failed
      ListError = 25,

      /// Object initialization failed
      NotInitializedError = 26,

      /// Undefined
      UnknownError = 27
    };

    DataStatus(const DataStatusType& status)
      : status(status) {}
    ~DataStatus() {}

    bool operator==(const DataStatusType& s) {
      return status == s;
    }
    bool operator==(const DataStatus& s) {
      return status == s.status;
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

    operator std::string(void) const;

  private:

    DataStatusType status;

  };


} // namespace Arc

inline std::ostream& operator<<(std::ostream& o, const Arc::DataStatus& d) {
  return (o << ((std::string)d));
}

#endif
