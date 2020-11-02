// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "JobListRetrieverPluginREST.h"
#include "JobControllerPluginREST.h"
#include "SubmitterPluginREST.h"
#include "TargetInformationRetrieverPluginREST.h"

extern Arc::PluginDescriptor const ARC_PLUGINS_TABLE_NAME[] = {
  { "REST", "HED:JobListRetrieverPlugin", "ARC REST Job List Interface", 0, &Arc::JobListRetrieverPluginREST::Instance },
  { "REST", "HED:SubmitterPlugin", "ARC REST Submission Interface for A-REX", 0, &Arc::SubmitterPluginREST::Instance },
  { "REST", "HED:JobControllerPlugin", "ARC REST Job Control Interface", 0, &Arc::JobControllerPluginREST::Instance },
  { "REST", "HED:TargetInformationRetrieverPlugin", "ARC REST GLUE2 Local Information", 0, &Arc::TargetInformationRetrieverPluginREST::Instance },
  { NULL, NULL, NULL, 0, NULL }
};
