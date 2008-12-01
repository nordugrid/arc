#include <arc/client/ACCLoader.h>

#include "SubmitterCREAM.h"
#include "TargetRetrieverCREAM.h"
#include "JobControllerCREAM.h"

Arc::PluginDescriptor PLUGINS_TABLE_NAME[] = {
  { "SubmitterCREAM", "HED:ACC", 0, &Arc::SubmitterCREAM::Instance },
  { "TargetRetrieverCREAM", "HED:ACC", 0, &Arc::TargetRetrieverCREAM::Instance },
  { "JobControllerCREAM", "HED:ACC", 0, &Arc::JobControllerCREAM::Instance },
  { NULL, NULL, 0, NULL }
};
