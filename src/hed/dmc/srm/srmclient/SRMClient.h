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
#include <arc/data/DataStatus.h>
#include <arc/communication/ClientInterface.h>

#include "SRMURL.h"
#include "SRMClientRequest.h"

namespace ArcDMCSRM {

  using namespace Arc;

  /// SRM-related file metadata
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


  /**
   * A client interface to the SRM protocol. Instances of SRM clients
   * are created by calling the getInstance() factory method. One client
   * instance can be used to make many requests to the same server (with
   * the same protocol version), but not multiple servers.
   */
  class SRMClient {

  private:

    SRMClient(SRMClient const&);
    SRMClient& operator=(SRMClient const&);

  protected:

    /// URL of the service endpoint, eg httpg://srm.host.org:8443/srm/managerv2
    /// All SURLs passed to methods must correspond to this endpoint.
    std::string service_endpoint;

    /// SOAP configuraton object
    MCCConfig cfg;

    /// SOAP client object
    ClientSOAP *client;

    /// SOAP namespace
    NS ns;

    /// The implementation of the server
    SRMImplementation implementation;

    /// Timeout for requests to the SRM service
    time_t user_timeout;

    /// The version of the SRM protocol used
    std::string version;

    /// Logger
    static Logger logger;

    /// Protected constructor
    SRMClient(const UserConfig& usercfg, const SRMURL& url);

    /// Process SOAP request
    DataStatus process(const std::string& action, PayloadSOAP *request, PayloadSOAP **response);

  public:
    /**
     * Create an SRMClient instance. The instance will be a SRM v2.2 client
     * unless another version is explicitly given in the url.
     * @param usercfg The user configuration.
     * @param url A SURL. A client connects to the service host derived from
     * this SURL. All operations with a client instance must use SURLs with
     * the same host as this one.
     * @param error Details of error if one occurred
     * @returns A pointer to an instance of SRMClient is returned, or NULL if
     * it was not possible to create one.
     */
    static SRMClient* getInstance(const UserConfig& usercfg,
                                  const std::string& url,
                                  std::string& error);

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
     * false suppresses the error message.
     * @param version The version returned by the server
     * @returns DataStatus specifying outcome of operation
     */
    virtual DataStatus ping(std::string& version) = 0;

    /**
     * Find the space tokens available to write to which correspond to
     * the space token description, if given. The list of tokens is
     * a list of numbers referring to the SRM internal definition of the
     * spaces, not user-readable strings.
     * @param tokens The list filled by the service
     * @param description The space token description
     * @returns DataStatus specifying outcome of operation
     */
    virtual DataStatus getSpaceTokens(std::list<std::string>& tokens,
                                      const std::string& description = "") = 0;

