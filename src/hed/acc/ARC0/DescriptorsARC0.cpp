// -*- indent-tabs-mode: nil -*-

#include <arc/loader/Plugin.h>
#include <arc/client/ACCLoader.h>

#include "TargetRetrieverARC0.h"
#include "JobControllerARC0.h"
#include "SubmitterARC0.h"

Arc::PluginDescriptor PLUGINS_TABLE_NAME[] = {
  { "TargetRetrieverARC0", "HED:ACC", 0, &Arc::TargetRetrieverARC0::Instance },
  { "SubmitterARC0", "HED:ACC", 0, &Arc::SubmitterARC0::Instance },
  { "JobControllerARC0", "HED:ACC", 0, &Arc::JobControllerARC0::Instance },
  { NULL, NULL, 0, NULL }
};
