// -*- indent-tabs-mode: nil -*-

#ifndef __HTTPSD_SRM_CLIENT_H__
#define __HTTPSD_SRM_CLIENT_H__

#include <string>
#include <list>
#include <exception>

#include <arc/DateTime.h>
#include <arc/Logger.h>
#include <arc/UserConfig.h>
#include <arc/message/MCC.h>
#include <arc/client/ClientInterface.h>

#include "SRMURL.h"

namespace Arc {

  /**
   * The version of the SRM protocol
   */
  enum SRMVersion {
    SRM_V1,
    SRM_V2_2,
    SRM_VNULL
  };

  /**
   * Return code specifying types of errors that can occur in client methods
   */
  enum SRMReturnCode {
    SRM_OK,
    SRM_ERROR_CONNECTION,
    SRM_ERROR_SOAP,
    // the next two only apply to valid repsonses from the service
    SRM_ERROR_TEMPORARY, // eg SRM_INTERNAL_ERROR, SRM_FILE_BUSY
    SRM_ERROR_PERMANENT, // eg no such file, permission denied
    SRM_ERROR_NOT_SUPPORTED, // not supported by this version of the protocol
    SRM_ERROR_OTHER // eg bad input parameters, unexpected result format
  };

  /**
   * Specifies whether file is on disk or only on tape
   */
  enum SRMFileLocality {
    SRM_ONLINE,
    SRM_NEARLINE,
    SRM_UNKNOWN,
    SRM_STAGE_ERROR
  };

  /**
   * Quality of retention
   */
  enum SRMRetentionPolicy {
    SRM_REPLICA,
    SRM_OUTPUT,
    SRM_CUSTODIAL,
    SRM_RETENTION_UNKNOWN
  };

  /**
   * The lifetime of the file
   */
  enum SRMFileStorageType {
    SRM_VOLATILE,
    SRM_DURABLE,
    SRM_PERMANENT,
    SRM_FILE_STORAGE_UNKNOWN
  };

  /**
   * File, directory or link
   */
  enum SRMFileType {
    SRM_FILE,
    SRM_DIRECTORY,
    SRM_LINK,
    SRM_FILE_TYPE_UNKNOWN
  };

  /**
   * Implementation of service. Found from srmPing (v2.2 only)
   */
  enum SRMImplementation {
    SRM_IMPLEMENTATION_DCACHE,
    SRM_IMPLEMENTATION_CASTOR,
    SRM_IMPLEMENTATION_DPM,
    SRM_IMPLEMENTATION_STORM,
    SRM_IMPLEMENTATION_UNKNOWN
  };

  /**
   * File metadata
   */
  struct SRMFileMetaData {
    std::string path;   // absolute dir and file path
    long long int size;
    Time createdAtTime;
    Time lastModificationTime;
    std::string checkSumType;
    std::string checkSumValue;
    SRMFileLocality fileLocality;
    SRMRetentionPolicy retentionPolicy;
    SRMFileStorageType fileStorageType;
    SRMFileType fileType;
    std::list<std::string> spaceTokens;
    std::string owner;
    std::string group;
    std::string permission;
    Period lifetimeLeft; // on the SURL
    Period lifetimeAssigned;
  };

  class SRMInvalidRequestException
    : public std::exception {};

  /**
   * The status of a request
   */
  enum SRMRequestStatus {
    SRM_REQUEST_CREATED,
    SRM_REQUEST_ONGOING,
    SRM_REQUEST_FINISHED_SUCCESS,
    SRM_REQUEST_FINISHED_PARTIAL_SUCCESS,
    SRM_REQUEST_FINISHED_ERROR,
    SRM_REQUEST_SHOULD_ABORT,
    SRM_REQUEST_CANCELLED
  };

  /**
   * Class to represent a request which may be used for multiple operations,
   * for example calling getTURLs() sets the request token in the request
   * object (for a v2.2 client) and then same object is passed to releaseGet().
   */
  class SRMClientRequest {

  private:

