// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/client/EndpointQueryingStatus.h>
#include <arc/loader/Plugin.h>

#include "ServiceEndpointRetrieverPluginEGIIS.h"
#include "ServiceEndpointRetrieverPluginEMIR.h"

Arc::PluginDescriptor PLUGINS_TABLE_NAME[] = {
  { "EGIIS", "HED:ServiceEndpointRetrieverPlugin", "", 0, &Arc::ServiceEndpointRetrieverPluginEGIIS::Instance },
  { "EMIR", "HED:ServiceEndpointRetrieverPlugin", "", 0, &Arc::ServiceEndpointRetrieverPluginEMIR::Instance },
  { NULL, NULL, NULL, 0, NULL }
};
