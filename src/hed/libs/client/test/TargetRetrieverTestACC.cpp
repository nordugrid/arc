// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "TargetRetrieverTestACC.h"

Arc::Plugin* TargetRetrieverTestACC::GetInstance(Arc::PluginArgument *arg) {
  Arc::TargetRetrieverPluginArgument *trarg = dynamic_cast<Arc::TargetRetrieverPluginArgument*>(arg);
  if (!trarg) {
    return NULL;
  }
  return new TargetRetrieverTestACC(*trarg, *trarg, *trarg);
}
