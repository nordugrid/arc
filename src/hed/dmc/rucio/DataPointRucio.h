#ifndef __ARC_DATAPOINTRUCIO_H__
#define __ARC_DATAPOINTRUCIO_H__

#include <arc/Logger.h>
#include <arc/URL.h>
#include <arc/data/DataPointIndex.h>

namespace ArcDMCRucio {

  /// Store of auth tokens for different accounts. Not thread-safe so locking
  /// should be applied by the user of this class.
  class RucioTokenStore {
   private:
    /// Token associated to account and with expiry time
    class RucioToken {
     public:
      Arc::Time expirytime;
      std::string token;
    };
    /// Map of account to RucioToken
    std::map<std::string, RucioToken> tokens;
    static Arc::Logger logger;
   public:
    /// Add a token to the store. An existing token with same account will be replaced.
    void AddToken(const std::string& account, const Arc::Time& expirytime, const std::string& token);
    /// Get a token from the store. Returns empty string if token is not in the
    /// store or is expired.
    std::string GetToken(const std::string& account);
  };

  /**
   * Rucio is the ATLAS Data Management System. A file in Rucio is represented
   * by a URL like rucio://rucio.cern.ch/replicas/scope/lfn. Calling GET/POST on
   * this URL with content-type metalink gives a list of physical locations
   * along with some metadata. Only reading from Rucio is currently supported.
   *
   * Before resolving a URL an auth token is obtained from the Rucio auth
   * service (currently hard-coded). These tokens are valid for one hour
   * and are cached to allow the same credentials to use a token many times.
   */
  class DataPointRucio
    : public Arc::DataPointIndex {
  public:
    DataPointRucio(const Arc::URL& url, const Arc::UserConfig& usercfg, Arc::PluginArgument* parg);
    ~DataPointRucio();
    static Plugin* Instance(Arc::PluginArgument *arg);
    virtual Arc::DataStatus Resolve(bool source);
    virtual Arc::DataStatus Resolve(bool source, const std::list<DataPoint*>& urls);
    virtual Arc::DataStatus Check(bool check_meta);
    virtual Arc::DataStatus PreRegister(bool replication, bool force = false);
    virtual Arc::DataStatus PostRegister(bool replication);
    virtual Arc::DataStatus PreUnregister(bool replication);
    virtual Arc::DataStatus Unregister(bool all);
    virtual Arc::DataStatus Stat(Arc::FileInfo& file, Arc::DataPoint::DataPointInfoType verb = INFO_TYPE_ALL);
    virtual Arc::DataStatus Stat(std::list<Arc::FileInfo>& files,
                                 const std::list<Arc::DataPoint*>& urls,
                                 Arc::DataPoint::DataPointInfoType verb = INFO_TYPE_ALL);
    virtual Arc::DataStatus List(std::list<Arc::FileInfo>& files, Arc::DataPoint::DataPointInfoType verb = INFO_TYPE_ALL);
    virtual Arc::DataStatus CreateDirectory(bool with_parents=false);
    virtual Arc::DataStatus Rename(const Arc::URL& newurl);
  protected:
    static Arc::Logger logger;
  private:
    /// Rucio account to use for communication with rucio
    std::string account;
    /// In-memory cache of auth tokens
    static RucioTokenStore tokens;
    /// Lock to protect access to tokens
    static Glib::Mutex lock;
    /// Rucio auth url
    Arc::URL auth_url;
    /// Length of time for which a token is valid
    const static Arc::Period token_validity;
    /// Check if a valid auth token exists in the cache and if not get a new one
    Arc::DataStatus checkToken(std::string& token);
    /// Call Rucio to obtain json of replica info
    Arc::DataStatus queryRucio(std::string& content, const std::string& token) const;
    /// Parse replica json
    Arc::DataStatus parseLocations(const std::string& content);

  };

} // namespace ArcDMCRucio

#endif /* __ARC_DATAPOINTRUCIO_H__ */
