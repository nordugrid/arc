// -*- indent-tabs-mode: nil -*-

// Summary page for libarcdata doxygen module
// Enclosed in Arc namespace so automatic linking to other classes works
namespace Arc {
/**
 * \defgroup data ARC data library (libarcdata)
 *
 * libarcdata is a library for access to data on the Grid. It provides a
 * uniform interface to several types of Grid storage and catalogs using
 * various protocols. The protocols usable on a given system depend on the
 * packages installed. The interface can be used to read, write, list, transfer
 * and delete data to and from storage systems and catalogs.
 *
 * The library uses ARC's dynamic plugin mechanism to load plugins for
 * specific protocols only when required at runtime. These plugins are
 * called Data Manager Components (DMCs). The DataHandle class takes care of
 * automatically loading the required DMC at runtime to create a DataPoint object
 * representing a resource accessible through a given protocol. DataHandle
 * should always be used instead of DataPoint directly.
 *
 * To create a new DMC for a protocol which is not yet supported see the
 * instruction and examples in the DataPoint class documentation. This
 * documentation also gives a complete overview of the interface.
 *
 * The following protocols are currently supported in standard distributions
 * of ARC.
 *
 * - File (%file://) - Regular local file system.
 *
 * - GridFTP (gsiftp://) - GridFTP is essentially the FTP protocol with GSI
 * security. Regular FTP can also be used.
 *
 * - HTTP(S/G) (%http://) - Hypertext Transfer Protocol. HTTP over SSL (HTTPS)
 * and HTTP over GSI (HTTPG) are also supported.
 *
 * - LDAP (ldap://) - Lightweight Directory Access Protocol. LDAP is used in
 * grids mainly to store information about grid services or resources rather
 * than to store data itself.
 *
 * - SRM (srm://) - The Storage Resource Manager (SRM) protocol allows access
 * to data distributed across physical storage through a unified namespace
 * and management interface.
 *
 * - XRootd (root://) - Protocol for data access across large scale storage
 * clusters. More information can be found at http://xrootd.slac.stanford.edu/
 *
 * - ACIX (acix://) - The ARC Cache Index contains locations of cached files
 * in ARC CE caches.
 *
 * DataMover provides a simple high-level interface to copy files. Fine-grained
 * control over data transfer is shown in the following example:
 *
 * \include partial_copy.cpp
 *
 * And the same example in python
 *
 * \include partial_copy.py
 */
} // namespace Arc

#ifndef __ARC_DATAPOINT_H__
#define __ARC_DATAPOINT_H__

