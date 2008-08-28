#ifndef __ARC_USERCONFIG_H__
#define __ARC_USERCONFIG_H__

#include <list>
#include <string>

#include <arc/ArcConfig.h>
#include <arc/XMLNode.h>

namespace Arc {

  class Logger;

  class UserConfig {

  public:

    UserConfig();
    ~UserConfig();

    const std::string& ConfFile();
    const std::string& JobsFile();

    const XMLNode ConfTree();

    operator bool();
    bool operator!();
    static std::list<std::string> ResolveAlias(std::string lookup, XMLNode cfg);

  private:
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
