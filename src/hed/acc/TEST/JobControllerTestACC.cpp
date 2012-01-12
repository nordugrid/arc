// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "JobControllerTestACC.h"

Arc::Plugin* JobControllerTestACC::GetInstance(Arc::PluginArgument *arg) {
  Arc::JobControllerPluginArgument *jcarg = dynamic_cast<Arc::JobControllerPluginArgument*>(arg);
  if (!jcarg) {
    return NULL;
  }
  return new JobControllerTestACC(*jcarg);
}
