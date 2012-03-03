// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "JobControllerARC1.h"
#include "SubmitterARC1.h"
#include "JobControllerBES.h"
#include "SubmitterBES.h"

Arc::PluginDescriptor PLUGINS_TABLE_NAME[] = {
  { "ARC1", "HED:Submitter", "A-REX (ARC REsource coupled EXecution service)", 0, &Arc::SubmitterARC1::Instance },
  { "ARC1", "HED:JobController", "A-REX (ARC REsource coupled EXecution service)", 0, &Arc::JobControllerARC1::Instance },
  { "BES",  "HED:Submitter", "OGSA-BES conforming execution service", 0, &Arc::SubmitterBES::Instance },
  { "BES",  "HED:JobController", "OGSA-BES conforming execution service", 0, &Arc::JobControllerBES::Instance },
  { NULL, NULL, NULL, 0, NULL }
};
