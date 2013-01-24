// -*- indent-tabs-mode: nil -*-

#ifndef __HTTPSD_SRM_CLIENT_2_2_H__
#define __HTTPSD_SRM_CLIENT_2_2_H__

#include "SRMClient.h"

namespace ArcDMCSRM {

  using namespace Arc;

  class SRM22Client
    : public SRMClient {
  private:

    /// Return codes for requests and files as defined in spec
    enum SRMStatusCode {
      SRM_SUCCESS,
      SRM_FAILURE,
      SRM_AUTHENTICATION_FAILURE,
      SRM_AUTHORIZATION_FAILURE,
      SRM_INVALID_REQUEST,
      SRM_INVALID_PATH,
      SRM_FILE_LIFETIME_EXPIRED,
      SRM_SPACE_LIFETIME_EXPIRED,
      SRM_EXCEED_ALLOCATION,
      SRM_NO_USER_SPACE,
      SRM_NO_FREE_SPACE,
      SRM_DUPLICATION_ERROR,
      SRM_NON_EMPTY_DIRECTORY,
      SRM_TOO_MANY_RESULTS,
      SRM_INTERNAL_ERROR,
      SRM_FATAL_INTERNAL_ERROR,
      SRM_NOT_SUPPORTED,
      SRM_REQUEST_QUEUED,
      SRM_REQUEST_INPROGRESS,
      SRM_REQUEST_SUSPENDED,
      SRM_ABORTED,
      SRM_RELEASED,
      SRM_FILE_PINNED,
      SRM_FILE_IN_CACHE,
      SRM_SPACE_AVAILABLE,
      SRM_LOWER_SPACE_GRANTED,
      SRM_DONE,
      SRM_PARTIAL_SUCCESS,
      SRM_REQUEST_TIMED_OUT,
      SRM_LAST_COPY,
      SRM_FILE_BUSY,
      SRM_FILE_LOST,
      SRM_FILE_UNAVAILABLE,
      SRM_CUSTOM_STATUS
    };

    /// Extract status code and explanation from xml result structure
    SRMStatusCode GetStatus(XMLNode res, std::string& explanation);

    /**
     * Remove a file by srmRm
     */
    DataStatus removeFile(SRMClientRequest& req);

    /**
     * Remove a directory by srmRmDir
     */
    DataStatus removeDir(SRMClientRequest& req);

    /**
     * Return a metadata struct with values filled from the given details
     * @param directory Whether these are entries in a directory. Determines
     * whether the full path is specified (if false) or not (if true)
     */
    SRMFileMetaData fillDetails(XMLNode details, bool directory);

    /**
     * Fill out status of files in the request object from the file_statuses
     */
    void fileStatus(SRMClientRequest& req, XMLNode file_statuses);

    /**
     * Convert SRM error code to errno. If file-level status is defined that
     * will be used over request-level status.
     */
    int srm2errno(SRMStatusCode reqstat, SRMStatusCode filestat = SRM_SUCCESS);

  public:
    /**
     * Constructor
     */
    SRM22Client(const UserConfig& usercfg, const SRMURL& url);

    /**
     * Destructor
     */
    ~SRM22Client();

    /**
     * Get the server version from srmPing
     */
    DataStatus ping(std::string& version);

    /**
     * Use srmGetSpaceTokens to return a list of spaces available
     */
    DataStatus getSpaceTokens(std::list<std::string>& tokens,
                              const std::string& description = "");

    /**
     * Use srmGetRequestTokens to return a list of spaces available
     */
    DataStatus getRequestTokens(std::list<std::string>& tokens,
                                const std::string& description = "");

    /**
     * Get a list of TURLs for the given SURL. Uses srmPrepareToGet and waits
     * until file is ready (online and pinned) if the request is synchronous.
     * If not it returns after making the request. Although a list is returned,
     * SRMv2.2 only returns one TURL per SURL.
     */
    DataStatus getTURLs(SRMClientRequest& req,
                        std::list<std::string>& urls);

    /**
     * Uses srmStatusOfGetRequest to query the status of the given request.
     */
    DataStatus getTURLsStatus(SRMClientRequest& req,
                              std::list<std::string>& urls);

    /**
     * Retrieve TURLs which a file can be written to. Uses srmPrepareToPut and
     * waits until a suitable TURL has been assigned if the request is
     * synchronous. If not it returns after making the request. Although a
     * list is returned, SRMv2.2 only returns one TURL per SURL.
     */
    DataStatus putTURLs(SRMClientRequest& req,
                        std::list<std::string>& urls);
  
   /**
     * Uses srmStatusOfPutRequest to query the status of the given request.
     */
    DataStatus putTURLsStatus(SRMClientRequest& req,
                              std::list<std::string>& urls);

    /**
     * Call srmBringOnline with the SURLs specified in req.
     */
    DataStatus requestBringOnline(SRMClientRequest& req);

    /**
     * Call srmStatusOfBringOnlineRequest and update req with any changes.
     */
    DataStatus requestBringOnlineStatus(SRMClientRequest& req);

    /**
     * Use srmLs to get info on the given SURLs. Info on each file or content
     * of directory is put in a list of metadata structs.
     */
    DataStatus info(SRMClientRequest& req,
                    std::map<std::string, std::list<struct SRMFileMetaData> >& metadata);

    /**
     * Use srmLs to get info on the given SURL. Info on each file or content
     * of directory is put in a list of metadata structs
     */
    DataStatus info(SRMClientRequest& req,
                    std::list<struct SRMFileMetaData>& metadata);

    /**
     * Release files that have been pinned by srmPrepareToGet using
     * srmReleaseFiles. Called after successful file transfer or
     * failed prepareToGet.
     */
    DataStatus releaseGet(SRMClientRequest& req);

    /**
     * Mark a put request as finished.
     * Called after successful file transfer or failed prepareToPut.
     */
    DataStatus releasePut(SRMClientRequest& req);

    /**
     * Not used in this version of SRM
     */
    DataStatus release(SRMClientRequest& /* req */,
                       bool /* source */) {
      return DataStatus(DataStatus::UnimplementedError, EOPNOTSUPP);
    }

    /**
     * Abort request.
     * Called after any failure in the data transfer or putDone calls
     */
    DataStatus abort(SRMClientRequest& req,
                     bool source);

    /**
     * Delete by srmRm or srmRmDir
     */
    DataStatus remove(SRMClientRequest& req);

    /**
     * Implemented in pull mode, ie the endpoint defined in the
     * request object performs the copy.
     */
    DataStatus copy(SRMClientRequest& req,
                    const std::string& source);

    /**
     * Call srmMkDir
     */
    DataStatus mkDir(SRMClientRequest& req);

    /**
     * Call srmMv
     */
    DataStatus rename(SRMClientRequest& req,
                      const URL& newurl);

    /**
     * Call srmCheckPermission
     */
    DataStatus checkPermissions(SRMClientRequest& req);

  };

} // namespace ArcDMCSRM

#endif // __HTTPSD_SRM_CLIENT_2_2_H__