    /**
     * The SURLs of the files involved in the request, mapped to their locality.
     */
    std::map<std::string, SRMFileLocality> _surls;

    /**
     * int ids are used in SRM1
     */
    int _request_id;

    /**
     * string request tokens (eg "-21249586") are used in SRM2.2
     */
    std::string _request_token;

    /**
     * A list of file ids is kept in SRM1
     */
    std::list<int> _file_ids;

    /**
     * The space token associated with a request
     */
    std::string _space_token;

    /**
     * A map of SURLs for which requests failed to failure reason.
     * Used for bring online requests.
     */
    std::map<std::string, std::string> _surl_failures;

    /**
     * Estimated waiting time as returned by the server to wait
     * until the next poll of an asychronous request.
     */
    int _waiting_time;

    /**
     * status of request. Only useful for asynchronous requests.
     */
    SRMRequestStatus _status;

    /**
     * For operations like getTURLs and putTURLs _request_timeout specifies
     * the timeout for these operations to complete. If it is zero then
     * these operations will act asynchronously, i.e. return and expect
     * the caller to poll for the status. If it is non-zero the operations
     * will block until completed or this timeout has been reached.
     */
    unsigned int _request_timeout;

    /**
     * Total size of all files in request. Can be used when reserving space.
     */
    unsigned long long _total_size;

    /**
     * Whether a detailed listing is requested
     */
    bool _long_list;

    /**
     * Error LogLevel. This can be changed when errors should not be reported
     * at ERROR level
     */
    LogLevel _error_loglevel;

    /**
     * List of requested transport protocols
     */
    std::list<std::string> _transport_protocols;

  public:
    /**
     * Creates a request object with multiple SURLs.
     * The URLs here are in the form
     * srm://srm.ndgf.org/data/atlas/disk/user/user.mlassnig.dataset.1/file3
     */
    SRMClientRequest(const std::list<std::string>& urls)
      throw (SRMInvalidRequestException)
        : _request_id(0),
          _space_token(""),
          _waiting_time(1),
          _status(SRM_REQUEST_CREATED),
          _request_timeout(60),
          _total_size(0),
          _long_list(false),
          _error_loglevel(ERROR) {
      if (urls.empty())
        throw SRMInvalidRequestException();
      for (std::list<std::string>::const_iterator it = urls.begin();
           it != urls.end(); ++it)
        _surls[*it] = SRM_UNKNOWN;
    };
    
    /**
     * Creates a request object with a single SURL.
     * The URL here are in the form
     * srm://srm.ndgf.org/data/atlas/disk/user/user.mlassnig.dataset.1/file3
     */
    SRMClientRequest(const std::string& url="", const std::string& id="")
      throw (SRMInvalidRequestException)
        : _request_id(0),
          _space_token(""),
          _waiting_time(1),
          _status(SRM_REQUEST_CREATED),
          _request_timeout(60),
          _total_size(0),
          _long_list(false),
          _error_loglevel(ERROR) {
      if (url.empty() && id.empty())
        throw SRMInvalidRequestException();
      if (!url.empty())
        _surls[url] = SRM_UNKNOWN;
      else
        _request_token = id;
    }

    /**
     * set and get request id
     */
    void request_id(int id) {
      _request_id = id;
    }
    int request_id() const {
      return _request_id;
    }

    /**
     * set and get request token
     */
    void request_token(const std::string& token) {
      _request_token = token;
    }
    std::string request_token() const {
      return _request_token;
    }

    /**
     * set and get file id list
     */
    void file_ids(const std::list<int>& ids) {
      _file_ids = ids;
    }
    std::list<int> file_ids() const {
      return _file_ids;
    }

    /**
     * set and get space token
     */
    void space_token(const std::string& token) {
      _space_token = token;
    }
    std::string space_token() const {
      return _space_token;
    }

    /**
     * get SURLs
     */
    std::list<std::string> surls() const {
      std::list<std::string> surl_list;
      for (std::map<std::string, SRMFileLocality>::const_iterator it =
             _surls.begin(); it != _surls.end(); ++it)
        surl_list.push_back(it->first);
      return surl_list;
    }

