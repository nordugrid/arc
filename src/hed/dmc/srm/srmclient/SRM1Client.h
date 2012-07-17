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
      return EOPNOTSUPP;
    }
    // not supported in v1
    SRMReturnCode getSpaceTokens(std::list<std::string>& /* tokens */,
                                 const std::string& /* description */ = "") {
      return EOPNOTSUPP;
    }
    // not supported in v1
    SRMReturnCode getRequestTokens(std::list<std::string>& /* tokens */,
                                   const std::string& /* description */ = "") {
      return EOPNOTSUPP;
    }
    // not supported in v1
    SRMReturnCode requestBringOnline(SRMClientRequest& /* req */) {
      return EOPNOTSUPP;
    }
    // not supported in v1
    SRMReturnCode requestBringOnlineStatus(SRMClientRequest& /* req */) {
      return EOPNOTSUPP;
    }
    // not supported
    SRMReturnCode mkDir(SRMClientRequest& /* req */) {
      return EOPNOTSUPP;
    }
    // not supported
    SRMReturnCode rename(SRMClientRequest& /* req */,
                         const URL& /* newurl */) {
      return EOPNOTSUPP;
    }
    // not supported
    SRMReturnCode checkPermissions(SRMClientRequest& /* req */) {
      return EOPNOTSUPP;
    }
  
    // v1 only operates in synchronous mode
    SRMReturnCode getTURLs(SRMClientRequest& req,
                           std::list<std::string>& urls);
    SRMReturnCode getTURLsStatus(SRMClientRequest& req,
                                 std::list<std::string>& urls) {
      return EOPNOTSUPP;
    }
    SRMReturnCode putTURLs(SRMClientRequest& req,
                           std::list<std::string>& urls);
    SRMReturnCode putTURLsStatus(SRMClientRequest& req,
                                 std::list<std::string>& urls) {
      return EOPNOTSUPP;
    }

    SRMReturnCode releaseGet(SRMClientRequest& req);
    SRMReturnCode releasePut(SRMClientRequest& req);
    SRMReturnCode release(SRMClientRequest& req);
    SRMReturnCode abort(SRMClientRequest& req);
    SRMReturnCode info(SRMClientRequest& req,
                       std::map<std::string, std::list<struct SRMFileMetaData> >& metadata);
    SRMReturnCode info(SRMClientRequest& req,
                       std::list<struct SRMFileMetaData>& metadata);
    SRMReturnCode remove(SRMClientRequest& req);
    SRMReturnCode copy(SRMClientRequest& req, const std::string& source);
  };

} // namespace Arc

#endif // __HTTPSD_SRM1_CLIENT_H__
