// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "SubmitterPluginCREAM.h"
#include "JobControllerPluginCREAM.h"
#include "JobListRetrieverPluginWSRFCREAM.h"

Arc::PluginDescriptor ARC_PLUGINS_TABLE_NAME[] = {
  { "CREAM", "HED:SubmitterPlugin", "The Computing Resource Execution And Management service", 0, &Arc::SubmitterPluginCREAM::Instance },
  { "CREAM", "HED:JobControllerPlugin", "The Computing Resource Execution And Management service", 0, &Arc::JobControllerPluginCREAM::Instance },
  { "CREAM", "HED:JobListRetrieverPlugin", "The Computing Resource Execution And Management service", 0, &Arc::JobListRetrieverPluginWSRFCREAM::Instance },
  { NULL, NULL, NULL, 0, NULL }
};
