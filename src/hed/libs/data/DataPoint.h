#ifndef __ARC_DATAPOINT_H__
#define __ARC_DATAPOINT_H__

#include <string>
#include <list>
#include <map>

#include <arc/DateTime.h>
#include <arc/URL.h>
#include <arc/data/FileInfo.h>

namespace Arc {

  class Logger;
  class DataBufferPar;
  class DataCallback;

  /// This class is an abstraction of URL. 
  /** It can handle URLs of type file://, ftp://, gsiftp://, http://, https://,
   httpg:// (HTTP over GSI), se:// (NG web service over HTTPG) and meta-URLs 
   (URLs of Infexing Services) rc://, rls://.
   DataPoint provides means to resolve meta-URL into multiple URLs and
   to loop through them. */

  class DataPoint {
   public:
    /// Constructor requires URL or meta-URL to be provided
    DataPoint(const URL& url);
    virtual ~DataPoint() {};

    /// Reason of transfer failure
    typedef enum {
      common_failure = 0,
      credentials_expired_failure = 1
    } failure_reason_t;

    /*
     *  Direct actions. Valid only for direct URLs
     */

    /// Start reading data from URL.
    /// Separate thread to transfer data will be created. No other operation
    /// can be performed while 'reading' is in progress.
    /// \param buffer operation will use this buffer to put information into.
    /// Should not be destroyed before stop_reading was called and returned.
    /// \returns true on success.
    virtual bool start_reading(DataBufferPar& buffer) = 0;

    /// Start writing data to URL.
    /// Separate thread to transfer data will be created. No other operation
    /// can be performed while 'writing' is in progress.
    /// \param buffer operation will use this buffer to get information from.
    /// Should not be destroyed before stop_writing was called and returned.
    /// \param space_cb callback which is called if there is not
    /// enough to space storing data. Currently implemented only for
    /// file:/// URL. 
    /// \returns true on success.
    virtual bool start_writing(DataBufferPar& buffer,
                               DataCallback *space_cb = NULL) = 0;

    /// Stop reading. It MUST be called after corressponding start_reading
    /// method. Either after whole data is transfered or to cancel transfer.
    /// Use 'buffer' object to find out when data is transfered.
    virtual bool stop_reading() = 0;

    /// Same as stop_reading but for corresponding start_writing.
    virtual bool stop_writing() = 0;

    /// Query remote server or local file system to check if object is
    /// accessible. If possible this function will also try to fill meta
    /// information about object in associated DataPoint.
    virtual bool check() = 0;

    /// Remove/delete object at URL.
    virtual bool remove() = 0;

    /// Returns true if URL can accept scatterd data (like arbitrary access
    /// to local file) for 'writing' operation.
    virtual bool out_of_order() = 0;

    /// Allow/disallow DataPoint to produce scattered data during
    /// 'reading' operation.
    /// \param v true if allowed.
    virtual void out_of_order(bool v) = 0;

    /// Allow/disallow to make check for existance of remote file
    /// (and probably other checks too) before initiating 'reading' and
    /// 'writing' operations.
    /// \param v true if allowed (default is true).
    virtual void additional_checks(bool v) = 0;

    /// Check if additional checks before 'reading' and 'writing' will
    /// be performed.
    virtual bool additional_checks() = 0;

    /// Allow/disallow heavy security during data transfer.
    /// \param v true if allowed (default is true only for gsiftp://).
    virtual void secure(bool v) = 0;

    /// Check if heavy security during data transfer is allowed.
    virtual bool secure() = 0;

    /// Request passive transfers for FTP-like protocols.
    /// \param true to request.
    virtual void passive(bool v) = 0;

    /// Returns reason of transfer failure.
    virtual failure_reason_t failure_reason() = 0;

    /// Set range of bytes to retrieve. Default values correspond to
    /// whole file.
    virtual void range(unsigned long long int start = 0,
                       unsigned long long int end = 0) = 0;

    virtual std::string failure_text() = 0;

    /*
     *  META actions. Valid only for meta-URLs
     */

    /// Resolve meta-URL into list of ordinary URLs and obtain meta-information
    /// about file. Can be called for object representing ordinary URL or
    /// already resolved object.
    /// \param source true if DataPoint object represents source of information
    virtual bool meta_resolve(bool source) = 0;

    /// This function registers physical location of file into Indexing
    /// Service. It should be called *before* actual transfer to that
    /// location happens.
    /// \param replication if true then file is being replicated between
    /// 2 locations registered in Indexing Service under same name.
    /// \param force if true, perform registration of new file even if it
    /// already exists. Should be used to fix failures in Indexing Service.
    virtual bool meta_preregister(bool replication, bool force = false) = 0;

    /// Used for same purpose as meta_preregister. Should be called after
    /// actual transfer of file successfully finished.
    /// \param replication if true then file is being replicated between
    /// 2 locations registered in Indexing Service under same name.
    virtual bool meta_postregister(bool replication) = 0;

    // Same operation as meta_preregister and meta_postregister together.
    virtual bool meta_register(bool replication) {
      if(!meta_preregister(replication)) return false;
      if(!meta_postregister(replication)) return false;
      return true;
    };

    /// Should be called if file transfer failed. It removes changes made
    /// by meta_preregister.
    virtual bool meta_preunregister(bool replication) = 0;

    /// Remove information about file registered in Indexing Service.
    /// \param all if true information about file itself is (LFN) is removed.
    /// Otherwise only particular physical instance is unregistered.
    virtual bool meta_unregister(bool all) = 0;

    /*
     * Set and get corresponding meta-information related to URL.
     * Those attributes can be supported by non-meta-URLs too.
     */

