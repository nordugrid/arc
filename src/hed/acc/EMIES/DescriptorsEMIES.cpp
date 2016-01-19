// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/loader/Plugin.h>

#include "SubmitterPluginEMIES.h"
#include "JobControllerPluginEMIES.h"
#include "JobListRetrieverPluginEMIES.h"
#include "TargetInformationRetrieverPluginEMIES.h"

extern Arc::PluginDescriptor const ARC_PLUGINS_TABLE_NAME[] = {
  { "EMIES", "HED:SubmitterPlugin", "EMI-ES conforming execution service", 0, &Arc::SubmitterPluginEMIES::Instance },
  { "EMIES", "HED:JobControllerPlugin", "EMI-ES conforming execution service", 0, &Arc::JobControllerPluginEMIES::Instance },
  { "EMIES", "HED:TargetInformationRetrieverPlugin", "EMI-ES conforming execution service", 0, &Arc::TargetInformationRetrieverPluginEMIES::Instance },
  { "EMIES", "HED:JobListRetrieverPlugin", "EMI-ES conforming execution service", 0, &Arc::JobListRetrieverPluginEMIES::Instance },
  { NULL, NULL, NULL, 0, NULL }
};