    /**
     * Returns a list of request tokens for the user calling the method
     * which are still active requests, or the tokens corresponding to the
     * token description, if given. Was used by the old ngstage command but
     * is currently unused.
     * @param tokens The list filled by the service
     * @param description The user request description, which can be specified
     * when the request is created
     * @returns DataStatus specifying outcome of operation
     */
    virtual DataStatus getRequestTokens(std::list<std::string>& tokens,
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
     * @returns DataStatus specifying outcome of operation
     */
    virtual DataStatus getTURLs(SRMClientRequest& req,
                                std::list<std::string>& urls) = 0;

    /**
     * In the case where getTURLs was called asynchronously and the request
     * was not completed, this method should be called to poll the status of
     * the request. getTURLs must be called before this method and the request
     * object must have ongoing request status.
     * @param req The request object. Status must be ongoing.
     * @param urls A list of TURLs filled by the method if the request
     * completed successfully
     * @returns DataStatus specifying outcome of operation
     */
    virtual DataStatus getTURLsStatus(SRMClientRequest& req,
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
     * @returns DataStatus specifying outcome of operation
     */
    virtual DataStatus requestBringOnline(SRMClientRequest& req) = 0;

    /**
     * Query the status of a request to bring files online. The SURLs map
     * of the request object is updated if the status of any files in the
     * request has changed. requestBringOnline() but be called before
     * this method.
     * @param req The request object to query the status of
     * @returns DataStatus specifying outcome of operation
     */
    virtual DataStatus requestBringOnlineStatus(SRMClientRequest& req) = 0;

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
     * @returns DataStatus specifying outcome of operation
     */
    virtual DataStatus putTURLs(SRMClientRequest& req,
                                std::list<std::string>& urls) = 0;

    /**
     * In the case where putTURLs was called asynchronously and the request
     * was not completed, this method should be called to poll the status of
     * the request. putTURLs must be called before this method and the request
     * object must have ongoing request status.
     * @param req The request object. Status must be ongoing.
     * @param urls A list of TURLs filled by the method if the request
     * completed successfully
     * @returns DataStatus specifying outcome of operation
     */
    virtual DataStatus putTURLsStatus(SRMClientRequest& req,
                                      std::list<std::string>& urls) = 0;

    /**
     * Should be called after a successful copy from SRM storage.
     * @param req The request object
     * @returns DataStatus specifying outcome of operation
     */
    virtual DataStatus releaseGet(SRMClientRequest& req) = 0;

    /**
     * Should be called after a successful copy to SRM storage.
     * @param req The request object
     * @returns DataStatus specifying outcome of operation
     */
    virtual DataStatus releasePut(SRMClientRequest& req) = 0;

    /**
     * Used in SRM v1 only. Called to release files after successful transfer.
     * @param req The request object
     * @param source Whether source or destination is being released
     * @returns DataStatus specifying outcome of operation
     */
    virtual DataStatus release(SRMClientRequest& req,
                               bool source) = 0;

    /**
     * Called in the case of failure during transfer or releasePut. Releases
     * all TURLs involved in the transfer.
     * @param req The request object
     * @param source Whether source or destination is being aborted
     * @returns DataStatus specifying outcome of operation
     */
    virtual DataStatus abort(SRMClientRequest& req,
                             bool source) = 0;

    /**
     * Returns information on a file or files (v2.2 and higher) stored in SRM,
     * such as file size, checksum and estimated access latency. If a directory
     * or directories is listed with recursion >= 1 then the list mapped to
     * each SURL in metadata will contain the content of the directory or
     * directories.
     * @param req The request object
     * @param metadata A map mapping each SURL in the request to a list of
     * structs filled with file information. If a SURL is missing from the
     * map it means there was some problem accessing it.
     * @returns DataStatus specifying outcome of operation
     * @see SRMFileMetaData
     */
    virtual DataStatus info(SRMClientRequest& req,
                            std::map<std::string, std::list<struct SRMFileMetaData> >& metadata) = 0;

    /**
     * Returns information on a file stored in an SRM, such as file size,
     * checksum and estimated access latency. If a directory is listed
     * with recursion >= 1 then the list in metadata will contain the
     * content of the directory.
     * @param req The request object
     * @param metadata A list of structs filled with file information.
     * @returns DataStatus specifying outcome of operation
     * @see SRMFileMetaData
     */
    virtual DataStatus info(SRMClientRequest& req,
                            std::list<struct SRMFileMetaData>& metadata) = 0;

    /**
     * Delete a file physically from storage and the SRM namespace.
     * @param req The request object
     * @returns DataStatus specifying outcome of operation
     */
    virtual DataStatus remove(SRMClientRequest& req) = 0;

    /**
     * Copy a file between two SRM storages.
     * @param req The request object
     * @param source The source SURL
     * @returns DataStatus specifying outcome of operation
     */
    virtual DataStatus copy(SRMClientRequest& req,
                            const std::string& source) = 0;

    /**
     * Make required directories for the SURL in the request
     * @param req The request object
     * @returns DataStatus specifying outcome of operation
     */
    virtual DataStatus mkDir(SRMClientRequest& req) = 0;

    /**
     * Rename the URL in req to newurl.
     * @oaram req The request object
     * @param newurl The new URL
     * @returns DataStatus specifying outcome of operation
     */
    virtual DataStatus rename(SRMClientRequest& req,
                              const URL& newurl) = 0;
    /**
     * Check permissions for the SURL in the request using the
     * current credentials.
     * @oaram req The request object
     * @returns DataStatus specifying outcome of operation
     */
    virtual DataStatus checkPermissions(SRMClientRequest& req) = 0;


    operator bool() const {
      return client;
    }
    bool operator!() const {
      return !client;
    }
  };

} // namespace ArcDMCSRM

#endif // __HTTPSD_SRM_CLIENT_H__
