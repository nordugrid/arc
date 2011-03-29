// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_DATAPOINT_H__
#define __ARC_DATAPOINT_H__

#include <list>
#include <string>

#include <arc/DateTime.h>
#include <arc/URL.h>
#include <arc/UserConfig.h>
#include <arc/data/DataStatus.h>
#include <arc/data/FileInfo.h>
#include <arc/data/URLMap.h>
#include <arc/loader/Loader.h>
#include <arc/loader/Plugin.h>

namespace Arc {

  class Logger;
  class DataBuffer;
  class DataCallback;
  class XMLNode;
  class CheckSum;

  /// This base class is an abstraction of URL.
  /** Specializations should be provided for different kind of direct
      access URLs (file://, ftp://, gsiftp://, http://, https://,
      httpg://, ...) or indexing service URLs (rls://, lfc://, ...).
      DataPoint provides means to resolve an indexing service URL into
      multiple URLs and to loop through them. */

  class DataPoint
    : public Plugin {
  public:

    /// Describes the latency to access this URL
    /** For now this value is one of a small set specified
       by the enumeration. In the future with more sophisticated
       protocols or information it could be replaced by a more
       fine-grained list of possibilities such as an int value. */
    enum DataPointAccessLatency {
      /// URL can be accessed instantly
      ACCESS_LATENCY_ZERO,
      /// URL has low (but non-zero) access latency, for example staged from disk
      ACCESS_LATENCY_SMALL,
      /// URL has a large access latency, for example staged from tape
      ACCESS_LATENCY_LARGE
    };

    /// Describes type of information about URL to request
    enum DataPointInfoType {
      INFO_TYPE_MINIMAL = 0, /// Whatever protocol can get with no additional effort. 
      INFO_TYPE_NAME = 1, /// Only name of object (relative).
      INFO_TYPE_TYPE = 2, /// Type of object - currently file or dir.
      INFO_TYPE_TIMES = 4, /// Timestamps associated with object.
      INFO_TYPE_CONTENT = 8, /// Metadata describing content, like size, checksum, etc.
      INFO_TYPE_ACCESS = 16, /// Access control - ownership, permission, etc.
      INFO_TYPE_STRUCT = 32, /// Fine structure - replicas, transfer locations, redirections.
      INFO_TYPE_REST = 64, /// All the other parameters.
      INFO_TYPE_ALL = 127 /// All the parameters.
    };

    /// Constructor requires URL to be provided.
    /** Reference to usercfg argument is stored 
       internally and hence corresponding objects must stay
       available during whole lifetime of this instance.
       TODO: do we really need it? */
    DataPoint(const URL& url, const UserConfig& usercfg);

    /// Destructor.
    virtual ~DataPoint();

    /// Returns the URL that was passed to the constructor.
    virtual const URL& GetURL() const;

    /// Returns the UserConfig that was passed to the constructor.
    virtual const UserConfig& GetUserConfig() const;

    /// Assigns new URL.
    /// Main purpose of this method is to reuse existing 
    /// connection for accessing different object at same server.
    /// Implementation does not have to implement this method.
    /// If supplied URL is not suitable or method is not implemented
    /// false is returned.
    virtual bool SetURL(const URL& url);

    /// Returns a string representation of the DataPoint.
    virtual std::string str() const;

    /// Is DataPoint valid?
    virtual operator bool() const;

    /// Is DataPoint valid?
    virtual bool operator!() const;

    /// Prepare DataPoint for reading.
    /** This method should be implemented by protocols which require
       preparation or staging of physical files for reading. It can act
       synchronously or asynchronously (if protocol supports it). In the
       first case the method will block until the file is prepared or the
       specified timeout has passed. In the second case the method can
       return with a ReadPrepareWait status before the file is prepared.
       The caller should then wait some time (a hint from the remote service
       may be given in wait_time) and call PrepareReading() again to poll for
       the preparation status, until the file is prepared. In this case it is
       also up to the caller to decide when the request has taken too long
       and if so cancel it by calling FinishReading().
       When file preparation has finished, the physical file(s)
       to read from can be found from TransferLocations().
       \param timeout If non-zero, this method will block until either the
       file has been prepared successfully or the timeout has passed. A zero
       value means that the caller would like to call and poll for status.
       \param wait_time If timeout is zero (caller would like asynchronous
       operation) and ReadPrepareWait is returned, a hint for how long to wait
       before a subsequent call may be given in wait_time.
     */
    virtual DataStatus PrepareReading(unsigned int timeout,
                                      unsigned int& wait_time);

    /// Prepare DataPoint for writing.
    /** This method should be implemented by protocols which require
       preparation of physical files for writing. It can act
       synchronously or asynchronously (if protocol supports it). In the
       first case the method will block until the file is prepared or the
       specified timeout has passed. In the second case the method can
       return with a WritePrepareWait status before the file is prepared.
       The caller should then wait some time (a hint from the remote service
       may be given in wait_time) and call PrepareWriting() again to poll for
       the preparation status, until the file is prepared. In this case it is
       also up to the caller to decide when the request has taken too long
       and if so cancel or abort it by calling FinishWriting(true).
       When file preparation has finished, the physical file(s)
       to write to can be found from TransferLocations().
       \param timeout If non-zero, this method will block until either the
       file has been prepared successfully or the timeout has passed. A zero
       value means that the caller would like to call and poll for status.
       \param wait_time If timeout is zero (caller would like asynchronous
       operation) and WritePrepareWait is returned, a hint for how long to wait
       before a subsequent call may be given in wait_time.
     */
    virtual DataStatus PrepareWriting(unsigned int timeout,
                                      unsigned int& wait_time);

    /// Start reading data from URL.
    /** Separate thread to transfer data will be created. No other
       operation can be performed while reading is in progress.
       \param buffer operation will use this buffer to put
       information into. Should not be destroyed before StopReading()
       was called and returned. */
    virtual DataStatus StartReading(DataBuffer& buffer) = 0;

    /// Start writing data to URL.
    /** Separate thread to transfer data will be created. No other
       operation can be performed while writing is in progress.
       \param buffer operation will use this buffer to get
       information from. Should not be destroyed before stop_writing
       was called and returned.
       \param space_cb callback which is called if there is not
       enough space to store data. May not implemented for all
       protocols. */
    virtual DataStatus StartWriting(DataBuffer& buffer,
                                    DataCallback *space_cb = NULL) = 0;

    /// Stop reading.
    /** Must be called after corresponding start_reading method,
       either after all data is transferred or to cancel transfer.
       Use buffer object to find out when data is transferred.
       Must return failure if any happened during transfer. */
    virtual DataStatus StopReading() = 0;

    /// Stop writing.
    /** Must be called after corresponding start_writing method,
       either after all data is transferred or to cancel transfer.
       Use buffer object to find out when data is transferred.
       Must return failure if any happened during transfer. */
    virtual DataStatus StopWriting() = 0;

    /// Finish reading from the URL.
    /** Must be called after transfer of physical file has completed and if
       PrepareReading() was called, to free resources, release requests that
       were made during preparation etc.
       \param error If true then action is taken depending on the error. */
    virtual DataStatus FinishReading(bool error = false);

    /// Finish writing to the URL.
    /** Must be called after transfer of physical file has completed and if
       PrepareWriting() was called, to free resources, release requests that
       were made during preparation etc.
       \param error If true then action is taken depending on the error. */
    virtual DataStatus FinishWriting(bool error = false);

    /// Query the DataPoint to check if object is accessible.
    /** If possible this method will also try to provide meta
       information about the object. It returns positive response
       if object's content can be retrieved. */
    virtual DataStatus Check() = 0;

    /// Remove/delete object at URL.
    virtual DataStatus Remove() = 0;

    /// Retrieve information about this object
    /** If the DataPoint represents a directory or something similar,
       information about the object itself and not its contents will
       be obtained.
       \param file will contain object name and requested attributes.
       There may be more attributes than requested. There may be less 
       if object can't provide particular information.
       \param verb defines attribute types which method must try to
       retrieve. It is not a failure if some attributes could not
       be retrieved due to limitation of protocol or access control. */
    virtual DataStatus Stat(FileInfo& file, DataPointInfoType verb = INFO_TYPE_ALL) = 0;

    /// List hierarchical content of this object.
    /** If the DataPoint represents a directory or something similar its
       contents will be listed.
       \param files will contain list of file names and requested
       attributes. There may be more attributes than requested. There
       may be less if object can't provide particular information.
       \param verb defines attribute types which method must try to
       retrieve. It is not a failure if some attributes could not
       be retrieved due to limitation of protocol or access control. */
    virtual DataStatus List(std::list<FileInfo>& files, DataPointInfoType verb = INFO_TYPE_ALL) = 0;

    /// Allow/disallow DataPoint to produce scattered data during
    /// *reading* operation.
    /** \param v true if allowed (default is false). */
    virtual void ReadOutOfOrder(bool v) = 0;

    /// Returns true if URL can accept scattered data for *writing*
    /// operation.
    virtual bool WriteOutOfOrder() = 0;

    /// Allow/disallow additional checks
    /** Check for existence of remote file (and probably other checks
       too) before initiating reading and writing operations.
       \param v true if allowed (default is true). */
    virtual void SetAdditionalChecks(bool v) = 0;

    /// Check if additional checks before will be performed.
    virtual bool GetAdditionalChecks() const = 0;

    /// Allow/disallow heavy security during data transfer.
    /** \param v true if allowed (default depends on protocol). */
    virtual void SetSecure(bool v) = 0;

    /// Check if heavy security during data transfer is allowed.
    virtual bool GetSecure() const = 0;

    /// Request passive transfers for FTP-like protocols.
    /// \param true to request.
    virtual void Passive(bool v) = 0;

    /// Returns reason of transfer failure, as reported by callbacks.
    /// This could be different from the failure returned by the methods themselves.
    virtual DataStatus GetFailureReason(void) const;

    /// Set range of bytes to retrieve.
    /** Default values correspond to whole file. */
    virtual void Range(unsigned long long int start = 0,
                       unsigned long long int end = 0) = 0;

    /// Resolves index service URL into list of ordinary URLs.
    /** Also obtains meta information about the file.
       \param source true if DataPoint object represents source of
       information. */
    virtual DataStatus Resolve(bool source) = 0;

    /// Check if file is registered in Indexing Service.
    /** Proper value is obtainable only after Resolve. */
    virtual bool Registered() const = 0;

    /// Index service preregistration.
    /** This function registers the physical location of a file into
       an indexing service. It should be called *before* the actual
       transfer to that location happens.
       \param replication if true, the file is being replicated
       between two locations registered in the indexing service under
       same name.
       \param force if true, perform registration of a new file even
       if it already exists. Should be used to fix failures in
       Indexing Service. */
    virtual DataStatus PreRegister(bool replication, bool force = false) = 0;

    /// Index Service postregistration.
    /** Used for same purpose as PreRegister. Should be called
       after actual transfer of file successfully finished.
       \param replication if true, the file is being replicated
       between two locations registered in Indexing Service under
       same name. */
    virtual DataStatus PostRegister(bool replication) = 0;

    /// Index Service preunregistration.
    /** Should be called if file transfer failed. It removes changes made
       by PreRegister.
       \param replication if true, the file is being replicated
       between two locations registered in Indexing Service under
       same name. */
    virtual DataStatus PreUnregister(bool replication) = 0;

    /// Index Service unregistration.
    /** Remove information about file registered in Indexing Service.
       \param all if true, information about file itself is (LFN) is
       removed. Otherwise only particular physical instance is
       unregistered. */
    virtual DataStatus Unregister(bool all) = 0;

    /// Check if meta-information 'size' is available.
    virtual bool CheckSize() const;

    /// Set value of meta-information 'size'.
    virtual void SetSize(const unsigned long long int val);

    /// Get value of meta-information 'size'.
    virtual unsigned long long int GetSize() const;

    /// Check if meta-information 'checksum' is available.
    virtual bool CheckCheckSum() const;

    /// Set value of meta-information 'checksum'.
    virtual void SetCheckSum(const std::string& val);

    /// Get value of meta-information 'checksum'.
    virtual const std::string& GetCheckSum() const;

    /// Default checksum type
    virtual const std::string DefaultCheckSum() const;

    /// Check if meta-information 'creation/modification time' is available.
    virtual bool CheckCreated() const;

    /// Set value of meta-information 'creation/modification time'.
    virtual void SetCreated(const Time& val);

    /// Get value of meta-information 'creation/modification time'.
    virtual const Time& GetCreated() const;

    /// Check if meta-information 'validity time' is available.
    virtual bool CheckValid() const;

    /// Set value of meta-information 'validity time'.
    virtual void SetValid(const Time& val);

    /// Get value of meta-information 'validity time'.
    virtual const Time& GetValid() const;

    /// Set value of meta-information 'access latency'
    virtual void SetAccessLatency(const DataPointAccessLatency& latency);

    /// Get value of meta-information 'access latency'
    virtual DataPointAccessLatency GetAccessLatency() const;

    /// Get suggested buffer size for transfers.
    virtual long long int BufSize() const = 0;

    /// Get suggested number of buffers for transfers.
    virtual int BufNum() const = 0;

    /// Returns true if file is cacheable.
    virtual bool Cache() const;

    /// Returns true if file is local, e.g. file:// urls.
    virtual bool Local() const = 0;

    // Returns true if file is readonly.
    virtual bool ReadOnly() const = 0;

    /// Returns number of retries left.
    virtual int GetTries() const;

    /// Set number of retries.
    virtual void SetTries(const int n);

    /// Decrease number of retries left.
    virtual void NextTry(void);

    /// Check if URL is an Indexing Service.
    virtual bool IsIndex() const = 0;

    /// If URL should be staged or queried for Transport URL (TURL)
    virtual bool IsStageable() const;

    /// If endpoint can have any use from meta information.
    virtual bool AcceptsMeta() = 0;

    /// If endpoint can provide at least some meta information
    /// directly.
    virtual bool ProvidesMeta() = 0;

    /// Copy meta information from another object.
    /** Already defined values are not overwritten.
       \param p object from which information is taken. */
    virtual void SetMeta(const DataPoint& p);

    /// Compare meta information from another object.
    /** Undefined values are not used for comparison.
       \param p object to which to compare. */
    virtual bool CompareMeta(const DataPoint& p) const;

    /// Returns physical file(s) to read/write, if different from CurrentLocation()
    /** To be used with protocols which re-direct to different URLs such as
       Transport URLs (TURLs). The list is initially filled by PrepareReading
       and PrepareWriting. If this list is non-empty then real transfer
       should use a URL from this list. It is up to the caller to choose the
       best URL and instantiate new DataPoint for handling it.
       For consistency protocols which do not require redirections return 
       original URL.
       For protocols which need redirection calling StartReading and StartWriting 
       will use first URL in the list. */
    virtual std::vector<URL> TransferLocations() const;

    /// Returns current (resolved) URL.
    virtual const URL& CurrentLocation() const = 0;

    /// Returns meta information used to create current URL.
    /** Usage differs between different indexing services. */
    virtual const std::string& CurrentLocationMetadata() const = 0;

    /// Compare metadata of DataPoint and current location
    /** Returns inconsistency error or error encountered during
     * operation, or success */
    virtual DataStatus CompareLocationMetadata() const = 0;
    
    /// Switch to next location in list of URLs.
    /** At last location switch to first if number of allowed retries
       is not exceeded. Returns false if no retries left. */
    virtual bool NextLocation() = 0;

    /// Returns false if out of retries.
    virtual bool LocationValid() const = 0;

    /// Returns true if the current location is the last
    virtual bool LastLocation() = 0;

    /// Returns true if number of resolved URLs is not 0.
    virtual bool HaveLocations() const = 0;

    /// Add URL to list.
    /** \param url Location URL to add.
       \param meta Location meta information. */
    virtual DataStatus AddLocation(const URL& url,
                                   const std::string& meta) = 0;

    /// Remove current URL from list
    virtual DataStatus RemoveLocation() = 0;

    /// Remove locations present in another DataPoint object
    virtual DataStatus RemoveLocations(const DataPoint& p) = 0;

    /// Add a checksum object which will compute checksum during transmission.
    /// \param cksum object which will compute checksum. Should not be
    /// destroyed till DataPointer itself.
    /// \return integer position in the list of checksum objects.
    virtual int AddCheckSumObject(CheckSum *cksum) = 0;

    virtual const CheckSum* GetCheckSumObject(int index) const = 0;

    /// Sort locations according to the specified pattern.
    /// \param pattern a set of strings, separated by |, to match against.
    virtual void SortLocations(const std::string& pattern,
                               const URLMap& url_map) = 0;

  protected:
    URL url;
    const UserConfig& usercfg;

    // attributes
    unsigned long long int size;
    std::string checksum;
    Time created;
    Time valid;
    DataPointAccessLatency access_latency;
    int triesleft;
    DataStatus failure_code; /* filled by callback methods */
    bool cache;
    bool stageable;
    /** Subclasses should add their own specific options to this list */
    std::list<std::string> valid_url_options;

    static Logger logger;
  };

  class DataPointLoader
    : public Loader {
  private:
    DataPointLoader();
    ~DataPointLoader();
    DataPoint* load(const URL& url, const UserConfig& usercfg);
    friend class DataHandle;
  };

  class DataPointPluginArgument
    : public PluginArgument {
  public:
    DataPointPluginArgument(const URL& url, const UserConfig& usercfg)
      : url(url),
	usercfg(usercfg) {}
    ~DataPointPluginArgument() {}
    operator const URL&() {
      return url;
    }
    operator const UserConfig&() {
      return usercfg;
    }
  private:
    const URL& url;
    const UserConfig& usercfg;
  };

} // namespace Arc

#endif // __ARC_DATAPOINT_H__
