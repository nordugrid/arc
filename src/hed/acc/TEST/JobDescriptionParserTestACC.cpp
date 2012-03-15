// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "JobDescriptionParserTestACC.h"

namespace Arc {

Plugin* JobDescriptionParserTestACC::GetInstance(PluginArgument *arg) {
  return new JobDescriptionParserTestACC(arg);
}

}
