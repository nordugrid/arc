// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/compute/TestACCControl.h>

#include "ServiceEndpointRetrieverPluginTEST.h"

namespace Arc {

Plugin* ServiceEndpointRetrieverPluginTEST::Instance(PluginArgument* arg) {
  return new ServiceEndpointRetrieverPluginTEST(arg);
}

EndpointQueryingStatus ServiceEndpointRetrieverPluginTEST::Query(const UserConfig& userconfig,
                                                          const Endpoint& registry,
                                                          std::list<Endpoint>& endpoints,
                                                          const EndpointQueryOptions<Endpoint>&) const {
  Glib::usleep(ServiceEndpointRetrieverPluginTESTControl::delay*1000000);
  endpoints = ServiceEndpointRetrieverPluginTESTControl::endpoints;
  return ServiceEndpointRetrieverPluginTESTControl::status;
};


}
