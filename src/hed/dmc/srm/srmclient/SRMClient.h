#ifndef __HTTPSD_SRM_CLIENT_H__
#define __HTTPSD_SRM_CLIENT_H__

#include <string>
#include <sstream>
#include <list>
#include <exception>

#include <arc/Logger.h>

#include "../https/HTTPSClient.h"

#include "SRMURL.h"

//namespace Arc {
  
  /**
   * The version of the SRM protocol
   */
  enum SRMVersion {
    SRM_V1,
    SRM_V2_2,
    SRM_VNULL,
  };
  
  /**
   * Return code specifying types of errors that can occur in client methods
   */
  enum SRMReturnCode {
    SRM_OK,
    SRM_ERROR_CONNECTION,
    SRM_ERROR_SOAP,
    SRM_ERROR_NOT_SUPPORTED,
    SRM_ERROR_OTHER,
  };
  
  /**
   * Specifies whether file is on disk or only on tape
   */
  enum SRMFileLocality {
    SRM_ONLINE,
    SRM_NEARLINE,
    SRM_UNKNOWN,
    SRM_STAGE_ERROR,
  };
   
  /**
   * Quality of retention
   */
  enum SRMRetentionPolicy {
    SRM_REPLICA,
    SRM_OUTPUT,
    SRM_CUSTODIAL,
    SRM_RETENTION_UNKNOWN,
  };
  
  /**
   * The lifetime of the file
   */
  enum SRMFileStorageType {
    SRM_VOLATILE,
    SRM_DURABLE,
    SRM_PERMANENT,
    SRM_FILE_STORAGE_UNKNOWN,
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
    SRM_IMPLEMENTATION_UNKNOWN,
  };
  
  /**
   * File metadata
   */
  struct SRMFileMetaData {
    std::string path;   // absolute dir and file path
    long long int size;
    time_t createdAtTime;
    time_t lastModificationTime;
    std::string checkSumType;
    std::string checkSumValue;
    SRMFileLocality fileLocality;
    SRMRetentionPolicy retentionPolicy;
    SRMFileStorageType fileStorageType;
    SRMFileType fileType;
    std::string arrayOfSpaceTokens;
    std::string owner;
    std::string group;
    std::string permission;
    int lifetimeLeft; // on the SURL    
    int lifetimeAssigned;
  };
  
  class SRMInvalidRequestException: public std::exception {};
  
  /**
   * The status of a request
   */
  enum SRMRequestStatus {
    SRM_REQUEST_ONGOING,
    SRM_REQUEST_FINISHED_SUCCESS,
    SRM_REQUEST_FINISHED_PARTIAL_SUCCESS,
    SRM_REQUEST_FINISHED_ERROR,
    SRM_REQUEST_CANCELLED,
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
     * Whether a detailed listing is requested
     */
    bool _long_list;
    
   public:
    
    /**
     * Creates a request object with multiple SURLs. The URLs here are in the form
     * srm://srm.ndgf.org/pnfs/ndgf.org/data/atlas/disk/user/user.mlassnig.dataset.1/dummyfile3
     */
    SRMClientRequest(std::list<std::string> urls) 
      throw (SRMInvalidRequestException):
      _request_token(""),
      _space_token(""), 
      _waiting_time(1), 
      _status(SRM_REQUEST_ONGOING),
      _long_list(false) {
  
      if(urls.empty()) throw SRMInvalidRequestException();
      for (std::list<std::string>::iterator i = urls.begin();
         i != urls.end();
         ++i) _surls[*i] = SRM_UNKNOWN;
    };
    
    /**
     * Creates a request object with a single SURL. The URL here are in the form
     * srm://srm.ndgf.org/pnfs/ndgf.org/data/atlas/disk/user/user.mlassnig.dataset.1/dummyfile3
     */
    SRMClientRequest(std::string url="", std::string id="")
      throw (SRMInvalidRequestException):
      _request_token(""),
      _space_token(""),
      _waiting_time(1),
      _status(SRM_REQUEST_ONGOING),
      _long_list(false) {
        
      if(url.compare("") == 0 && id.compare("") == 0) throw SRMInvalidRequestException();
      if(url.compare("") != 0) _surls[url] = SRM_UNKNOWN;
      else _request_token = (char*)id.c_str();
    };
  
    /**
     * set and get request id
     */
    void request_id(int id) {_request_id = id;};
    int request_id() {return _request_id;};
    
    /**
     * set and get request token
     */
    void request_token(char* token) {_request_token = token;};
    std::string request_token() {return _request_token;};
    
    /**
     * set and get file id list
     */
    void file_ids(std::list<int> ids) {_file_ids = ids;};
    std::list<int> file_ids() {return _file_ids;};
  
    /**
     * set and get space token
     */
    void space_token(std::string token) {_space_token = token;};
    std::string space_token() {return _space_token;};
  
    /**
     * get SURLs
     */
    std::list<std::string> surls() {
      std::list<std::string> surl_list;
      for (std::map<std::string,SRMFileLocality>::iterator it=_surls.begin() ; it != _surls.end(); it++ ) {
        surl_list.push_back((*it).first);
      };
      return surl_list;
    };
  
    /**
     * set and get surl statuses
     */
    void surl_statuses(std::string surl, SRMFileLocality locality) {_surls[surl] = locality;};
    std::map<std::string, SRMFileLocality> surl_statuses() {return _surls;};
  
    /**
     * set and get surl failures
     */
    void surl_failures(std::string surl, std::string reason) {_surl_failures[surl] = reason;};
    std::map<std::string, std::string> surl_failures() {return _surl_failures;};
  
    /**
     * set and get waiting time
     */
    void waiting_time(int wait_time) {_waiting_time = wait_time;};
    int waiting_time() {return _waiting_time;};
  
    /**
     * set and get status of request
     */
    void finished_success() {_status = SRM_REQUEST_FINISHED_SUCCESS;};
    void finished_partial_success() {_status = SRM_REQUEST_FINISHED_PARTIAL_SUCCESS;};
    void finished_error() {_status = SRM_REQUEST_FINISHED_ERROR;};
    void cancelled() {_status = SRM_REQUEST_CANCELLED;};
    SRMRequestStatus status() {return _status;};
    
    /**
     * set and get long list flag
     */
    void long_list(bool list) { _long_list = list; };
    bool long_list() { return _long_list; };
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
     * The URL of the service endpoint, eg httpg://srm.ndgf.org:8443/srm/managerv2
     * All SURLs passed to methods must correspond to this endpoint.
     */
    std::string service_endpoint;
  
    /**
     * SOAP client object
     */
    Arc::HTTPSClientSOAP * csoap;
  
    /**
     * The implementation of the server
     */
    SRMImplementation implementation;
  
    /**
     * Timeout for requests to the SRM service
     */
    static time_t request_timeout;
  
    /**
     * The version of the SRM protocol used
     */
    std::string version;
    
    /**
     * Logger
     */
    static Arc::Logger logger;
  
   public:
  
    /**
     * Establish a connection to the service
     */
    bool connect(void) { if(!csoap) return false; return (csoap->connect() == 0);};
  
    /**
     * Disconnect from the service and destroy the connection
     */
    bool disconnect(void) { if(!csoap) return true; return (csoap->disconnect() == 0);};
  
    /**
     * Returns an SRMClient instance with the required protocol version.
     * This must be used to create SRMClient instances. Specifying a
     * version explicitly forces creation of a client with that version.
     * @param url A SURL. A client connects to the service host derived from
     * this SURL. All operations with a client instance must use SURLs with
     * the same host as this one.
     * @param version If this is non-NULL a client instance of this version
     * is returned.
     */
    static SRMClient* getInstance(std::string url,
                                  time_t timeout=300,
                                  SRMVersion srm_version=SRM_VNULL);
  
    /**
     * empty destructor
     */
    virtual ~SRMClient() {};
  
    /**
     * set the request timeout
     */
    void Timeout(int t) { request_timeout=t; };
  
    /**
     * Returns the version of the SRM protocol used by this instance
     */
    std::string getVersion() {return version;};
  
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
     * @returns True on success, false on failure
     */
    virtual SRMReturnCode getSpaceTokens(std::list<std::string>& tokens,
                                         std::string description = "") = 0;
  
    /**
     * Returns a list of request tokens for the user calling the method
     * which are still active requests, or the tokens corresponding to the
     * token description, if given.
     * @param tokens The list filled by the service
     * @param description The user request description, which can be specified
     * when the request is created
     * @returns True on success, false on failure
     */
    virtual SRMReturnCode getRequestTokens(std::list<std::string>& tokens,
                                           std::string description = "") = 0;
  
  
    /**
     * If the user wishes to copy a file from somewhere, getTURLs() is called
     * to retrieve the transport URL to copy the file from.
     * @param req The request object
     * @param urls A list of TURLs filled by the method
     * @returns True on success, false on failure
     */
    virtual bool getTURLs(SRMClientRequest& req,
                          std::list<std::string>& urls) = 0;
  
    /**
     * Submit a request to bring online files. This operation is asynchronous
     * and the status of the request can be checked by calling
     * requestBringOnlineStatus() with the request token in req which is
     * assigned by this method.
     * @param req The request object
     * @returns SRMReturnCode specifying outcome of operation
     */
    virtual SRMReturnCode requestBringOnline(SRMClientRequest& req) = 0;
  
    /**
     * Query the status of a request to bring files online. The SURLs map
     * is updated if the status of any files in the request has changed.
     * @param req The request object to query the status of
     * @returns SRMReturnCode specifying outcome of operation
     */
    virtual SRMReturnCode requestBringOnlineStatus(SRMClientRequest& req) = 0;
  
    /**
     * If the user wishes to copy a file to somewhere, putTURLs() is called
     * to retrieve the transport URL to copy the file to.
     * @param req The request object
     * @param urls A list of TURLs filled by the method
     * @param size The size of the file
     * @returns True on success, false on failure
     */
    virtual bool putTURLs(SRMClientRequest& req,
                          std::list<std::string>& urls,
                          unsigned long long size = 0) = 0;
  
    /**
     * Should be called after a successful copy from SRM storage.
     * @param req The request object
     * @returns True on success, false on failure
     */
    virtual bool releaseGet(SRMClientRequest& req) = 0;
  
    /**
     * Should be called after a successful copy to SRM storage.
     * @param req The request object
     * @returns True on success, false on failure
     */   
    virtual bool releasePut(SRMClientRequest& req) = 0;
  
    /**
     * Used in SRM v1 only. Called to release files after successful transfer.
     * @param req The request object
     * @returns True on success, false on failure
     */   
    virtual bool release(SRMClientRequest& req) = 0;
  
    /**
     * Called in the case of failure during transfer or releasePut. Releases
     * all TURLs involved in the transfer.
     * @param req The request object
     * @returns True on success, false on failure
     */
    virtual bool abort(SRMClientRequest& req) = 0;
  
    /**
     * Returns information on a file or files (v2.2 and higher)
     * stored in an SRM, such as file size, checksum and
     * estimated access latency.
     * @param req The request object
     * @param metadata A list of structs filled with file information
     * @param recursive The level of recursion into sub directories
     * @returns True on success, false on failure
     * @see SRMFileMetaData
     */
    virtual bool info(SRMClientRequest& req,
                      std::list<struct SRMFileMetaData>& metadata,
                      const int recursive = 0) = 0;
  
    /**
     * Delete a file physically from storage and the SRM namespace.
     * @param req The request object
     * @returns True on success, false on failure
     */
    virtual bool remove(SRMClientRequest& req) = 0;
  
    /**
     * Copy a file between two SRM storages.
     * @param req The request object
     * @param source The source SURL
     * @returns True on success, false on failure
     */
    virtual bool copy(SRMClientRequest& req,
                      const std::string& source) = 0;
  
    operator bool(void) { return csoap; };
    bool operator!(void) { return !csoap; };
  };

//} // namespace Arc
  
#endif // __HTTPSD_SRM_CLIENT_H__
