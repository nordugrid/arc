#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <string.h>

#include <glibmm.h>

#include <arc/ArcLocation.h>

#include "auth.h"

namespace ArcSHCLegacy {

int AuthUser::match_lcas(const char* line) {
  // TODO: escape
  std::string lcas_plugin = "\""+
    Arc::ArcLocation::Get()+G_DIR_SEPARATOR_S+PKGLIBEXECSUBDIR+
    G_DIR_SEPARATOR_S+"arc-lcas\" \""+DN()+"\" \""+proxy()+"\" ";
  lcas_plugin+=line;
  int res = match_plugin(lcas_plugin.c_str());
  return res;
}

} // namespace ArcSHCLegacy

