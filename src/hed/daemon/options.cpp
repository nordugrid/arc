#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "options.h"

namespace Arc {

  ServerOptions::ServerOptions()
    : OptionParser() {

    foreground = false;
    AddOption('f', "foreground", "run daemon in foreground", foreground);
#ifdef WIN32
    config_file = "service.xml";
#else
    config_file = "/etc/arc/server.xml";
#endif
    AddOption('c', "config", "full path of config file", "path", config_file);
    AddOption('p', "pid-file", "full path of pid file", "path", pid_file);
#ifdef WIN32
    install = false;
    AddOption('i', "install", "install windows service", install);
    uninstall = false;
    AddOption('u', "uninstall", "uninstall windows service", uninstall);
#endif
  }

} // namespace Arc
