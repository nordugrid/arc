// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/loader/Plugin.h>

#include "JobControllerARC0.h"
#include "SubmitterPluginARC0.h"

Arc::PluginDescriptor PLUGINS_TABLE_NAME[] = {
  { "ARC0", "HED:SubmitterPlugin", "ARCs classic Grid Manager", 0, &Arc::SubmitterPluginARC0::Instance },
  { "ARC0", "HED:JobController", "ARCs classic Grid Manager", 0, &Arc::JobControllerARC0::Instance },
  { NULL, NULL, NULL, 0, NULL }
};
