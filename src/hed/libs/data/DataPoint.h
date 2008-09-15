#ifndef __ARC_DATAPOINT_H__
#define __ARC_DATAPOINT_H__

#include <list>
#include <map>
#include <string>

#include <arc/DateTime.h>
#include <arc/URL.h>
#include <arc/data/DataStatus.h>
#include <arc/data/FileInfo.h>

namespace Arc {

  class Logger;
  class DataBufferPar;
  class DataCallback;
  class XMLNode;

  /// This base class is an abstraction of URL.
  /** Specializations should be provided for different kind of direct
      access URLs (file://, ftp://, gsiftp://, http://, https://,
      httpg://, ...) or indexing service URLs (rls://, lfc://, ...).
      DataPoint provides means to resolve an indexing service URL into
      multiple URLs and to loop through them. */

  class DataPoint {
  public:
    /// Constructor requires URL to be provided.
    DataPoint(const URL& url);

    /// Destructor.
    virtual ~DataPoint();

    /// Returns the URL that was passed to the constructor.
    virtual const URL& GetURL() const;

    /// Returns a string representation of the DataPoint.
    virtual std::string str() const;

    /// Is DataPoint valid?
    virtual operator bool() const;

    /// Is DataPoint valid?
    virtual bool operator!() const;

    /// Start reading data from URL.
    /** Separate thread to transfer data will be created. No other
       operation can be performed while reading is in progress.
     \param buffer operation will use this buffer to put
       information into. Should not be destroyed before stop_reading
       was called and returned. */
    virtual DataStatus StartReading(DataBufferPar& buffer) = 0;

    /// Start writing data to URL.
    /** Separate thread to transfer data will be created. No other
       operation can be performed while writing is in progress.
     \param buffer operation will use this buffer to get
       information from. Should not be destroyed before stop_writing
       was called and returned.
     \param space_cb callback which is called if there is not
       enough space to store data. May not implemented for all
       protocols. */
    virtual DataStatus StartWriting(DataBufferPar& buffer,
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

    /// Query the DataPoint to check if object is accessible.
    /** If possible this method will also try to provide meta
       information about the object. */
    virtual DataStatus Check() = 0;

    /// Remove/delete object at URL.
    virtual DataStatus Remove() = 0;

    /// List file(s).
    /** If the DataPoint represents a directory its contents will be
       listed.
     \param files will contain list of file names and optionally
       their attributes.
     \param resolve if false, do not try to obtain properties of
       objects. */
    virtual DataStatus ListFiles(std::list<FileInfo>& files,
				 bool resolve = true) = 0;

    /// Allow/disallow DataPoint to produce scattered data during
    /// *reading* operation.
    /** \param v true if allowed (default is false). */
    virtual void ReadOutOfOrder(bool v) = 0;

    /// Returns true if URL can accept scattered data for *writing*
    /// operation.
    virtual bool WriteOutOfOrder() = 0;

    /// Allow/disallow additional checks
    /** Check for existance of remote file (and probably other checks
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
    /** Used for same purpose as meta_preregister. Should be called
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

    /// Get suggested buffer size for transfers.
    virtual unsigned long long int BufSize() const = 0;

    /// Get suggested number of buffers for transfers.
    virtual int BufNum() const = 0;

    /// Returns true if file is cacheable.
    virtual bool Cache() const = 0;

    /// Returns true if file is local, e.g. file:// urls.
    virtual bool Local() const = 0;

    // Returns true if file is readonly.
    virtual bool ReadOnly() const = 0;

    /// Returns number of retries left.
    virtual int GetTries() const;

    /// Set number of retries.
    virtual void SetTries(const int n);

    /// Check if URL is an Indexing Service.
    virtual bool IsIndex() const = 0;

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

    /// Returns current (resolved) URL.
    virtual const URL& CurrentLocation() const = 0;

    /// Returns meta information used to create current URL.
    /** Usage differs between different indexing services. */
    virtual const std::string& CurrentLocationMetadata() const = 0;

    /// Switch to next location in list of URLs.
    /** At last location switch to first if number of allowed retries
       is not exceeded. Returns false if no retries left. */
    virtual bool NextLocation() = 0;

    /// Returns false if out of retries.
    virtual bool LocationValid() const = 0;

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

    /// Assing credentials used for authentication
    void AssignCredentials(const std::string& proxyPath,
			   const std::string& certifcatePath,
			   const std::string& keyPath,
			   const std::string& caCertificatesDir);

    /// Assing credentials used for authentication (using XML node)
    void AssignCredentials(const XMLNode& node);

  protected:
    URL url;
    static Logger logger;

    // attributes
    unsigned long long int size;
    std::string checksum;
    Time created;
    Time valid;
    int triesleft;

    // authentication
    std::string proxyPath;
    std::string certificatePath;
    std::string keyPath;
    std::string caCertificatesDir;
  };

} // namespace Arc

#endif // __ARC_DATAPOINT_H__
