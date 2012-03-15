#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <fstream>
#include <sys/types.h>

#include "DenyPDP.h"

using namespace Arc;
using namespace ArcSec;

Plugin* DenyPDP::get_deny_pdp(PluginArgument* arg) {
    ArcSec::PDPPluginArgument* pdparg =
            arg?dynamic_cast<ArcSec::PDPPluginArgument*>(arg):NULL;
    if(!pdparg) return NULL;
    return new DenyPDP((Arc::Config*)(*pdparg),arg);
}

DenyPDP::DenyPDP(Config* cfg,PluginArgument* parg):PDP(cfg,parg){
}

bool DenyPDP::isPermitted(Message*) const {
  return false;
}

