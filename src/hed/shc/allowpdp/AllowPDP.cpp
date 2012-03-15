#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <fstream>
#include <sys/types.h>

#include "AllowPDP.h"

using namespace Arc;
namespace ArcSec {

Plugin* AllowPDP::get_allow_pdp(PluginArgument *arg) {
    PDPPluginArgument* pdparg =
            arg?dynamic_cast<PDPPluginArgument*>(arg):NULL;
    if(!pdparg) return NULL;
    return new AllowPDP((Config*)(*pdparg),pdparg);
}

AllowPDP::AllowPDP(Config* cfg, PluginArgument* parg):PDP(cfg,parg){
}

bool AllowPDP::isPermitted(Message*) const {
  return true;
}

} // namespace ArcSec
 
