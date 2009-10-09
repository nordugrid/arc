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
  { "ARC1", "HED:TargetRetriever", 0, &Arc::TargetRetrieverARC1::Instance },
  { "ARC1", "HED:Submitter", 0, &Arc::SubmitterARC1::Instance },
  { "ARC1", "HED:JobController", 0, &Arc::JobControllerARC1::Instance },
  { "BES",  "HED:TargetRetriever", 0, &Arc::TargetRetrieverBES::Instance },
  { "BES",  "HED:Submitter", 0, &Arc::SubmitterBES::Instance },
  { "BES",  "HED:JobController", 0, &Arc::JobControllerBES::Instance },
  { NULL, NULL, 0, NULL }
};
