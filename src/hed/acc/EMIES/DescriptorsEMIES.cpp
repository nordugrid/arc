// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/loader/Plugin.h>

#include "SubmitterPluginEMIES.h"
#include "JobControllerEMIES.h"
#include "JobListRetrieverPluginEMIES.h"
#include "TargetInformationRetrieverPluginEMIES.h"

Arc::PluginDescriptor PLUGINS_TABLE_NAME[] = {
  { "EMIES", "HED:SubmitterPlugin", "EMI-ES conforming execution service", 0, &Arc::SubmitterPluginEMIES::Instance },
  { "EMIES", "HED:JobController", "EMI-ES conforming execution service", 0, &Arc::JobControllerEMIES::Instance },
  { "EMIES", "HED:TargetInformationRetrieverPlugin", "", 0, &Arc::TargetInformationRetrieverPluginEMIES::Instance },
  { "EMIES", "HED:JobListRetrieverPlugin", "", 0, &Arc::JobListRetrieverPluginEMIES::Instance },
  { NULL, NULL, NULL, 0, NULL }
};
