// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "JobControllerTestACC.h"

namespace Arc {

Plugin* JobControllerTestACC::GetInstance(PluginArgument *arg) {
  JobControllerPluginArgument *jcarg = dynamic_cast<JobControllerPluginArgument*>(arg);
  if (!jcarg) {
    return NULL;
  }
  return new JobControllerTestACC(*jcarg);
}

}
