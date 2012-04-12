#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/stat.h>
#include "conf.h"

static const char* default_central_config_file = DEFAULT_CENTRAL_CONFIG_FILE;
static const char* default_central_config_file2 = DEFAULT_CENTRAL_CONFIG_FILE2;
const char* config_file = NULL;


std::string config_open_gridftp(std::ifstream &cfile) {
  std::string config_name;
  gridftpd::GMEnvironment env;
  if(!env)
    exit(1);

  if(config_file) {
    config_name=config_file;
  } else {
    struct stat st;
    if(stat(default_central_config_file,&st) == 0) {
      config_name=default_central_config_file;
    } else {
      config_name=default_central_config_file2;
    };
  };
  if(gridftpd::config_open(cfile,config_name)) return config_name;
  return "";
}

void config_strip(std::string &rest) {
  int n=rest.find_first_not_of(" \t",0);
  if(n>0) rest.erase(0,n);
}

