#ifndef __ARC_USERCONFIG_H__
#define __ARC_USERCONFIG_H__

#include <list>
#include <string>

#include <arc/ArcConfig.h>
#include <arc/User.h>

namespace Arc {

  class Logger;
  class XMLNode;

  class UserConfig {

  public:

    UserConfig(const std::string& conffile);
    ~UserConfig();

    const std::string& ConfFile() const;
    const std::string& JobsFile() const;

    bool ApplySecurity(XMLNode &ccfg) const;
    std::list<std::string> ResolveAlias(const std::string& lookup) const;

    operator bool() const;
    bool operator!() const;

  private:
    User user;
    std::string confdir;
    std::string conffile;
    std::string jobsfile;
    Config cfg;
    Config syscfg;
    bool ok;

    static Logger logger;
  };

} // namespace Arc

#endif // __ARC_USERCONFIG_H__
