#ifndef __ARC_USERCONFIG_H__
#define __ARC_USERCONFIG_H__

#include <arc/XMLNode.h>
#include <string>

namespace Arc {

  class Logger;

  class UserConfig {

  public:

    UserConfig();
    ~UserConfig();

    const std::string& ConfFile();
    const std::string& JobsFile();

    operator bool();
    bool operator!();
    static std::list<std::string> ResolveAlias(std::string lookup, XMLNode cfg);


  private:
    std::string confdir;
    std::string conffile;
    std::string jobsfile;
    Config FinalCfg;
    bool ok;

    static Logger logger;
  };

} // namespace Arc

#endif // __ARC_USERCONFIG_H__
