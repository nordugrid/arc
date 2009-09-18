#ifndef __ARC_SERVER_OPTIONS_H__
#define __ARC_SERVER_OPTIONS_H__

#include <arc/OptionParser.h>

namespace Arc {

  class ServerOptions : public OptionParser {
  public:
    ServerOptions();
        
    /* Command line options values */
    bool foreground;
    std::string xml_config_file;
    std::string ini_config_file;
    std::string pid_file;
    std::string user;
    std::string group;
    bool config_dump;
    std::string schema_file;
#ifdef WIN32
    bool install;
    bool uninstall;
#endif
  };

} // namespace Arc

#endif // __ARC_SERVER_OPTIONS_H__