    /// Check if meta-information 'size' is available.
    virtual bool CheckSize() const {
      return (size != (unsigned long long int)(-1));
    };

    /// Set value of meta-information 'size'.
    virtual void SetSize(const unsigned long long int val) {
      size = val;
    };

    /// Get value of meta-information 'size'.
    virtual unsigned long long int GetSize() const {
      return size;
    };

    /// Check if meta-information 'checksum' is available.
    virtual bool CheckCheckSum() const {
      return (!checksum.empty());
    };

    /// Set value of meta-information 'checksum'.
    virtual void SetCheckSum(const std::string& val) {
      checksum = val;
    };

    /// Get value of meta-information 'checksum'.
    virtual const std::string& GetCheckSum() const {
      return checksum;
    };

    /// Check if meta-information 'creation/modification time' is available.
    virtual bool CheckCreated() const {
      return (created != -1);
    };

    /// Set value of meta-information 'creation/modification time'.
    virtual void SetCreated(const Time& val) {
      created = val;
    };

    /// Get value of meta-information 'creation/modification time'.
    virtual const Time& GetCreated() const {
      return created;
    };

    /// Check if meta-information 'validity time' is available.
    virtual bool CheckValid() const {
      return (valid != -1);
    };

    /// Set value of meta-information 'validity time'.
    virtual void SetValid(const Time& val) {
      valid = val;
    };

    /// Get value of meta-information 'validity time'.
    virtual const Time& GetValid() const {
      return valid;
    };

    /// Get suggested buffer size for transfers.
    virtual unsigned long long int BufSize() const {
      return bufsize;
    }

    /// Get suggested number of buffers for transfers.
    virtual int BufNum() const {
      return bufnum;
    }

    /// Returns true if file is cacheable.
    virtual bool Cache() const {
      return cache;
    }

    /// Returns true if file is local, e.g. file:// urls.
    virtual bool Local() const {
      return local;
    }

    // Returns true if file is readonly.
    virtual bool ReadOnly() const {
      return readonly;
    }

    /// Check if URL is meta-URL.
    virtual bool meta() const = 0;

    /// If endpoint can have any use from meta information.
    virtual bool accepts_meta() = 0;

    /// If endpoint can provide at least some meta information directly.
    virtual bool provides_meta() = 0;

    /// Acquire meta-information from another object. Defined values are
    /// not overwritten.
    /// \param p object from which information is taken.
    virtual void meta(const DataPoint& p) {
      if(!CheckSize())
        SetSize(p.GetSize());
      if(!CheckCheckSum())
        SetCheckSum(p.GetCheckSum());
      if(!CheckCreated())
        SetCreated(p.GetCreated());
      if(!CheckValid())
        SetValid(p.GetValid());
    };

    /// Compare meta-information form another object. Undefined values
    /// are not used for comparison. Default result is 'true'.
    /// \param p object to which compare.
    virtual bool meta_compare(const DataPoint& p) const {
      if(CheckSize() && p.CheckSize())
        if(GetSize() != p.GetSize())
          return false;
      // TODO: compare checksums properly
      if(CheckCheckSum() && p.CheckCheckSum())
        if(strcasecmp(GetCheckSum().c_str(), p.GetCheckSum().c_str()))
          return false;
      if(CheckCreated() && p.CheckCreated())
        if(GetCreated() != p.GetCreated())
          return false;
      if(CheckValid() && p.CheckValid())
        if(GetValid() != p.GetValid())
          return false;
      return true;
    };

    /// Check if file is registered in Indexing Service. Proper value is
    /// obtainable only after meta-resolve.
    virtual bool meta_stored() = 0;

    virtual operator bool () const {
      return (bool)url;
    };

    virtual bool operator!() const {
      return !url;
    };

    /*
     *  Methods to manage list of locations.
     */

    /// Returns current (resolved) URL.
    virtual const URL& current_location() const = 0;

    /// Returns meta information used to create curent URL. For RC that is
    ///  location's name. For RLS that is equal to pfn.
    virtual const std::string& current_meta_location() const = 0;

    /// Switch to next location in list of URLs. At last location
    /// switch to first if number of allowed retries does not exceeded.
    /// Returns false if no retries left.
    virtual bool next_location() = 0;

    /// Returns false if out of retries.
    virtual bool have_location() const = 0;

    /// Returns true if number of resolved URLs is not 0.
    virtual bool have_locations() const = 0;

    /// Add URL to list.
    /// \param meta meta-name (name of location/service).
    /// \param loc URL.
    virtual bool add_location(const std::string& meta, const URL& loc) = 0;

    /// Remove current URL from list
    virtual bool remove_location() = 0;

    /// Remove locations present in another DataPoint object
    virtual bool remove_locations(const DataPoint& p) = 0;

    /// List files in directory or service.
    /// \param files will contain list of file names and optionally
    /// their attributes.
    /// \param resolve if false, do not try to obtain properties of objects.
    virtual bool list_files(std::list<FileInfo>& files,
                            bool resolve = true) = 0;

    /// Returns number of retries left.
    virtual int GetTries() const;

    /// Set number of retries.
    virtual void SetTries(const int n);

    /// Returns URL which was passed to constructor
    virtual const URL& base_url() const;

    /// Returns a string representation of the DataPoint.
    virtual std::string str() const;

   protected:
    URL url;
    static Logger logger;

    // attributes
    unsigned long long int size;
    std::string checksum;
    Time created;
    Time valid;
    int triesleft;
    unsigned long long int bufsize;
    int bufnum;
    bool cache;
    bool local;
    bool readonly;
  };

} // namespace Arc

#endif // __ARC_DATAPOINT_H__
