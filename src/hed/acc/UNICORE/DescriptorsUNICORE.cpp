// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "JobControllerUNICORE.h"
#include "SubmitterPluginUNICORE.h"

Arc::PluginDescriptor PLUGINS_TABLE_NAME[] = {
  { "UNICORE", "HED:SubmitterPlugin", NULL, 0, &Arc::SubmitterPluginUNICORE::Instance },
  { "UNICORE", "HED:JobController", NULL, 0, &Arc::JobControllerUNICORE::Instance },
  { NULL, NULL, NULL, 0, NULL }
};
