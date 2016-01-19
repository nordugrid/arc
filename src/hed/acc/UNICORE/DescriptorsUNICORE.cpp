// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "JobControllerPluginUNICORE.h"
#include "SubmitterPluginUNICORE.h"

extern Arc::PluginDescriptor const ARC_PLUGINS_TABLE_NAME[] = {
  { "UNICORE", "HED:SubmitterPlugin", NULL, 0, &Arc::SubmitterPluginUNICORE::Instance },
  { "UNICORE", "HED:JobControllerPlugin", NULL, 0, &Arc::JobControllerPluginUNICORE::Instance },
  { NULL, NULL, NULL, 0, NULL }
};
