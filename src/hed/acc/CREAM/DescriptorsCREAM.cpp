// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "SubmitterCREAM.h"
#include "TargetRetrieverCREAM.h"
#include "JobControllerCREAM.h"

Arc::PluginDescriptor PLUGINS_TABLE_NAME[] = {
  { "CREAM", "HED:Submitter", 0, &Arc::SubmitterCREAM::Instance },
  { "CREAM", "HED:TargetRetriever", 0, &Arc::TargetRetrieverCREAM::Instance },
  { "CREAM", "HED:JobController", 0, &Arc::JobControllerCREAM::Instance },
  { NULL, NULL, 0, NULL }
};
