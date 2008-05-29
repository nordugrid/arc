#ifndef __ARC_SERVER_OPTIONS_H__
#define __ARC_SERVER_OPTIONS_H__

#include <arc/OptionParser.h>

namespace Arc {

  class ServerOptions : public OptionParser {
  public:
    ServerOptions();
        
    /* Command line options values */
    bool foreground;
    std::string config_file;
    std::string pid_file;
    std::string user;
    std::string group;
#ifdef WIN32
    bool install;
    bool uninstall;
#endif
  };

} // namespace Arc

#endif // __ARC_SERVER_OPTIONS_H__
