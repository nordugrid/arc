// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "SubmitterPluginCREAM.h"
#include "JobControllerPluginCREAM.h"

Arc::PluginDescriptor PLUGINS_TABLE_NAME[] = {
  { "CREAM", "HED:SubmitterPlugin", "The Computing Resource Execution And Management service", 0, &Arc::SubmitterPluginCREAM::Instance },
  { "CREAM", "HED:JobControllerPlugin", "The Computing Resource Execution And Management service", 0, &Arc::JobControllerPluginCREAM::Instance },
  { NULL, NULL, NULL, 0, NULL }
};
