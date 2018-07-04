// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/loader/Plugin.h>

#include "SubmitterPluginINTERNAL.h"
#include "JobControllerPluginINTERNAL.h"
#include "JobListRetrieverPluginINTERNAL.h"
#include "TargetInformationRetrieverPluginINTERNAL.h"

extern Arc::PluginDescriptor const ARC_PLUGINS_TABLE_NAME[] = {
  { "INTERNAL", "HED:SubmitterPlugin", "INTERNAL execution service", 0, &ARexINTERNAL::SubmitterPluginINTERNAL::Instance },
  { "INTERNAL", "HED:JobControllerPlugin", "INTERNAL execution service", 0, &ARexINTERNAL::JobControllerPluginINTERNAL::Instance },
  { "INTERNAL", "HED:TargetInformationRetrieverPlugin", "INTERNAL execution service", 0, &ARexINTERNAL::TargetInformationRetrieverPluginINTERNAL::Instance },
  { "INTERNAL", "HED:JobListRetrieverPlugin", "INTERNAL execution service", 0, &ARexINTERNAL::JobListRetrieverPluginINTERNAL::Instance },
  { NULL, NULL, NULL, 0, NULL }
};
