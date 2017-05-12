// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/compute/EndpointQueryingStatus.h>
#include <arc/loader/Plugin.h>

#include "ServiceEndpointRetrieverPluginEMIR.h"
#ifdef HAVE_LDNS
#include "ServiceEndpointRetrieverPluginARCHERY.h"
#endif

extern Arc::PluginDescriptor const ARC_PLUGINS_TABLE_NAME[] = {
  { "EMIR", "HED:ServiceEndpointRetrieverPlugin", "EMIR registry", 0, &Arc::ServiceEndpointRetrieverPluginEMIR::Instance },
#ifdef HAVE_LDNS
  { "ARCHERY", "HED:ServiceEndpointRetrieverPlugin", "ARC Hierarchical Endpoints DNS-based Registry", 0, &Arc::ServiceEndpointRetrieverPluginARCHERY::Instance },
#endif
  { NULL, NULL, NULL, 0, NULL }
};
