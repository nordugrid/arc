// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "TargetRetrieverUNICORE.h"
#include "JobControllerUNICORE.h"
#include "SubmitterUNICORE.h"

Arc::PluginDescriptor PLUGINS_TABLE_NAME[] = {
  { "UNICORE", "HED:TargetRetriever", NULL, 0, &Arc::TargetRetrieverUNICORE::Instance },
  { "UNICORE", "HED:Submitter", NULL, 0, &Arc::SubmitterUNICORE::Instance },
  { "UNICORE", "HED:JobController", NULL, 0, &Arc::JobControllerUNICORE::Instance },
  { NULL, NULL, NULL, 0, NULL }
};
