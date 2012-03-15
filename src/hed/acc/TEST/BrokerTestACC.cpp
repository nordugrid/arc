// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "BrokerTestACC.h"

namespace Arc {

Plugin* BrokerTestACC::GetInstance(PluginArgument *arg) {
  BrokerPluginArgument *jcarg = dynamic_cast<BrokerPluginArgument*>(arg);
  if (!jcarg) {
    return NULL;
  }
  return new BrokerTestACC(*jcarg,arg);
}

}
