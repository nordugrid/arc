// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/loader/Plugin.h>

#include "TargetRetrieverARC0.h"
#include "JobControllerARC0.h"
#include "SubmitterARC0.h"

Arc::PluginDescriptor PLUGINS_TABLE_NAME[] = {
  { "ARC0", "HED:TargetRetriever", 0, &Arc::TargetRetrieverARC0::Instance },
  { "ARC0", "HED:Submitter", 0, &Arc::SubmitterARC0::Instance },
  { "ARC0", "HED:JobController", 0, &Arc::JobControllerARC0::Instance },
  { NULL, NULL, 0, NULL }
};
