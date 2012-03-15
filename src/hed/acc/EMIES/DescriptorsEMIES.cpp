// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/loader/Plugin.h>

#include "SubmitterEMIES.h"
#include "JobControllerEMIES.h"
#include "TargetInformationRetrieverPluginEMIES.h"

Arc::PluginDescriptor PLUGINS_TABLE_NAME[] = {
  { "EMIES", "HED:Submitter", "EMI-ES conforming execution service", 0, &Arc::SubmitterEMIES::Instance },
  { "EMIES", "HED:JobController", "EMI-ES conforming execution service", 0, &Arc::JobControllerEMIES::Instance },
  { "EMIES", "HED:TargetInformationRetrieverPlugin", "", 0, &Arc::TargetInformationRetrieverPluginEMIES::Instance },
  { NULL, NULL, NULL, 0, NULL }
};
