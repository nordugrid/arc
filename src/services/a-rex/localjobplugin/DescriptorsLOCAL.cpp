// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/loader/Plugin.h>

#include "SubmitterPluginLOCAL.h"
#include "JobControllerPluginLOCAL.h"
#include "JobListRetrieverPluginLOCAL.h"
#include "TargetInformationRetrieverPluginLOCAL.h"

extern Arc::PluginDescriptor const ARC_PLUGINS_TABLE_NAME[] = {
  { "LOCAL", "HED:SubmitterPlugin", "LOCAL execution service", 0, &ARexLOCAL::SubmitterPluginLOCAL::Instance },
  { "LOCAL", "HED:JobControllerPlugin", "LOCAL execution service", 0, &ARexLOCAL::JobControllerPluginLOCAL::Instance },
  { "LOCAL", "HED:TargetInformationRetrieverPlugin", "LOCAL execution service", 0, &ARexLOCAL::TargetInformationRetrieverPluginLOCAL::Instance },
  { "LOCAL", "HED:JobListRetrieverPlugin", "LOCAL execution service", 0, &ARexLOCAL::JobListRetrieverPluginLOCAL::Instance },
  { NULL, NULL, NULL, 0, NULL }
};