    /**
     * set and get surl statuses
     */
    void surl_statuses(const std::string& surl, SRMFileLocality locality) {
      _surls[surl] = locality;
    }
    std::map<std::string, SRMFileLocality> surl_statuses() const {
      return _surls;
    }

    /**
     * set and get surl failures
     */
    void surl_failures(const std::string& surl, const std::string& reason) {
      _surl_failures[surl] = reason;
    }
    std::map<std::string, std::string> surl_failures() const {
      return _surl_failures;
    }

    /**
     * set and get waiting time. A waiting time of zero means no estimate was given
     * by the remote service.
     */
    void waiting_time(int wait_time) {
      _waiting_time = wait_time;
    }
    int waiting_time() const {
      return _waiting_time;
    }

    /**
     * set and get status of request
     */
    void finished_success() {
      _status = SRM_REQUEST_FINISHED_SUCCESS;
    }
    void finished_partial_success() {
      _status = SRM_REQUEST_FINISHED_PARTIAL_SUCCESS;
    }
    void finished_error() {
      _status = SRM_REQUEST_FINISHED_ERROR;
    }
    void finished_abort() {
      _status = SRM_REQUEST_SHOULD_ABORT;
    }
    void wait(int t = 0) {
      _status = SRM_REQUEST_ONGOING;
      _waiting_time = t;
    }
    void cancelled() {
      _status = SRM_REQUEST_CANCELLED;
    }
    SRMRequestStatus status() const {
      return _status;
    }

    /**
     * set and get request timeout
     */
    void request_timeout(unsigned int timeout) { _request_timeout = timeout; };
    unsigned int request_timeout() const { return _request_timeout; };

    /**
     * set and get total size
     */
    void total_size(unsigned long long size) { _total_size = size; };
    unsigned long long total_size() const { return _total_size; };

    /**
     * set and get long list flag
     */
    void long_list(bool list) {
      _long_list = list;
    }
    bool long_list() const {
      return _long_list;
    }

    /**
     * set and get error log level
     */
    void error_loglevel(LogLevel level) {
      _error_loglevel = level;
    }
    LogLevel error_loglevel() const {
      return _error_loglevel;
    }

    /**
     * set and get transport protocols
     */
    void transport_protocols(const std::list<std::string>& protocols) {
      _transport_protocols = protocols;
    }
    std::list<std::string> transport_protocols() const {
      return _transport_protocols;
    }
  };

  /**
   * A client interface to the SRM protocol. Instances of SRM clients
   * are created by calling the getInstance() factory method. One client
   * instance can be used to make many requests to the same server (with
   * the same protocol version), but not multiple servers.
   */
  class SRMClient {

  protected:

    /**
     * The URL of the service endpoint, eg
     * httpg://srm.ndgf.org:8443/srm/managerv2
     * All SURLs passed to methods must correspond to this endpoint.
     */
    std::string service_endpoint;

    /**
     * SOAP configuraton object
     */
    MCCConfig cfg;

    /**
     * SOAP client object
     */
    ClientSOAP *client;

    /**
     * SOAP namespace
     */
    NS ns;

    /**
     * The implementation of the server
     */
    SRMImplementation implementation;

    /**
     * Timeout for requests to the SRM service
     */
    time_t user_timeout;

    /**
     * The version of the SRM protocol used
     */
    std::string version;

    /**
     * Logger
     */
    static Logger logger;

    /**
     * Constructor
     */
    SRMClient(const UserConfig& usercfg, const SRMURL& url);

    /**
     * Process SOAP request
     */
    SRMReturnCode process(const std::string& action, PayloadSOAP *request, PayloadSOAP **response);

