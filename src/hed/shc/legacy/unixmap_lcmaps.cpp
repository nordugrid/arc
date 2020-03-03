#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <iostream>
#include <stdio.h>

#include <arc/Logger.h>
//#include <arc/Thread.h>
#include <arc/ArcLocation.h>

//#include "../misc/escaped.h"
//#include "../misc/proxy.h"
//#include "../run/run_plugin.h"
#include "unixmap.h"

namespace ArcSHCLegacy {

static Arc::Logger logger(Arc::Logger::getRootLogger(),"UnixMap");

AuthResult UnixMap::map_lcmaps(const AuthUser& user,unix_user_t& unix_user,const char* line) {
  // TODO: escape
  // TODO: hardcoded 60s timeout for lcmaps
  std::string lcmaps_plugin = "60 \""+
    Arc::ArcLocation::Get()+G_DIR_SEPARATOR_S+PKGLIBEXECSUBDIR+
    G_DIR_SEPARATOR_S+"arc-lcmaps\" ";
  lcmaps_plugin+=std::string("\"")+user_.subject()+"\" ";
  lcmaps_plugin+=std::string("\"")+user_.proxy()+"\" ";
  lcmaps_plugin+=line;
  AuthResult res = map_mapplugin(user,unix_user,lcmaps_plugin.c_str());
  return res;
}

} // namespace ArcSHCLegacy

