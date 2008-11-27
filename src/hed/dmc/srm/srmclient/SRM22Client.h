#ifndef __HTTPSD_SRM_CLIENT_2_2_H__
#define __HTTPSD_SRM_CLIENT_2_2_H__

#include "SRMURL.h"
#include "SRMClient.h"

#include "srm2_2_soapH.h"

const static SOAP_NMAC struct Namespace srm2_2_soap_namespaces[] =
{
  {"SOAP-ENV", "http://schemas.xmlsoap.org/soap/envelope/", "http://www.w3.org/*/soap-envelope", NULL},
  {"SOAP-ENC", "http://schemas.xmlsoap.org/soap/encoding/", "http://www.w3.org/*/soap-encoding", NULL},
  {"xsi", "http://www.w3.org/2001/XMLSchema-instance", "http://www.w3.org/*/XMLSchema-instance", NULL},
  {"xsd", "http://www.w3.org/2001/XMLSchema", "http://www.w3.org/*/XMLSchema", NULL},
  {"SRMv2", "http://srm.lbl.gov/StorageResourceManager", NULL, NULL},
  {NULL, NULL, NULL, NULL}
};

// put here to avoid multiple definition errors
//const static SOAP_NMAC struct Namespace srm2_2_soap_namespaces[] =
//{
// {"SOAP-ENV", "http://schemas.xmlsoap.org/soap/envelope/", "http://www.w3.org/*/soap-envelope", NULL},
//  {"SOAP-ENC", "http://schemas.xmlsoap.org/soap/encoding/", "http://www.w3.org/*/soap-encoding", NULL},
//  {"xsi", "http://www.w3.org/2001/XMLSchema-instance", "http://www.w3.org/*/XMLSchema-instance", NULL},
//  {"xsd", "http://www.w3.org/2001/XMLSchema", "http://www.w3.org/*/XMLSchema", NULL},
//  {"SRMv2", "http://srm.lbl.gov/StorageResourceManager", NULL, NULL},
//  {NULL, NULL, NULL, NULL}
//};

/**
 * The max number of files returned when listing dirs
 * current limits are 1000 for dcache, 1024 for castor
 * info() will be called multiple times for directories
 * with more entries than max_files_list
 */
const static unsigned int max_files_list = 999;

//namespace Arc {
  
  class SRM22Client: public SRMClient {
   private:
    struct soap soapobj;
    static Arc::Logger logger;
    
    /**
     * Internal version of info(), when repeated listing is needed to
     * list large directories.
     * @param offset The index to start at
     * @param count The number of files to list
     */
    bool info(SRMClientRequest& req,
               std::list<struct SRMFileMetaData>& metadata,
               const int recursive,
               const int offset,
               const int count);
  
    /**
     * Remove a file by srmRm
     */
    bool removeFile(SRMClientRequest& req);
  
    /**
     * Remove a directory by srmRmDir
     */
    bool removeDir(SRMClientRequest& req);
  
    /**
     * Return a metadata struct with values filled from the given details
     * @param directory Whether these are entries in a directory. Determines
     * whether the full path is specified (if false) or not (if true)
     */
    SRMFileMetaData fillDetails(SRMv2__TMetaDataPathDetail * details,
                                bool directory);
  
    /**
     * Fill out status of files in the request object from the file_statuses
     */
    void fileStatus(SRMClientRequest& req,
                    SRMv2__ArrayOfTBringOnlineRequestFileStatus * file_statuses);
  
   public:
    SRM22Client(std::string url);
    ~SRM22Client(void);
  
    /**
     * Get the server version from srmPing
     */
    SRMReturnCode ping(std::string& version, bool report_error=true);
  
    /**
     * Use srmGetSpaceTokens to return a list of spaces available
     */
    SRMReturnCode getSpaceTokens(std::list<std::string>& tokens,
                                 std::string description = "");
  
    /**
     * Use srmGetRequestTokens to return a list of spaces available
     */
    SRMReturnCode getRequestTokens(std::list<std::string>& tokens,
                                   std::string description = "");
  
    /**
     * Get a list of TURLs for the given SURL. Uses srmPrepareToGet and waits
     * until file is ready (online and pinned). Although a list is returned,
     * SRMv2.2 only returns one TURL per SURL.
     */
    bool getTURLs(SRMClientRequest& req,
                  std::list<std::string>& urls);
  
    /**
     * Retrieve TURLs which a file can be written to. Uses srmPrepareToPut and
     * waits until a suitable TURL has been assigned. Although a list is returned,
     * SRMv2.2 only returns one TURL per SURL.
     */
    bool putTURLs(SRMClientRequest& req,
                  std::list<std::string>& urls,
                  unsigned long long size = 0);
  
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
    bool info(SRMClientRequest& req,
              std::list<struct SRMFileMetaData>& metadata,
              const int recursive = 0);
  
    /**
     * Release files that have been pinned by srmPrepareToGet using
     * srmReleaseFiles. Called after successful file transfer or
     * failed prepareToGet.
     */
    bool releaseGet(SRMClientRequest& req);
  
    /**
     * Mark a put request as finished.
     * Called after successful file transfer or failed prepareToPut.
     */
    bool releasePut(SRMClientRequest& req);
  
    /**
     * Not used in this version of SRM
     */
    bool release(SRMClientRequest& req) {return false;};
  
    /**
     * Abort request. Called after any failure in the data transfer or putDone calls
     */
    bool abort(SRMClientRequest& req);
  
    /**
     * Delete by srmRm or srmRmDir
     */
    bool remove(SRMClientRequest& req);
  
    /**
     * Implemented in pull mode, ie the endpoint defined in the
     * request object performs the copy.
     */
    bool copy(SRMClientRequest& req,
              const std::string& source);
  
  };
//} // namespace Arc

#endif // __HTTPSD_SRM_CLIENT_2_2_H__
