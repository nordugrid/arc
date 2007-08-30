#ifndef __ARC_DATAPOINTDIRECT_H__
#define __ARC_DATAPOINTDIRECT_H__

#include <string>
#include <list>

#include "DataPoint.h"

#define MAX_PARALLEL_STREAMS 20
#define MAX_BLOCK_SIZE (1024 * 1024)

namespace Arc {

  class DataBufferPar;
  class DataCallback;

  /// This is kind of generalized file handle. 
  /** Differently from file handle it does not support operations 
    read() and write(). Instead it initiates operation and uses object
    of class DataBufferPar to pass actual data. It also provides
    other operations like querying parameters of remote object.
    It is used by higher-level classes DataMove and DataMovePar
    to provide data transfer service for application. */
  class DataPointDirect : public DataPoint {
   public:
    /// Reason of transfer failure
    typedef enum {
      common_failure = 0,
      credentials_expired_failure = 1
    } failure_reason_t;

   protected:
    virtual bool init_handle();
    virtual bool deinit_handle();

   public:
    /// Constructor
    /// \param url URL.
    DataPointDirect(const URL& url);
    /// Destructor. No comments.
    virtual ~DataPointDirect();

    virtual bool meta() const {
      return false;
    };

    /// Start reading data from URL.
    /// Separate thread to transfer data will be created. No other operation
    /// can be performed while 'reading' is in progress.
    /// \param buffer operation will use this buffer to put information into.
    /// Should not be destroyed before stop_reading was called and returned.
    /// Returns true on success.
    virtual bool start_reading(DataBufferPar& buffer);

    /// Start writing data to URL.
    /// Separate thread to transfer data will be created. No other operation
    /// can be performed while 'writing' is in progress.
    /// \param buffer operation will use this buffer to get information from.
    /// Should not be destroyed before stop_writing was called and returned.
    /// space_cb callback which is called if there is not enough to space
    /// storing data. Currently implemented only for file:/// URL.
    /// Returns true on success.
    virtual bool start_writing(DataBufferPar& buffer,
                               DataCallback *space_cb = NULL);

    /// Stop reading. It MUST be called after corressponding start_reading
    /// method. Either after whole data is transfered or to cancel transfer.
    /// Use 'buffer' object to find out when data is transfered.
    virtual bool stop_reading();

    /// Same as stop_reading but for corresponding start_writing.
    virtual bool stop_writing();

    /// Structure used in analyze() call.
    /// \param bufsize returns suggested size of buffers to store data.
    /// \param bufnum returns suggested number of buffers.
    /// \param cache returns true if url is allowed to be cached.
    /// \param local return true if URL is accessed locally (file://)
    class analyze_t {
     public:
      long int bufsize;
      int bufnum;
      bool cache;
      bool local;
      bool readonly;
      analyze_t() : bufsize(-1), bufnum(1), cache(true),
                    local(false), readonly(true) {};
    };

    /// Analyze url and provide hints.
    /// \param arg returns suggested values.
    virtual bool analyze(analyze_t& arg);

    /// Query remote server or local file system to check if object is
    /// accessible. If possible this function will also try to fill meta
    /// information about object in associated DataPoint.
    virtual bool check();

    /// Remove/delete object at URL.
    virtual bool remove();

    /// List files in directory or service (URL must point to
    /// directory/group/service access point).
    /// \param files will contain list of file names and optionally
    /// their attributes.
    /// \param resolve if false no information about attributes will be
    /// retrieved.
    virtual bool list_files(std::list <FileInfo>& files, bool resolve = true);

    /// Returns true if URL can accept scatterd data (like arbitrary access
    /// to local file) for 'writing' operation.

    virtual bool out_of_order();
    /// Allow/disallow DataPointDirect to produce scattered data during
    /// 'reading' operation.
    /// \param v true if allowed.

    virtual void out_of_order(bool v);
    /// Allow/disallow to make check for existance of remote file
    /// (and probably other checks too) before initiating 'reading' and
    /// 'writing' operations.
    /// \param v true if allowed (default is true).

    virtual void additional_checks(bool v);
    /// Check if additional checks before 'reading' and 'writing' will
    /// be performed.

    virtual bool additional_checks();
    /// Allow/disallow heavy security during data transfer.
    /// \param v true if allowed (default is true only for gsiftp://).

    virtual void secure(bool v);
    /// Check if heavy security during data transfer is allowed.

    virtual bool secure();
    /// Request passive transfers for FTP-like protocols.
    /// \param true to request.

    virtual void passive(bool v);
    /// Returns reason of transfer failure.

    virtual failure_reason_t failure_reason();
    virtual std::string failure_text();

    /// Set range of bytes to retrieve. Default values correspond to
    /// whole file.
    virtual void range(unsigned long long int start = 0,
                       unsigned long long int end = 0);

   protected:
    DataBufferPar *buffer;
    bool cacheable;
    bool linkable;
    bool is_secure;
    bool force_secure;
    bool force_passive;
    bool reading;
    bool writing;
    bool no_checks;
    /* true if allowed to produce scattered data (non-sequetial offsets) */
    bool allow_out_of_order;
    int transfer_streams;
    unsigned long long int range_start;
    unsigned long long int range_end;

    failure_reason_t failure_code;
    std::string failure_description;
  };

} // namespace Arc

#endif // __ARC_DATAPOINTDIRECT_H__
