// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "JobListRetrieverPluginARC1.h"
#include "JobControllerPluginARC1.h"
#include "SubmitterPluginARC1.h"
#include "JobControllerPluginBES.h"
#include "TargetInformationRetrieverPluginBES.h"
#include "TargetInformationRetrieverPluginWSRFGLUE2.h"

Arc::PluginDescriptor PLUGINS_TABLE_NAME[] = {
  { "WSRFGLUE2", "HED:JobListRetrieverPlugin", "", 0, &Arc::JobListRetrieverPluginARC1::Instance },
  { "ARC1", "HED:SubmitterPlugin", "A-REX (ARC REsource coupled EXecution service)", 0, &Arc::SubmitterPluginARC1::Instance },
  { "ARC1", "HED:JobControllerPlugin", "A-REX (ARC REsource coupled EXecution service)", 0, &Arc::JobControllerPluginARC1::Instance },
  { "BES",  "HED:JobControllerPlugin", "OGSA-BES conforming execution service", 0, &Arc::JobControllerPluginBES::Instance },
  { "BES", "HED:TargetInformationRetrieverPlugin", "OGSA-BES Local Information", 0, &Arc::TargetInformationRetrieverPluginBES::Instance },
  { "WSRFGLUE2", "HED:TargetInformationRetrieverPlugin", "WSRF GLUE2 Local Information", 0, &Arc::TargetInformationRetrieverPluginWSRFGLUE2::Instance },
  { NULL, NULL, NULL, 0, NULL }
};
