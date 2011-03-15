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

Arc::PluginDescriptor PLUGINS_TABLE_NAME[] = {
  { "TEST", "HED:JobController", 0, &JobControllerTestACC::GetInstance },
  { NULL, NULL, 0, NULL }
};
