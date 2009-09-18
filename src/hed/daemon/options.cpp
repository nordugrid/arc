#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "options.h"

namespace Arc {

  ServerOptions::ServerOptions()
    : OptionParser() {

    foreground = false;
    AddOption('f', "foreground", "run daemon in foreground", foreground);
    AddOption('c', "xml-config", "full path of XML config file", "path", xml_config_file);
    AddOption('i', "ini-config", "full path of InI config file", "path", ini_config_file);
    config_dump = false;
    AddOption('d', "config-dump", "dump generated XML config", config_dump);
    AddOption('p', "pid-file", "full path of pid file", "path", pid_file);
    AddOption('u', "user", "user name", "user", user);
    AddOption('g', "group", "group name", "group", group);
    AddOption('s', "schema", "full path of XML schema file", "path", schema_file);
#ifdef WIN32
    install = false;
    AddOption('a', "install", "install windows service", install);
    uninstall = false;
    AddOption('r', "uninstall", "uninstall windows service", uninstall);
#endif
  }

} // namespace Arc
