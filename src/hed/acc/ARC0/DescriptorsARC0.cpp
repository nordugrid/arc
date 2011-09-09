// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/loader/Plugin.h>

#include "TargetRetrieverARC0.h"
#include "JobControllerARC0.h"
#include "SubmitterARC0.h"

Arc::PluginDescriptor PLUGINS_TABLE_NAME[] = {
  { "ARC0", "HED:TargetRetriever", "Classic ARC EGIIS and ARIS", 0, &Arc::TargetRetrieverARC0::Instance },
  { "ARC0", "HED:Submitter", "ARCs classic Grid Manager", 0, &Arc::SubmitterARC0::Instance },
  { "ARC0", "HED:JobController", "ARCs classic Grid Manager", 0, &Arc::JobControllerARC0::Instance },
  { NULL, NULL, NULL, 0, NULL }
};
