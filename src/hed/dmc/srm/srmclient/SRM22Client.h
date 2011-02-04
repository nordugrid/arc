// -*- indent-tabs-mode: nil -*-

#ifndef __HTTPSD_SRM_CLIENT_2_2_H__
#define __HTTPSD_SRM_CLIENT_2_2_H__

#include "SRMClient.h"

/**
 * The max number of files returned when listing dirs
 * current limits are 1000 for dcache, 1024 for castor
 * info() will be called multiple times for directories
 * with more entries than max_files_list
 */
const static unsigned int max_files_list = 999;

namespace Arc {

  class SRM22Client
    : public SRMClient {
  private:
    /**
     * Internal version of info(), when repeated listing is needed to
     * list large directories.
     * @param offset The index to start at
     * @param count The number of files to list
     */
    SRMReturnCode info(SRMClientRequest& req,
                       std::list<struct SRMFileMetaData>& metadata,
                       const int recursive,
                       bool report_error,
                       const int offset,
                       const int count);

    /**
     * Remove a file by srmRm
     */
    SRMReturnCode removeFile(SRMClientRequest& req);

    /**
     * Remove a directory by srmRmDir
     */
    SRMReturnCode removeDir(SRMClientRequest& req);

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
    SRMReturnCode ping(std::string& version, bool report_error = true);

    /**
     * Use srmGetSpaceTokens to return a list of spaces available
     */
    SRMReturnCode getSpaceTokens(std::list<std::string>& tokens,
                                 const std::string& description = "");

    /**
     * Use srmGetRequestTokens to return a list of spaces available
     */
    SRMReturnCode getRequestTokens(std::list<std::string>& tokens,
                                   const std::string& description = "");

    /**
     * Get a list of TURLs for the given SURL. Uses srmPrepareToGet and waits
     * until file is ready (online and pinned) if the request is synchronous.
     * If not it returns after making the request. Although a list is returned,
     * SRMv2.2 only returns one TURL per SURL.
     */
    SRMReturnCode getTURLs(SRMClientRequest& req,
                           std::list<std::string>& urls);

    /**
     * Uses srmStatusOfGetRequest to query the status of the given request.
     */
    SRMReturnCode getTURLsStatus(SRMClientRequest& req,
                                 std::list<std::string>& urls);

    /**
     * Retrieve TURLs which a file can be written to. Uses srmPrepareToPut and
     * waits until a suitable TURL has been assigned if the request is
     * synchronous. If not it returns after making the request. Although a
     * list is returned, SRMv2.2 only returns one TURL per SURL.
     */
    SRMReturnCode putTURLs(SRMClientRequest& req,
                           std::list<std::string>& urls);
  
   /**
     * Uses srmStatusOfPutRequest to query the status of the given request.
     */
    SRMReturnCode putTURLsStatus(SRMClientRequest& req,
                                 std::list<std::string>& urls);

    /**
     * Call srmBringOnline with the SURLs specified in req.
     */
    SRMReturnCode requestBringOnline(SRMClientRequest& req);

    /**
     * Call srmStatusOfBringOnlineRequest and update req with any changes.
     */
    SRMReturnCode requestBringOnlineStatus(SRMClientRequest& req);

    /**
     * Use srmLs to get info on the given SURL. Info on each file is put in a
     * metadata struct and added to the list.
     */
    SRMReturnCode info(SRMClientRequest& req,
                       std::list<struct SRMFileMetaData>& metadata,
                       const int recursive = 0,
                       bool report_error = true);

    /**
     * Release files that have been pinned by srmPrepareToGet using
     * srmReleaseFiles. Called after successful file transfer or
     * failed prepareToGet.
     */
    SRMReturnCode releaseGet(SRMClientRequest& req);

    /**
     * Mark a put request as finished.
     * Called after successful file transfer or failed prepareToPut.
     */
    SRMReturnCode releasePut(SRMClientRequest& req);

    /**
     * Not used in this version of SRM
     */
    SRMReturnCode release(SRMClientRequest& /* req */) {
      return SRM_ERROR_NOT_SUPPORTED;
    }

    /**
     * Abort request.
     * Called after any failure in the data transfer or putDone calls
     */
    SRMReturnCode abort(SRMClientRequest& req);

    /**
     * Delete by srmRm or srmRmDir
     */
    SRMReturnCode remove(SRMClientRequest& req);

    /**
     * Implemented in pull mode, ie the endpoint defined in the
     * request object performs the copy.
     */
    SRMReturnCode copy(SRMClientRequest& req,
                       const std::string& source);

    /**
     * Call srmMkDir
     */
    SRMReturnCode mkDir(SRMClientRequest& req);
  };

} // namespace Arc

#endif // __HTTPSD_SRM_CLIENT_2_2_H__
