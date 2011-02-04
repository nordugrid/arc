// -*- indent-tabs-mode: nil -*-

#ifndef __HTTPSD_SRM1_CLIENT_H__
#define __HTTPSD_SRM1_CLIENT_H__

#include "SRMClient.h"

namespace Arc {

  class SRM1Client
    : public SRMClient {
  private:
    SRMReturnCode acquire(SRMClientRequest& req, std::list<std::string>& urls);

  public:
    SRM1Client(const UserConfig& usercfg, const SRMURL& url);
    ~SRM1Client();

    // not supported in v1
    SRMReturnCode ping(std::string& /* version */,
                       bool /* report_error */ = true) {
      return SRM_ERROR_NOT_SUPPORTED;
    }
    // not supported in v1
    SRMReturnCode getSpaceTokens(std::list<std::string>& /* tokens */,
                                 const std::string& /* description */ = "") {
      return SRM_ERROR_NOT_SUPPORTED;
    }
    // not supported in v1
    SRMReturnCode getRequestTokens(std::list<std::string>& /* tokens */,
                                   const std::string& /* description */ = "") {
      return SRM_ERROR_NOT_SUPPORTED;
    }
    // not supported in v1
    SRMReturnCode requestBringOnline(SRMClientRequest& /* req */) {
      return SRM_ERROR_NOT_SUPPORTED;
    }
    // not supported in v1
    SRMReturnCode requestBringOnlineStatus(SRMClientRequest& /* req */) {
      return SRM_ERROR_NOT_SUPPORTED;
    }
    // not supported
    SRMReturnCode mkDir(SRMClientRequest& /* req */) {
      return SRM_ERROR_NOT_SUPPORTED;
    }
  
    // v1 only operates in synchronous mode
    SRMReturnCode getTURLs(SRMClientRequest& req,
                           std::list<std::string>& urls);
    SRMReturnCode getTURLsStatus(SRMClientRequest& req,
                                 std::list<std::string>& urls) {
      return SRM_ERROR_NOT_SUPPORTED;
    }
    SRMReturnCode putTURLs(SRMClientRequest& req,
                           std::list<std::string>& urls);
    SRMReturnCode putTURLsStatus(SRMClientRequest& req,
                                 std::list<std::string>& urls) {
      return SRM_ERROR_NOT_SUPPORTED;
    }

    SRMReturnCode releaseGet(SRMClientRequest& req);
    SRMReturnCode releasePut(SRMClientRequest& req);
    SRMReturnCode release(SRMClientRequest& req);
    SRMReturnCode abort(SRMClientRequest& req);
    SRMReturnCode info(SRMClientRequest& req,
                       std::list<struct SRMFileMetaData>& metadata,
                       const int recursive = 0,
                       bool report_error = true);
    SRMReturnCode remove(SRMClientRequest& req);
    SRMReturnCode copy(SRMClientRequest& req, const std::string& source);
  };

} // namespace Arc

#endif // __HTTPSD_SRM1_CLIENT_H__
