// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "JobListRetrieverPluginARC1.h"
#include "JobListRetrieverPluginREST.h"
#include "JobControllerPluginARC1.h"
#include "JobControllerPluginREST.h"
#include "JobControllerPluginBES.h"
#include "SubmitterPluginARC1.h"
#include "SubmitterPluginREST.h"
#include "TargetInformationRetrieverPluginBES.h"
#include "TargetInformationRetrieverPluginWSRFGLUE2.h"
#include "TargetInformationRetrieverPluginREST.h"

extern Arc::PluginDescriptor const ARC_PLUGINS_TABLE_NAME[] = {
  { "WSRFGLUE2", "HED:JobListRetrieverPlugin", "", 0, &Arc::JobListRetrieverPluginARC1::Instance },
  { "REST",      "HED:JobListRetrieverPlugin", "ARC REST Job List Interface", 0, &Arc::JobListRetrieverPluginREST::Instance },

  { "ARC1", "HED:SubmitterPlugin", "A-REX (ARC REsource coupled EXecution service)", 0, &Arc::SubmitterPluginARC1::Instance },
  { "REST", "HED:SubmitterPlugin", "ARC REST Submission Interface for A-REX", 0, &Arc::SubmitterPluginREST::Instance },

  { "ARC1", "HED:JobControllerPlugin", "A-REX (ARC REsource coupled EXecution service)", 0, &Arc::JobControllerPluginARC1::Instance },
  { "BES",  "HED:JobControllerPlugin", "OGSA-BES conforming execution service", 0, &Arc::JobControllerPluginBES::Instance },
  { "REST", "HED:JobControllerPlugin", "ARC REST Job Control Interface", 0, &Arc::JobControllerPluginREST::Instance },

  { "BES",       "HED:TargetInformationRetrieverPlugin", "OGSA-BES Local Information", 0, &Arc::TargetInformationRetrieverPluginBES::Instance },
  { "WSRFGLUE2", "HED:TargetInformationRetrieverPlugin", "WSRF GLUE2 Local Information", 0, &Arc::TargetInformationRetrieverPluginWSRFGLUE2::Instance },
  { "REST",      "HED:TargetInformationRetrieverPlugin", "ARC REST GLUE2 Local Information", 0, &Arc::TargetInformationRetrieverPluginREST::Instance },
  { NULL, NULL, NULL, 0, NULL }
};
