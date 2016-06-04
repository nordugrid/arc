#ifndef __GFS_CONF_H__
#define __GFS_CONF_H__

#include <cstring>
#include <iostream>
#include <fstream>
#include <string>

#include <arc/ArcConfigFile.h>

#include "conf/environment.h"

#define DEFAULT_CONFIG_FILE "/etc/gridftpd.conf"
#define DEFAULT_CENTRAL_CONFIG_FILE "/etc/arc.conf"
#define DEFAULT_CENTRAL_CONFIG_FILE2 "/etc/nordugrid.conf"
extern const char* config_file;

std::string config_open_gridftp(Arc::ConfigFile &cfile);
void config_strip(std::string &rest);

#endif // __GFS_CONF_H__
