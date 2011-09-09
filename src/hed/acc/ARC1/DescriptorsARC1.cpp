// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "TargetRetrieverARC1.h"
#include "JobControllerARC1.h"
#include "SubmitterARC1.h"
#include "TargetRetrieverBES.h"
#include "JobControllerBES.h"
#include "SubmitterBES.h"

Arc::PluginDescriptor PLUGINS_TABLE_NAME[] = {
  { "ARC1", "HED:TargetRetriever", "WSRF information service and ISIS", 0, &Arc::TargetRetrieverARC1::Instance },
  { "ARC1", "HED:Submitter", "A-REX (ARC REsource coupled EXecution service)", 0, &Arc::SubmitterARC1::Instance },
  { "ARC1", "HED:JobController", "A-REX (ARC REsource coupled EXecution service)", 0, &Arc::JobControllerARC1::Instance },
  { "BES",  "HED:TargetRetriever", "OGSA-BES management attributes retrieval", 0, &Arc::TargetRetrieverBES::Instance },
  { "BES",  "HED:Submitter", "OGSA-BES conforming execution service", 0, &Arc::SubmitterBES::Instance },
  { "BES",  "HED:JobController", "OGSA-BES conforming execution service", 0, &Arc::JobControllerBES::Instance },
  { NULL, NULL, NULL, 0, NULL }
};
