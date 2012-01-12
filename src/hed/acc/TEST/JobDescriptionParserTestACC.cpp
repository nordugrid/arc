// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "JobDescriptionParserTestACC.h"

Arc::Plugin* JobDescriptionParserTestACC::GetInstance(Arc::PluginArgument *arg) {
  return new JobDescriptionParserTestACC();
}
