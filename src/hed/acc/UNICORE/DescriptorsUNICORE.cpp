// -*- indent-tabs-mode: nil -*-

#include <arc/client/ACCLoader.h>

#include "TargetRetrieverUNICORE.h"
#include "JobControllerUNICORE.h"
#include "SubmitterUNICORE.h"

Arc::PluginDescriptor PLUGINS_TABLE_NAME[] = {
  { "TargetRetrieverUNICORE", "HED:ACC", 0, &Arc::TargetRetrieverUNICORE::Instance },
  { "SubmitterUNICORE", "HED:ACC", 0, &Arc::SubmitterUNICORE::Instance },
  { "JobControllerUNICORE", "HED:ACC", 0, &Arc::JobControllerUNICORE::Instance },
  { NULL, NULL, 0, NULL }
};