#include <list>
#include <set>
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

  /// A DataPoint represents a data resource and is an abstraction of a URL.
  /**
   * DataPoint uses ARC's Plugin mechanism to dynamically load the required
   * Data Manager Component (DMC) when necessary. A DMC typically defines a
   * subclass of DataPoint (e.g. DataPointHTTP) and is responsible for a
   * specific protocol (e.g. http). DataPoints should not be used directly,
   * instead the DataHandle wrapper class should be used, which automatically
   * loads the correct DMC. Examples of how to use DataPoint methods are shown
   * in the DataHandle documentation.
   *
   * DataPoint defines methods for access to the data resource. To transfer
   * data between two DataPoints, DataMover::Transfer() can be used.
   *
   * There are two subclasses of DataPoint, DataPointDirect and DataPointIndex.
   * None of these three classes can be instantiated directly.
   * DataPointDirect and its subclasses handle "physical" resources through
   * protocols such as file, http and gsiftp. These classes implement methods
   * such as StartReading() and StartWriting(). DataPointIndex and its
   * subclasses handle resources such as indexes and catalogs and implement
   * methods like Resolve() and PreRegister().
   *
   * When creating a new DMC, a subclass of either DataPointDirect or
   * DataPointIndex should be created, and the appropriate methods implemented.
   * DataPoint itself has no direct external dependencies, but plugins may
   * rely on third-party components. The new DMC must also add itself to the
   * list of available plugins and provide an Instance() method which returns
   * a new instance of itself, if the supplied arguments are valid for the
   * protocol. Here is an example skeleton implementation of a new DMC for
   * protocol MyProtocol which represents a physical resource accessible through
   * protocol my://
   * \include DataPointMyProtocol.cpp
   *
   * \ingroup data
   * \headerfile DataPoint.h arc/data/DataPoint.h
   */
  class DataPoint
    : public Plugin {
  public:

    /// Callback for use in protocol-internal or 3rd party transfers.
    /**
     * Will be called periodically during the transfer with the number of bytes
     * transferred so far.
     * \param bytes_transferred the number of bytes transferred so far
     */
    typedef void(*TransferCallback)(unsigned long long int bytes_transferred);

    /// Describes the latency to access this URL
    /**
     * For now this value is one of a small set specified
     * by the enumeration. In the future with more sophisticated
     * protocols or information it could be replaced by a more
     * fine-grained list of possibilities such as an int value.
     */
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
      INFO_TYPE_MINIMAL = 0,  ///< Whatever protocol can get with no additional effort.
      INFO_TYPE_NAME = 1,     ///< Only name of object (relative).
      INFO_TYPE_TYPE = 2,     ///< Type of object - currently file or dir.
      INFO_TYPE_TIMES = 4,    ///< Timestamps associated with object.
      INFO_TYPE_CONTENT = 8,  ///< Metadata describing content, like size, etc.
      INFO_TYPE_CKSUM = 128,  ///< Metadata describing checksum.
      INFO_TYPE_ACCESS = 16,  ///< Access control - ownership, permission, etc.
      INFO_TYPE_STRUCT = 32,  ///< Fine structure - replicas, transfer locations, redirections.
      INFO_TYPE_REST = 64,    ///< All the other parameters.
      INFO_TYPE_ALL = 255     ///< All the parameters.
    };


    /// Do a transfer in a single operation
    /**
     * This method is designed for plugins which support doing transfers
     * between different endpoints in a single operation, rather than two
     * separate DataPoints communicating with each other via a DataBuffer.
     * \param endpoint Other endpoint to transfer to or from
     * \param source Whether this DataPoint is the source (true) or destination
     * (false)
     * \param callback Optional monitoring callback
     * \return Outcome of transfer
     * \since 6.0.0
     */
    virtual DataStatus Transfer(const URL& otherendpoint, bool source,
                                TransferCallback callback = NULL);

    /// Perform third party transfer.
    /**
     * Credentials are delegated to the destination and it pulls data from the
     * source, i.e. data flows directly between source and destination instead
     * of through the client. A callback function can be supplied to monitor
     * progress. This method blocks until the transfer is complete. It is
     * static because third party transfer requires different DMC plugins than
     * those loaded by DataHandle for the same protocol. The third party
     * transfer plugins are loaded internally in this method.
     * \param source Source URL to pull data from
     * \param destination Destination URL which pulls data to itself
     * \param usercfg Configuration information
     * \param callback Optional monitoring callback
     * \return outcome of transfer
     */
    static DataStatus Transfer3rdParty(const URL& source, const URL& destination,
                                       const UserConfig& usercfg, TransferCallback callback = NULL);

    /// Destructor.
    virtual ~DataPoint();

    /// Returns the URL that was passed to the constructor.
    virtual const URL& GetURL() const;

    /// Returns the UserConfig that was passed to the constructor.
    virtual const UserConfig& GetUserConfig() const;

    /// Assigns new URL.
    /**
     * The main purpose of this method is to reuse an existing connection for
     * accessing a different object on the same server. The DataPoint
     * implementation does not have to implement this method. If the supplied
     * URL is not suitable or method is not implemented false is returned.
     * \param url New URL
     * \return true if switching to new URL is supported and succeeded
     */
    virtual bool SetURL(const URL& url);

    /// Returns a string representation of the DataPoint.
    virtual std::string str() const;

    /// Is DataPoint valid?
    virtual operator bool() const;

    /// Is DataPoint valid?
    virtual bool operator!() const;

    /// Prepare DataPoint for reading.
    /**
     * This method should be implemented by protocols which require
     * preparation or staging of physical files for reading. It can act
     * synchronously or asynchronously (if protocol supports it). In the
     * first case the method will block until the file is prepared or the
     * specified timeout has passed. In the second case the method can
     * return with a ReadPrepareWait status before the file is prepared.
     * The caller should then wait some time (a hint from the remote service
     * may be given in wait_time) and call PrepareReading() again to poll for
     * the preparation status, until the file is prepared. In this case it is
     * also up to the caller to decide when the request has taken too long
     * and if so cancel it by calling FinishReading().
     * When file preparation has finished, the physical file(s)
     * to read from can be found from TransferLocations().
     * \param timeout If non-zero, this method will block until either the
     * file has been prepared successfully or the timeout has passed. A zero
     * value means that the caller would like to call and poll for status.
     * \param wait_time If timeout is zero (caller would like asynchronous
     * operation) and ReadPrepareWait is returned, a hint for how long to wait
     * before a subsequent call may be given in wait_time.
     * \return Status of the operation
     */
    virtual DataStatus PrepareReading(unsigned int timeout,
                                      unsigned int& wait_time);

    /// Prepare DataPoint for writing.
    /**
     * This method should be implemented by protocols which require
     * preparation of physical files for writing. It can act
     * synchronously or asynchronously (if protocol supports it). In the
     * first case the method will block until the file is prepared or the
     * specified timeout has passed. In the second case the method can
     * return with a WritePrepareWait status before the file is prepared.
     * The caller should then wait some time (a hint from the remote service
     * may be given in wait_time) and call PrepareWriting() again to poll for
     * the preparation status, until the file is prepared. In this case it is
     * also up to the caller to decide when the request has taken too long
     * and if so cancel or abort it by calling FinishWriting(true).
     * When file preparation has finished, the physical file(s)
     * to write to can be found from TransferLocations().
     * \param timeout If non-zero, this method will block until either the
     * file has been prepared successfully or the timeout has passed. A zero
     * value means that the caller would like to call and poll for status.
     * \param wait_time If timeout is zero (caller would like asynchronous
     * operation) and WritePrepareWait is returned, a hint for how long to wait
     * before a subsequent call may be given in wait_time.
     * \return Status of the operation
     */
    virtual DataStatus PrepareWriting(unsigned int timeout,
                                      unsigned int& wait_time);

    /// Start reading data from URL.
    /**
     * A separate thread to transfer data will be created. No other
     * operation can be performed while reading is in progress. Progress of the
     * transfer should be followed using the DataBuffer object.
     * \param buffer operation will use this buffer to put
     * information into. Should not be destroyed before StopReading()
     * was called and returned. If StopReading() is not called explicitly
     * to release buffer it will be released in destructor of DataPoint
     * which also usually calls StopReading().
     * \return success if a thread was successfully started to start reading
     */
    virtual DataStatus StartReading(DataBuffer& buffer) = 0;

    /// Start writing data to URL.
    /**
     * A separate thread to transfer data will be created. No other
     * operation can be performed while writing is in progress. Progress of the
     * transfer should be followed using the DataBuffer object.
     * \param buffer operation will use this buffer to get
     * information from. Should not be destroyed before StopWriting()
     * was called and returned. If StopWriting() is not called explicitly
     * to release buffer it will be released in destructor of DataPoint
     * which also usually calls StopWriting().
     * \param space_cb callback which is called if there is not
     * enough space to store data. May not implemented for all
     * protocols.
     * \return success if a thread was successfully started to start writing
     */
    virtual DataStatus StartWriting(DataBuffer& buffer,
                                    DataCallback *space_cb = NULL) = 0;

    /// Stop reading.
    /**
     * Must be called after corresponding StartReading() method,
     * either after all data is transferred or to cancel transfer.
     * Use buffer object to find out when data is transferred.
     * \return outcome of stopping reading (not outcome of transfer itself)
     */
    virtual DataStatus StopReading() = 0;

    /// Stop writing.
    /**
     * Must be called after corresponding StartWriting() method,
     * either after all data is transferred or to cancel transfer.
     * Use buffer object to find out when data is transferred.
     * \return outcome of stopping writing (not outcome of transfer itself)
     */
    virtual DataStatus StopWriting() = 0;

    /// Finish reading from the URL.
    /**
     * Must be called after transfer of physical file has completed if
     * PrepareReading() was called, to free resources, release requests that
     * were made during preparation etc.
     * \param error If true then action is taken depending on the error.
     * \return success if source was released properly
     */
    virtual DataStatus FinishReading(bool error = false);

    /// Finish writing to the URL.
    /**
     * Must be called after transfer of physical file has completed if
     * PrepareWriting() was called, to free resources, release requests that
     * were made during preparation etc.
     * \param error if true then action is taken depending on the error, for
     * example cleaning the file from the storage
     * \return success if destination was released properly
     */
    virtual DataStatus FinishWriting(bool error = false);

    /// Query the DataPoint to check if object is accessible.
    /**
     * If check_meta is true this method will also try to provide meta
     * information about the object. Note that for many protocols an access
     * check also provides meta information and so check_meta may have no
     * effect.
     * \param check_meta If true then the method will try to retrieve meta data
     * during the check.
     * \return success if the object is accessible by the caller.
     */
    virtual DataStatus Check(bool check_meta) = 0;

    /// Remove/delete object at URL.
    virtual DataStatus Remove() = 0;

    /// Retrieve information about this object
    /**
     * If the DataPoint represents a directory or something similar,
     * information about the object itself and not its contents will
     * be obtained.
     * \param file will contain object name and requested attributes.
     * There may be more attributes than requested. There may be less
     * if object can't provide particular information.
     * \param verb defines attribute types which method must try to
     * retrieve. It is not a failure if some attributes could not
     * be retrieved due to limitation of protocol or access control.
     * \return success if any information could be retrieved
     */
    virtual DataStatus Stat(FileInfo& file, DataPointInfoType verb = INFO_TYPE_ALL) = 0;

    /// Retrieve information about several DataPoints.
    /**
     * If a DataPoint represents a directory or something similar,
     * information about the object itself and not its contents will
     * be obtained. This method can use bulk operations if the protocol
     * supports it. The protocols and hosts of all the DataPoints in
     * urls must be the same and the same as this DataPoint's protocol
     * and host. This method can be called on any of the urls, for
     * example urls.front()->Stat(files, urls);
     * Calling this method with an empty list of urls returns success if
     * the protocol supports bulk Stat, and an error if it does not and this
     * can be used as a check for bulk support.
     * \param files will contain objects' names and requested attributes.
     * There may be more attributes than requested. There may be less
     * if objects can't provide particular information. The order of this
     * list matches the order of urls. If a stat of any url fails then
     * the corresponding FileInfo in this list will evaluate to false.
     * \param urls list of DataPoints to stat. Protocols and hosts must
     * match and match this DataPoint's protocol and host.
     * \param verb defines attribute types which method must try to
     * retrieve. It is not a failure if some attributes could not
     * be retrieved due to limitation of protocol or access control.
     * \return success if any information could be retrieved for any DataPoint
     */
    virtual DataStatus Stat(std::list<FileInfo>& files,
                            const std::list<DataPoint*>& urls,
                            DataPointInfoType verb = INFO_TYPE_ALL) = 0;

    /// List hierarchical content of this object.
    /**
     * If the DataPoint represents a directory or something similar its
     * contents will be listed and put into files. If the DataPoint is file-
     * like an error will be returned.
     * \param files will contain list of file names and requested
     * attributes. There may be more attributes than requested. There
     * may be less if object can't provide particular information.
     * \param verb defines attribute types which method must try to
     * retrieve. It is not a failure if some attributes could not
     * be retrieved due to limitation of protocol or access control.
     * \return success if DataPoint is a directory-like object and could be
     * listed.
     */
    virtual DataStatus List(std::list<FileInfo>& files, DataPointInfoType verb = INFO_TYPE_ALL) = 0;

    /// Create a directory.
    /**
     * If the protocol supports it, this method creates the last directory
     * in the path to the URL. It assumes the last component of the path is a
     * file-like object and not a directory itself, unless the path ends in a
     * directory separator. If with_parents is true then all missing parent
     * directories in the path will also be created. The access control on the
     * new directories is protocol-specific and may vary depending on protocol.
     * \param with_parents If true then all missing directories in the path
     * are created
     * \return success if the directory was created
     */
    virtual DataStatus CreateDirectory(bool with_parents=false) = 0;

    /// Rename a URL.
    /**
     * This method renames the file or directory specified in the constructor
     * to the new name specified in newurl. It only performs namespace
     * operations using the paths of the two URLs and in general ignores any
     * differences in protocol and host between them. It is assumed that checks
     * that the URLs are consistent are done by the caller of this method.
     * This method does not do any data transfer and is only implemented for
     * protocols which support renaming as an atomic namespace operation.
     * \param newurl The new name for the URL
     * \return success if the object was renamed
     */
    virtual DataStatus Rename(const URL& newurl) = 0;

    /// Allow/disallow DataPoint to read data out of order.
    /**
     * If set to true then data may be read from source out of order or in
     * parallel from multiple threads. For a transfer between two DataPoints
     * this should only be set to true if WriteOutOfOrder() returns true for
     * the destination. Only certain protocols support this option.
     * \param v true if allowed (default is false).
     */
    virtual void ReadOutOfOrder(bool v) = 0;

    /// Returns true if DataPoint supports receiving data out of order during writing.
    virtual bool WriteOutOfOrder() const = 0;

    /// Allow/disallow additional checks on a source DataPoint before transfer
    /**
     * If set to true, extra checks will be performed in DataMover::Transfer()
     * before data transfer starts on for example existence of the source file
     * (and probably other checks too).
     * \param v true if allowed (default is true). */
    virtual void SetAdditionalChecks(bool v) = 0;

    /// Returns true unless SetAdditionalChecks() was set to false.
    virtual bool GetAdditionalChecks() const = 0;

    /// Allow/disallow heavy security (data encryption) during data transfer.
    /**
     * \param v true if allowed (default depends on protocol).
     */
    virtual void SetSecure(bool v) = 0;

    /// Returns true if heavy security during data transfer is allowed.
    virtual bool GetSecure() const = 0;

    /// Set passive transfers for FTP-like protocols.
    /**
     * \param v true if passive should be used.
     */
    virtual void Passive(bool v) = 0;

    /// Returns reason of transfer failure, as reported by callbacks.
    /**
     * This could be different from the failure returned by the methods themselves.
     */
    virtual DataStatus GetFailureReason(void) const;

    /// Set range of bytes to retrieve.
    /**
     * Default values correspond to whole file. Both start and end bytes are
     * included in the range, i.e. start - end + 1 bytes will be read.
     * \param start byte to start from
     * \param end byte to end at
     */
    virtual void Range(unsigned long long int start = 0,
                       unsigned long long int end = 0) = 0;

    /// Resolves index service URL into list of ordinary URLs.
    /**
     * Also obtains meta information about the file if possible. Resolve should
     * be called for both source and destination URLs before a transfer. If
     * source is true an error is returned if the file does not exist.
     * \param source true if DataPoint object represents source of information.
     * \return success if DataPoint was successfully resolved
     */
    virtual DataStatus Resolve(bool source) = 0;

    /// Resolves several index service URLs.
    /**
     * Can use bulk calls if protocol allows. The protocols and hosts of all
     * the DataPoints in urls must be the same and the same as this DataPoint's
     * protocol and host. This method can be called on any of the urls, for
     * example urls.front()->Resolve(true, urls);
     * \param source true if DataPoint objects represent source of information
     * \param urls List of DataPoints to resolve. Protocols and hosts must
     * match and match this DataPoint's protocol and host.
     * \return success if any DataPoint was successfully resolved
     */
    virtual DataStatus Resolve(bool source, const std::list<DataPoint*>& urls) = 0;

    /// Returns true if file is registered in indexing service (only known after Resolve())
    virtual bool Registered() const = 0;

    /// Index service pre-registration.
    /**
     * This function registers the physical location of a file into an indexing
     * service. It should be called *before* the actual transfer to that
     * location happens.
     * \param replication if true, the file is being replicated between two
     * locations registered in the indexing service under the same name.
     * \param force if true, perform registration of a new file even if it
     * already exists. Should be used to fix failures in indexing service.
     * \return success if pre-registration succeeded
     */
    virtual DataStatus PreRegister(bool replication, bool force = false) = 0;

    /// Index service post-registration.
    /**
     * Used for same purpose as PreRegister. Should be called after actual
     * transfer of file successfully finished to finalise registration in an
     * index service.
     * \param replication if true, the file is being replicated between two
     * locations registered in Indexing Service under the same name.
     * \return success if post-registration succeeded
     */
    virtual DataStatus PostRegister(bool replication) = 0;

    /// Index service pre-unregistration.
    /**
     * Should be called if file transfer failed. It removes changes made
     * by PreRegister().
     * \param replication if true, the file is being replicated between two
     * locations registered in Indexing Service under the same name.
     * \return success if pre-unregistration succeeded
     */
    virtual DataStatus PreUnregister(bool replication) = 0;

    /// Index service unregistration.
    /**
     * Remove information about file registered in indexing service.
     * \param all if true, information about file itself is (LFN) is
     * removed. Otherwise only particular physical instance in
     * CurrentLocation() is unregistered.
     * \return success if unregistration succeeded
     */
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

    /// Default checksum type (varies by protocol)
    virtual const std::string DefaultCheckSum() const;

    /// Check if meta-information 'modification time' is available.
    virtual bool CheckModified() const;

    /// Set value of meta-information 'modification time'.
    virtual void SetModified(const Time& val);

    /// Get value of meta-information 'modification time'.
    virtual const Time& GetModified() const;

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

    /// Returns true if file is readonly.
    virtual bool ReadOnly() const = 0;

    /// Returns number of retries left.
    virtual int GetTries() const;

    /// Set number of retries.
    virtual void SetTries(const int n);

    /// Decrease number of retries left.
    virtual void NextTry();

    /// Returns true if some kind of credentials are needed to use this DataPoint
    virtual bool RequiresCredentials() const;

    /// Returns true if credentials must be stored in files.
    /**
     * Some protocols require that credentials exist in files (in which case
     * this method returns true), whereas others can use credentials in memory
     * (then this method returns false).
     * \since Added in 4.0.0.
     */
    virtual bool RequiresCredentialsInFile() const;

    /// Check if URL is an Indexing Service.
    virtual bool IsIndex() const = 0;

    /// Check if URL should be staged or queried for Transport URL (TURL)
    virtual bool IsStageable() const;

    /// Returns true if DataPoint supports internal transfer
    virtual bool SupportsTransfer() const;

    /// Check if endpoint can have any use from meta information.
    virtual bool AcceptsMeta() const = 0;

    /// Check if endpoint can provide at least some meta information directly.
    virtual bool ProvidesMeta() const = 0;

    /// Copy meta information from another object.
    /**
     * Already defined values are not overwritten.
     * \param p object from which information is taken.
     */
    virtual void SetMeta(const DataPoint& p);

    /// Reset meta information to default (undefined) values.
    /**
     * \param p object from which information is taken.
     */
    virtual void ResetMeta();

    /// Compare meta information from another object.
    /**
     * Undefined values are not used for comparison.
     * \param p object to which to compare.
     */
    virtual bool CompareMeta(const DataPoint& p) const;

    /// Returns physical file(s) to read/write, if different from CurrentLocation()
    /**
     * To be used with protocols which re-direct to different URLs such as
     * Transport URLs (TURLs). The list is initially filled by PrepareReading
     * and PrepareWriting. If this list is non-empty then real transfer
     * should use a URL from this list. It is up to the caller to choose the
     * best URL and instantiate new DataPoint for handling it.
     * For consistency protocols which do not require redirections return
     * original URL.
     * For protocols which need redirection calling StartReading and StartWriting
     * will use first URL in the list. */
    virtual std::vector<URL> TransferLocations() const;

    /// Clear list of physical file(s) to read/write.
    /**
     * To be used with protocols which re-direct to different URLs such as
     * Transport URLs (TURLs). Has no effect for other protocols.
     * \since Added in 4.0.0.
     */
    virtual void ClearTransferLocations() {};

    /// Returns current (resolved) URL.
    virtual const URL& CurrentLocation() const = 0;

    /// Returns meta information used to create current URL.
    /**
     * Usage differs between different indexing services.
     */
    virtual const std::string& CurrentLocationMetadata() const = 0;

    /// Returns a pointer to the DataPoint representing the current location.
    virtual DataPoint* CurrentLocationHandle() const = 0;

    /// Compare metadata of DataPoint and current location.
    /**
     * \return inconsistency error or error encountered during operation, or
     * success
     */
    virtual DataStatus CompareLocationMetadata() const = 0;
    
    /// Switch to next location in list of URLs.
    /**
     * At last location switch to first if number of allowed retries is not
     * exceeded.
     * \return false if no retries left.
     */
    virtual bool NextLocation() = 0;

    /// Returns false no more locations are left and out of retries.
    virtual bool LocationValid() const = 0;

    /// Returns true if the current location is the last.
    virtual bool LastLocation() = 0;

    /// Returns true if number of resolved URLs is not 0.
    virtual bool HaveLocations() const = 0;

    /// Add URL representing physical replica to list of locations.
    /**
     * \param url Location URL to add.
     * \param meta Location meta information.
     * \return LocationAlreadyExistsError if location already exists, otherwise
     * success
     */
    virtual DataStatus AddLocation(const URL& url,
                                   const std::string& meta) = 0;

    /// Remove current URL from list
    virtual DataStatus RemoveLocation() = 0;

    /// Remove locations present in another DataPoint object
    virtual DataStatus RemoveLocations(const DataPoint& p) = 0;

    /// Remove all locations
    virtual DataStatus ClearLocations() = 0;

    /// Add a checksum object which will compute checksum during data transfer.
    /**
     * \param cksum object which will compute checksum. Should not be destroyed
     * until DataPointer itself.
     * \return integer position in the list of checksum objects.
     */
    virtual int AddCheckSumObject(CheckSum *cksum) = 0;

    /// Get CheckSum object at given position in list
    virtual const CheckSum* GetCheckSumObject(int index) const = 0;

    /// Sort locations according to the specified pattern and URLMap.
    /**
     * See DataMover::set_preferred_pattern for a more detailed explanation of
     * pattern matching. Locations present in url_map are preferred over
     * others.
     * \param pattern a set of strings, separated by |, to match against.
     * \param url_map map of URLs to local URLs
     */
    virtual void SortLocations(const std::string& pattern,
                               const URLMap& url_map) = 0;

    /// Add URL options to this DataPoint's URL object.
    /**
     * Invalid options for the specific DataPoint instance will not be added.
     * \param options map of option, value pairs
     */
    virtual void AddURLOptions(const std::map<std::string, std::string>& options);

  protected:
    /// URL supplied in constructor.
    URL url;
    /// UserConfig supplied in constructor.
    const UserConfig usercfg;

    // attributes
    /// Size of object represented by DataPoint.
    unsigned long long int size;
    /// Checksum of object represented by DataPoint.
    std::string checksum;
    /// Modification time of object represented by DataPoint.
    Time modified;
    /// Validity time of object represented by DataPoint.
    Time valid;
    /// Access latency of object represented by DataPoint.
    DataPointAccessLatency access_latency;
    /// Retries left for data transfer.
    int triesleft;
    /// Result of data read/write carried out in separate thread.
    DataStatus failure_code; /* filled by callback methods */
    /// Whether this DataPoint is cacheable.
    bool cache;
    /// Whether this DataPoint requires staging.
    bool stageable;
    /// Valid URL options. Subclasses should add their own specific options to this list.
    std::set<std::string> valid_url_options;

    /// Logger object
    static Logger logger;

    /// Constructor.
    /**
     * Constructor is protected because DataPoints should not be created
     * directly. Subclasses should however call this in their constructors to
     * set various common attributes.
     * \param url The URL representing the DataPoint
     * \param usercfg User configuration object
     * \param parg plugin argument
     */
    DataPoint(const URL& url, const UserConfig& usercfg, PluginArgument* parg);

    /// Perform third party transfer.
    /**
     * This method is protected because the static version should be used
     * instead to load the correct DMC plugin for third party transfer.
     * \param source Source URL to pull data from
     * \param destination Destination URL which pulls data to itself
     * \param callback Optional monitoring callback
     * \return outcome of transfer
     */
    virtual DataStatus Transfer3rdParty(const URL& source, const URL& destination, TransferCallback callback = NULL);
  };

  /** \cond Class used by DataHandle to load the required DMC. */
  class DataPointLoader
    : public Loader {
  private:
    DataPointLoader();
    ~DataPointLoader();
    DataPoint* load(const URL& url, const UserConfig& usercfg);
    friend class DataHandle;
  };
  /** \endcond */

  /** \cond Class representing the arguments passed to DMC plugins. */
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
  /** \endcond */

} // namespace Arc

#endif // __ARC_DATAPOINT_H__
