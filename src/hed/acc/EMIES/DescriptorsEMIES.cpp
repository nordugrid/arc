// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/loader/Plugin.h>

#include "TargetRetrieverEMIES.h"
//#include "JobControllerEMIES.h"
//#include "SubmitterEMIES.h"

Arc::PluginDescriptor PLUGINS_TABLE_NAME[] = {
  { "EMIES", "HED:TargetRetriever", 0, &Arc::TargetRetrieverEMIES::Instance },
//  { "EMIES", "HED:Submitter", 0, &Arc::SubmitterEMIES::Instance },
//  { "EMIES", "HED:JobController", 0, &Arc::JobControllerEMIES::Instance },
  { NULL, NULL, 0, NULL }
};
