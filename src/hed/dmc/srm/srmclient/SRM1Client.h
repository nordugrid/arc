// -*- indent-tabs-mode: nil -*-

#ifndef __HTTPSD_SRM1_CLIENT_H__
#define __HTTPSD_SRM1_CLIENT_H__

#include "SRMClient.h"

namespace ArcDMCSRM {

  using namespace Arc;

  class SRM1Client
    : public SRMClient {
  private:
    DataStatus acquire(SRMClientRequest& req, std::list<std::string>& urls, bool source);

  public:
    SRM1Client(const UserConfig& usercfg, const SRMURL& url);
    ~SRM1Client();

    // not supported in v1
    DataStatus ping(std::string& /* version */) {
      return DataStatus(DataStatus::UnimplementedError, EOPNOTSUPP);
    }
    // not supported in v1
    DataStatus getSpaceTokens(std::list<std::string>& /* tokens */,
                              const std::string& /* description */ = "") {
      return DataStatus(DataStatus::UnimplementedError, EOPNOTSUPP);
    }
    // not supported in v1
    DataStatus getRequestTokens(std::list<std::string>& /* tokens */,
                                const std::string& /* description */ = "") {
      return DataStatus(DataStatus::UnimplementedError, EOPNOTSUPP);
    }
    // not supported in v1
    DataStatus requestBringOnline(SRMClientRequest& /* req */) {
      return DataStatus(DataStatus::UnimplementedError, EOPNOTSUPP);
    }
    // not supported in v1
    DataStatus requestBringOnlineStatus(SRMClientRequest& /* req */) {
      return DataStatus(DataStatus::UnimplementedError, EOPNOTSUPP);
    }
    // not supported
    DataStatus mkDir(SRMClientRequest& /* req */) {
      return DataStatus(DataStatus::UnimplementedError, EOPNOTSUPP);
    }
    // not supported
    DataStatus rename(SRMClientRequest& /* req */,
                      const URL& /* newurl */) {
      return DataStatus(DataStatus::UnimplementedError, EOPNOTSUPP);
    }
    // not supported
    DataStatus checkPermissions(SRMClientRequest& /* req */) {
      return DataStatus(DataStatus::UnimplementedError, EOPNOTSUPP);
    }
  
    // v1 only operates in synchronous mode
    DataStatus getTURLs(SRMClientRequest& req,
                        std::list<std::string>& urls);
    DataStatus getTURLsStatus(SRMClientRequest& req,
                              std::list<std::string>& urls) {
      return DataStatus(DataStatus::UnimplementedError, EOPNOTSUPP);
    }
    DataStatus putTURLs(SRMClientRequest& req,
                        std::list<std::string>& urls);
    DataStatus putTURLsStatus(SRMClientRequest& req,
                              std::list<std::string>& urls) {
      return DataStatus(DataStatus::UnimplementedError, EOPNOTSUPP);
    }

    DataStatus releaseGet(SRMClientRequest& req);
    DataStatus releasePut(SRMClientRequest& req);
    DataStatus release(SRMClientRequest& req,
                       bool source);
    DataStatus abort(SRMClientRequest& req,
                     bool source);
    DataStatus info(SRMClientRequest& req,
                    std::map<std::string, std::list<struct SRMFileMetaData> >& metadata);
    DataStatus info(SRMClientRequest& req,
                    std::list<struct SRMFileMetaData>& metadata);
    DataStatus remove(SRMClientRequest& req);
    DataStatus copy(SRMClientRequest& req, const std::string& source);
  };

} // namespace ArcDMCSRM

#endif // __HTTPSD_SRM1_CLIENT_H__
