// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "SubmitterTestACC.h"

namespace Arc {

Plugin* SubmitterTestACC::GetInstance(PluginArgument *arg) {
  SubmitterPluginArgument *jcarg = dynamic_cast<SubmitterPluginArgument*>(arg);
  if (!jcarg) {
    return NULL;
  }
  return new SubmitterTestACC(*jcarg,arg);
}

}