  public:
    /**
     * Returns an SRMClient instance with the required protocol version.
     * This must be used to create SRMClient instances. Specifying a
     * version explicitly forces creation of a client with that version.
     * @param usercfg The user configuration.
     * @param url A SURL. A client connects to the service host derived from
     * this SURL. All operations with a client instance must use SURLs with
     * the same host as this one.
     * @param timedout Whether the connection timed out
     * @param conn_timeout Connection timeout to the SRM service
     * @returns A pointer to an instance of SRMClient is returned, or NULL if
     * it was not possible to create one.
     */
    static SRMClient* getInstance(const UserConfig& usercfg,
                                  const std::string& url,
                                  bool& timedout);

    /**
     * Destructor
     */
    virtual ~SRMClient();

    /**
     * Returns the version of the SRM protocol used by this instance
     */
    std::string getVersion() const {
      return version;
    }

    /**
     * Find out the version supported by the server this client
     * is connected to. Since this method is used to determine
     * which client version to instantiate, we may not want to
     * report an error to the user, so setting report_error to
     * false supresses the error message.
     * @param version The version returned by the server
     * @param report_error Whether an error should be reported
     * @returns SRMReturnCode specifying outcome of operation
     */
    virtual SRMReturnCode ping(std::string& version,
                               bool report_error = true) = 0;

    /**
     * Find the space tokens available to write to which correspond to
     * the space token description, if given. The list of tokens is
     * a list of numbers referring to the SRM internal definition of the
     * spaces, not user-readable strings.
     * @param tokens The list filled by the service
     * @param description The space token description
     * @returns SRMReturnCode specifying outcome of operation
     */
    virtual SRMReturnCode getSpaceTokens(std::list<std::string>& tokens,
                                         const std::string& description = "") = 0;

    /**
     * Returns a list of request tokens for the user calling the method
     * which are still active requests, or the tokens corresponding to the
     * token description, if given.
     * @param tokens The list filled by the service
     * @param description The user request description, which can be specified
     * when the request is created
     * @returns SRMReturnCode specifying outcome of operation
     */
    virtual SRMReturnCode getRequestTokens(std::list<std::string>& tokens,
                                           const std::string& description = "") = 0;


    /**
     * If the user wishes to copy a file from somewhere, getTURLs() is called
     * to retrieve the transport URL(s) to copy the file from. It may be used
     * synchronously or asynchronously, depending on the synchronous property
     * of the request object. In the former case it will block until
     * the TURLs are ready, in the latter case it will return after making the
     * request and getTURLsStatus() must be used to poll the request status if
     * it was not completed.
     * @param req The request object
     * @param urls A list of TURLs filled by the method
     * @returns SRMReturnCode specifying outcome of operation
     */
    virtual SRMReturnCode getTURLs(SRMClientRequest& req,
                                   std::list<std::string>& urls) = 0;

    /**
     * In the case where getTURLs was called asynchronously and the request
     * was not completed, this method should be called to poll the status of
     * the request. getTURLs must be called before this method and the request
     * object must have ongoing request status.
     * @param req The request object. Status must be ongoing.
     * @param urls A list of TURLs filled by the method if the request
     * completed successfully
     * @returns SRMReturnCode specifying outcome of operation
     */
    virtual SRMReturnCode getTURLsStatus(SRMClientRequest& req,
                                         std::list<std::string>& urls) = 0;

    /**
     * Submit a request to bring online files. If the synchronous property
     * of the request object is false, this operation is asynchronous and
     * the status of the request can be checked by calling
     * requestBringOnlineStatus() with the request token in req
     * which is assigned by this method. If the request is synchronous, this
     * operation blocks until the file(s) are online or the timeout specified
     * in the SRMClient constructor has passed.
     * @param req The request object
     * @returns SRMReturnCode specifying outcome of operation
     */
    virtual SRMReturnCode requestBringOnline(SRMClientRequest& req) = 0;

    /**
     * Query the status of a request to bring files online. The SURLs map
     * of the request object is updated if the status of any files in the
     * request has changed. requestBringOnline() but be called before
     * this method.
     * @param req The request object to query the status of
     * @returns SRMReturnCode specifying outcome of operation
     */
    virtual SRMReturnCode requestBringOnlineStatus(SRMClientRequest& req) = 0;

