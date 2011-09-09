// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/loader/Plugin.h>

#include "TargetRetrieverEMIREG.h"

Arc::PluginDescriptor PLUGINS_TABLE_NAME[] = {
  { "EMIREG", "HED:TargetRetriever", "EMI Registry service", 0, &Arc::TargetRetrieverEMIREG::Instance },
  { NULL, NULL, NULL, 0, NULL }
};
