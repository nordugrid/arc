// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "TargetRetrieverARC1.h"
#include "JobControllerARC1.h"
#include "SubmitterARC1.h"

Arc::PluginDescriptor PLUGINS_TABLE_NAME[] = {
  { "ARC1", "HED:TargetRetriever", 0, &Arc::TargetRetrieverARC1::Instance },
  { "ARC1", "HED:Submitter", 0, &Arc::SubmitterARC1::Instance },
  { "ARC1", "HED:JobController", 0, &Arc::JobControllerARC1::Instance },
  { NULL, NULL, 0, NULL }
};
