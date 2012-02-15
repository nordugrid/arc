// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "ServiceEndpointRetrieverPluginTEST.h"

namespace Arc {

Plugin* ServiceEndpointRetrieverTEST::Instance(PluginArgument* arg) {
  return new ServiceEndpointRetrieverTEST();
}

RegistryEndpointStatus ServiceEndpointRetrieverTEST::Query(const UserConfig& userconfig,
                                                          const RegistryEndpoint& registry,
                                                          std::list<ServiceEndpoint>& endpoints) const {
  Glib::usleep(ServiceEndpointRetrieverTESTControl::delay*1000000);
  endpoints = ServiceEndpointRetrieverTESTControl::endpoints;
  return ServiceEndpointRetrieverTESTControl::status;
};


}

Arc::PluginDescriptor PLUGINS_TABLE_NAME[] = {
  { "TEST", "HED:ServiceEndpointRetrieverPlugin", "ServiceEndpointRetriever test plugin", 0, &Arc::ServiceEndpointRetrieverTEST::Instance },
  { NULL, NULL, NULL, 0, NULL }
};
