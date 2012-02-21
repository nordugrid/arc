// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/client/EndpointQueryingStatus.h>

#include "ServiceEndpointRetrieverPluginTEST.h"

namespace Arc {

Plugin* ServiceEndpointRetrieverTEST::Instance(PluginArgument* arg) {
  return new ServiceEndpointRetrieverTEST();
}

EndpointQueryingStatus ServiceEndpointRetrieverTEST::Query(const UserConfig& userconfig,
                                                          const RegistryEndpoint& registry,
                                                          std::list<ServiceEndpoint>& endpoints,
                                                          const std::list<std::string>& capabilityFilter) const {
  Glib::usleep(ServiceEndpointRetrieverPluginTESTControl::delay*1000000);
  endpoints = ServiceEndpointRetrieverPluginTESTControl::endpoints;
  return ServiceEndpointRetrieverPluginTESTControl::status;
};


}

Arc::PluginDescriptor PLUGINS_TABLE_NAME[] = {
  { "TEST", "HED:ServiceEndpointRetrieverPlugin", "ServiceEndpointRetriever test plugin", 0, &Arc::ServiceEndpointRetrieverTEST::Instance },
  { NULL, NULL, NULL, 0, NULL }
};
