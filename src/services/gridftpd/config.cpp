#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/stat.h>
#include <arc/Utils.h>
#include "conf.h"

static const char* default_central_config_file = DEFAULT_CENTRAL_CONFIG_FILE;
const char* config_file = NULL;


std::string config_open_gridftp(Arc::ConfigFile &cfile) {
  std::string config_name;

  if(config_file) {
    config_name=config_file;
  } else if((config_name=Arc::GetEnv("ARC_CONFIG")).empty()) {
    config_name=default_central_config_file;
  };
  if(!cfile.open(config_name)) return "";
  // Set environment variable for other tools
  Arc::SetEnv("ARC_CONFIG",config_name.c_str());
  return config_name;
}

