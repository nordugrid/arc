// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "BrokerTestACC.h"

Arc::Logger BrokerTestACC::logger(Arc::Logger::getRootLogger(), "BrokerTestACC");

Arc::Plugin* BrokerTestACC::GetInstance(Arc::PluginArgument *arg) {
  Arc::BrokerPluginArgument *jcarg = dynamic_cast<Arc::BrokerPluginArgument*>(arg);
  if (!jcarg) {
    return NULL;
  }
  return new BrokerTestACC(*jcarg);
}
