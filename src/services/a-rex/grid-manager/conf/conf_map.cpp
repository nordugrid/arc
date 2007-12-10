#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <fstream>

#include <arc/Logger.h>

#include "conf_sections.h"
#include "environment.h"
#include "conf.h"

#include "conf_map.h"

static Arc::Logger& glogger = Arc::Logger::getRootLogger();

UrlMapConfig::UrlMapConfig(void) {
  std::ifstream cfile;
  ConfigSections* cf = NULL;
  if(nordugrid_config_loc.length() == 0) read_env_vars(true);
  if(!config_open(cfile)) {
    glogger.msg(Arc::ERROR,"Can't open configuration file.");
    return;
  };
  if(central_configuration) {
    cf=new ConfigSections(cfile);
    cf->AddSection("common");
    cf->AddSection("grid-manager");
  };
  for(;;) {
    std::string rest;
    std::string command;
    if(central_configuration) {
      cf->ReadNext(command,rest);
    } else {
      command = config_read_line(cfile,rest);
    };
    if(command.length() == 0) break;
    else if(command == "copyurl") {
      std::string initial = config_next_arg(rest);
      std::string replacement = config_next_arg(rest);
      if((initial.length() == 0) || (replacement.length() == 0)) {
        glogger.msg(Arc::ERROR,"Not enough parameters in copyurl");
        continue;
      };
      add(initial,replacement);
    }
    else if(command == "linkurl") {
      std::string initial = config_next_arg(rest);
      std::string replacement = config_next_arg(rest);
      if((initial.length() == 0) || (replacement.length() == 0)) {
        glogger.msg(Arc::ERROR,"Not enough parameters in linkurl");
        continue;
      };
      std::string access = config_next_arg(rest);
      if(access.length() == 0) access = replacement;
      add(initial,replacement,access);
    };
  };
  config_close(cfile);
  if(cf) delete cf;
}

UrlMapConfig::~UrlMapConfig(void) {
}

