#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "options.h"

namespace Arc {

  ServerOptions::ServerOptions()
    : OptionParser() {

    foreground = false;
    AddOption('f', "foreground", "run daemon in foreground", foreground);
    watchdog = false;
    AddOption('w', "watchdog", "enable watchdog to restart process", watchdog);
    AddOption('c', "xml-config", "full path of XML config file", "path", xml_config_file);
    AddOption('i', "ini-config", "full path of InI config file", "path", ini_config_file);
    config_dump = false;
    AddOption('d', "config-dump", "dump generated XML config", config_dump);
    AddOption('p', "pid-file", "full path of pid file", "path", pid_file);
    AddOption('l', "log-file", "full path of log file", "path", log_file);
    AddOption('u', "user", "user name", "user", user);
    AddOption('g', "group", "group name", "group", group);
    AddOption('s', "schema", "full path of XML schema file", "path", schema_file);
    version = false;
    AddOption('v', "version", "print version information", version);
  }

} // namespace Arc
