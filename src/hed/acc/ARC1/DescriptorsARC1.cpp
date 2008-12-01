#include <arc/client/ACCLoader.h>

#include "TargetRetrieverARC1.h"
#include "JobControllerARC1.h"
#include "SubmitterARC1.h"

Arc::PluginDescriptor PLUGINS_TABLE_NAME[] = {
  { "TargetRetrieverARC1", "HED:ACC", 0, &Arc::TargetRetrieverARC1::Instance },
  { "SubmitterARC1", "HED:ACC", 0, &Arc::SubmitterARC1::Instance },
  { "JobControllerARC1", "HED:ACC", 0, &Arc::JobControllerARC1::Instance },
  { NULL, NULL, 0, NULL }
};
