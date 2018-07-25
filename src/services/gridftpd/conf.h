#ifndef __GFS_CONF_H__
#define __GFS_CONF_H__

#include <cstring>
#include <iostream>
#include <fstream>
#include <string>

#include <arc/ArcConfigFile.h>

#define DEFAULT_CENTRAL_CONFIG_FILE "/etc/arc.conf"
extern const char* config_file;

std::string config_open_gridftp(Arc::ConfigFile &cfile);

#endif // __GFS_CONF_H__
