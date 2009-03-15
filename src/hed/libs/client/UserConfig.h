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
  class XMLNode;

  class UserConfig {

  public:

    UserConfig(const std::string& conffile);
    ~UserConfig();

    const std::string& ConfFile() const;
    const std::string& JobListFile() const;
    const XMLNode& ConfTree() const;

    bool ApplySecurity(XMLNode& ccfg) const;

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

    operator bool() const;
    bool operator!() const;

  private:
    User user;
    std::string confdir;
    std::string conffile;
    std::string joblistfile;
    Config cfg;
    Config syscfg;
    bool ok;

    static Logger logger;
  };

} // namespace Arc

#endif // __ARC_USERCONFIG_H__
