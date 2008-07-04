#ifndef __ARC_USERCONFIG_H__
#define __ARC_USERCONFIG_H__

#include <string>

namespace Arc {

  class Logger;

  class UserConfig {

  public:

    UserConfig();
    ~UserConfig();

    operator bool();
    bool operator!();

  private:
    std::string confdir;
    std::string conffile;
    std::string jobsfile;
    bool ok;

    static Logger logger;
  };

} // namespace Arc

#endif // __ARC_USERCONFIG_H__
