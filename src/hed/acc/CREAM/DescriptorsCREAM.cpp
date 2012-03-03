// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "SubmitterCREAM.h"
#include "JobControllerCREAM.h"

Arc::PluginDescriptor PLUGINS_TABLE_NAME[] = {
  { "CREAM", "HED:Submitter", "The Computing Resource Execution And Management service", 0, &Arc::SubmitterCREAM::Instance },
  { "CREAM", "HED:JobController", "The Computing Resource Execution And Management service", 0, &Arc::JobControllerCREAM::Instance },
  { NULL, NULL, NULL, 0, NULL }
};
