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
    return new DenyPDP((Arc::Config*)(*pdparg));
}

DenyPDP::DenyPDP(Config* cfg):PDP(cfg){
}

bool DenyPDP::isPermitted(Message*){
  return false;
}

