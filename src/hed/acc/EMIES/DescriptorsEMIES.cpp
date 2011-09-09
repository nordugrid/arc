// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/loader/Plugin.h>

#include "TargetRetrieverEMIES.h"
#include "SubmitterEMIES.h"
#include "JobControllerEMIES.h"

Arc::PluginDescriptor PLUGINS_TABLE_NAME[] = {
  { "EMIES", "HED:TargetRetriever", "EMI-ES resource information interface", 0, &Arc::TargetRetrieverEMIES::Instance },
  { "EMIES", "HED:Submitter", "EMI-ES conforming execution service", 0, &Arc::SubmitterEMIES::Instance },
  { "EMIES", "HED:JobController", "EMI-ES conforming execution service", 0, &Arc::JobControllerEMIES::Instance },
  { NULL, NULL, NULL, 0, NULL }
};
