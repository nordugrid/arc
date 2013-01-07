// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "JobDescriptionParserPluginTestACC.h"

namespace Arc {

Plugin* JobDescriptionParserPluginTestACC::GetInstance(PluginArgument *arg) {
  return new JobDescriptionParserPluginTestACC(arg);
}

}