    /**
     * If the user wishes to copy a file to somewhere, putTURLs() is called
     * to retrieve the transport URL(s) to copy the file to. It may be used
     * synchronously or asynchronously, depending on the synchronous property
     * of the request object. In the former case it will block until
     * the TURLs are ready, in the latter case it will return after making the
     * request and putTURLsStatus() must be used to poll the request status if
     * it was not completed.
     * @param req The request object
     * @param urls A list of TURLs filled by the method
     * @returns SRMReturnCode specifying outcome of operation
     */
    virtual SRMReturnCode putTURLs(SRMClientRequest& req,
                                   std::list<std::string>& urls) = 0;

    /**
     * In the case where putTURLs was called asynchronously and the request
     * was not completed, this method should be called to poll the status of
     * the request. putTURLs must be called before this method and the request
     * object must have ongoing request status.
     * @param req The request object. Status must be ongoing.
     * @param urls A list of TURLs filled by the method if the request
     * completed successfully
     * @returns SRMReturnCode specifying outcome of operation
     */
    virtual SRMReturnCode putTURLsStatus(SRMClientRequest& req,
                                         std::list<std::string>& urls) = 0;

    /**
     * Should be called after a successful copy from SRM storage.
     * @param req The request object
     * @returns SRMReturnCode specifying outcome of operation
     */
    virtual SRMReturnCode releaseGet(SRMClientRequest& req) = 0;

    /**
     * Should be called after a successful copy to SRM storage.
     * @param req The request object
     * @returns SRMReturnCode specifying outcome of operation
     */
    virtual SRMReturnCode releasePut(SRMClientRequest& req) = 0;

    /**
     * Used in SRM v1 only. Called to release files after successful transfer.
     * @param req The request object
     * @returns SRMReturnCode specifying outcome of operation
     */
    virtual SRMReturnCode release(SRMClientRequest& req) = 0;

    /**
     * Called in the case of failure during transfer or releasePut. Releases
     * all TURLs involved in the transfer.
     * @param req The request object
     * @returns SRMReturnCode specifying outcome of operation
     */
    virtual SRMReturnCode abort(SRMClientRequest& req) = 0;

    /**
     * Returns information on a file or files (v2.2 and higher)
     * stored in an SRM, such as file size, checksum and
     * estimated access latency.
     * @param req The request object
     * @param metadata A list of structs filled with file information
     * @param recursive The level of recursion into sub directories
     * @param report_error Determines if errors should be reported
     * @returns SRMReturnCode specifying outcome of operation
     * @see SRMFileMetaData
     */
    virtual SRMReturnCode info(SRMClientRequest& req,
                               std::list<struct SRMFileMetaData>& metadata,
                               const int recursive = 0) = 0;

    /**
     * Delete a file physically from storage and the SRM namespace.
     * @param req The request object
     * @returns SRMReturnCode specifying outcome of operation
     */
    virtual SRMReturnCode remove(SRMClientRequest& req) = 0;

    /**
     * Copy a file between two SRM storages.
     * @param req The request object
     * @param source The source SURL
     * @returns SRMReturnCode specifying outcome of operation
     */
    virtual SRMReturnCode copy(SRMClientRequest& req,
                               const std::string& source) = 0;

    /**
     * Make required directories for the SURL in the request
     * @param req The request object
     * @returns SRMReturnCode specifying outcome of operation
     */
    virtual SRMReturnCode mkDir(SRMClientRequest& req) = 0;

    /**
     * Check permissions for the SURL in the request using the
     * current credentials.
     * @oaram req The request object
     * @returns SRMReturnCode specifying outcome of operation
     */
    virtual SRMReturnCode checkPermissions(SRMClientRequest& req) = 0;

    operator bool() const {
      return client;
    }
    bool operator!() const {
      return !client;
    }
  };

} // namespace Arc

#endif // __HTTPSD_SRM_CLIENT_H__
