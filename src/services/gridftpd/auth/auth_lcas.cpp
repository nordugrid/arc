#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <iostream>
#include <dlfcn.h>
#include <stdio.h>

#include <arc/Thread.h>
#include <arc/Logger.h>
#include <arc/ArcLocation.h>

#include "../misc/escaped.h"
#include "../misc/proxy.h"
#include "auth.h"

static Arc::Logger logger(Arc::Logger::getRootLogger(),"AuthUserLCAS");

int AuthUser::match_lcas(const char* line) {
  // TODO: escape
  // TODO: hardcoded 60s timeout
  std::string lcas_plugin = "60 \""+
    Arc::ArcLocation::Get()+G_DIR_SEPARATOR_S+PKGLIBEXECSUBDIR+
    G_DIR_SEPARATOR_S+"arc-lcas\" \""+DN()+"\" \""+proxy()+"\" ";
  lcas_plugin+=std::string("\"")+DN()+"\" ";
  lcas_plugin+=std::string("\"")+proxy()+"\" ";
  lcas_plugin+=line;
  int res = match_plugin(lcas_plugin.c_str());
  return res;
}
