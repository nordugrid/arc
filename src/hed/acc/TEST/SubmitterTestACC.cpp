// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "SubmitterTestACC.h"

Arc::Plugin* SubmitterTestACC::GetInstance(Arc::PluginArgument *arg) {
  Arc::SubmitterPluginArgument *jcarg = dynamic_cast<Arc::SubmitterPluginArgument*>(arg);
  if (!jcarg) {
    return NULL;
  }
  return new SubmitterTestACC(*jcarg);
}
