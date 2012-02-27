// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/client/TestACCControl.h>

#include "ServiceEndpointRetrieverPluginTEST.h"

namespace Arc {

Plugin* ServiceEndpointRetrieverPluginTEST::Instance(PluginArgument* arg) {
  return new ServiceEndpointRetrieverPluginTEST();
}

EndpointQueryingStatus ServiceEndpointRetrieverPluginTEST::Query(const UserConfig& userconfig,
                                                          const RegistryEndpoint& registry,
                                                          std::list<ServiceEndpoint>& endpoints,
                                                          const EndpointFilter<ServiceEndpoint>&) const {
  Glib::usleep(ServiceEndpointRetrieverPluginTESTControl::delay*1000000);
  endpoints = ServiceEndpointRetrieverPluginTESTControl::endpoints;
  return ServiceEndpointRetrieverPluginTESTControl::status;
};


}
