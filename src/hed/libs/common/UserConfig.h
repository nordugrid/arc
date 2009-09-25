// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_USERCONFIG_H__
#define __ARC_USERCONFIG_H__

#include <list>
#include <map>
#include <string>

#include <arc/ArcConfig.h>
#include <arc/URL.h>
#include <arc/User.h>

namespace Arc {

  typedef std::map<std::string, std::list<URL> > URLListMap;

  class Logger;
  class JobController;
  class XMLNode;

  class UserConfig {

  public:
    UserConfig(bool initializeCredentials = true);
    UserConfig(const XMLNode& cfg);
    UserConfig(const std::string& conffile, bool initializeCredentials = true);
    UserConfig(const std::string& conffile, const std::string& joblistfile, bool initializeCredentials = true);
    UserConfig(const long int& ptraddr);
    ~UserConfig() {}

    const std::string& ConfFile() const {
      return conffile;
    }
    const std::string& JobListFile() const {
      return joblistfile;
    }
    const XMLNode& ConfTree() const {
      return cfg;
    }

    bool CredentialsFound() const {
      return !(proxyPath.empty() && (certificatePath.empty() || keyPath.empty() || caCertificatesDir.empty()));
    }

    void InitializeCredentials();

    void SetTimeOut(unsigned int timeout);
    void SetBroker(const std::string& broker);

    void ApplyToConfig(XMLNode& ccfg) const;
    void ApplyToConfig(BaseConfig& ccfg) const;

    bool DefaultServices(URLListMap& cluster,
                         URLListMap& index) const;

    bool ResolveAlias(const std::string alias,
                      URLListMap& cluster,
                      URLListMap& index) const;

    bool ResolveAlias(const std::list<std::string>& clusters,
                      const std::list<std::string>& indices,
                      URLListMap& clusterselect,
                      URLListMap& clusterreject,
                      URLListMap& indexselect,
                      URLListMap& indexreject) const;

    bool ResolveAlias(const std::list<std::string>& clusters,
                      URLListMap& clusterselect,
                      URLListMap& clusterreject) const;

    operator bool() const {
      return ok;
    }
    bool operator!() const {
      return !ok;
    }

    static const std::string ARCUSERDIR;

    static const int DEFAULT_TIMEOUT = 20;
    static const std::string DEFAULT_BROKER;

  private:
    bool loadUserConfiguration();
    void setDefaults();
    static bool makeDir(const std::string& path);
    static bool copyFile(const std::string& source, const std::string& destination);

    User user;
    std::string conffile;
    std::string joblistfile;
    bool userSpecifiedJobList;
    Config cfg;
    bool ok;

    static std::list<std::string> resolvedAlias;

    std::string proxyPath;
    std::string certificatePath;
    std::string keyPath;
    std::string caCertificatesDir;

    static Logger logger;
  };

} // namespace Arc

#endif // __ARC_USERCONFIG_H__
