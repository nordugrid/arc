// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "JobListRetrieverPluginWSRFGLUE2.h"
#include "JobControllerARC1.h"
#include "SubmitterARC1.h"
#include "JobControllerBES.h"
#include "SubmitterBES.h"
#include "TargetInformationRetrieverPluginBES.h"
#include "TargetInformationRetrieverPluginWSRFGLUE2.h"

Arc::PluginDescriptor PLUGINS_TABLE_NAME[] = {
  { "WSRFGLUE2", "HED:JobListRetrieverPlugin", "", 0, &Arc::JobListRetrieverPluginWSRFGLUE2::Instance },
  { "ARC1", "HED:Submitter", "A-REX (ARC REsource coupled EXecution service)", 0, &Arc::SubmitterARC1::Instance },
  { "ARC1", "HED:JobController", "A-REX (ARC REsource coupled EXecution service)", 0, &Arc::JobControllerARC1::Instance },
  { "BES",  "HED:Submitter", "OGSA-BES conforming execution service", 0, &Arc::SubmitterBES::Instance },
  { "BES",  "HED:JobController", "OGSA-BES conforming execution service", 0, &Arc::JobControllerBES::Instance },
  { "BES", "HED:TargetInformationRetrieverPlugin", "", 0, &Arc::TargetInformationRetrieverPluginBES::Instance },
  { "WSRFGLUE2", "HED:TargetInformationRetrieverPlugin", "", 0, &Arc::TargetInformationRetrieverPluginWSRFGLUE2::Instance },
  { NULL, NULL, NULL, 0, NULL }
};
