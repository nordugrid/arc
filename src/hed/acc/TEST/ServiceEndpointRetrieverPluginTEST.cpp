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
  if (!ServiceEndpointRetrieverPluginTESTControl::condition.empty()) {
    SimpleCondition* c = ServiceEndpointRetrieverPluginTESTControl::condition.front();
    ServiceEndpointRetrieverPluginTESTControl::condition.pop_front();
    if (c != NULL) {
      c->wait();
    }
  }
  if (!ServiceEndpointRetrieverPluginTESTControl::endpoints.empty()) {
    endpoints = ServiceEndpointRetrieverPluginTESTControl::endpoints.front();
    ServiceEndpointRetrieverPluginTESTControl::endpoints.pop_front();
  }
  if (!ServiceEndpointRetrieverPluginTESTControl::status.empty()) {
    EndpointQueryingStatus s = ServiceEndpointRetrieverPluginTESTControl::status.front();
    ServiceEndpointRetrieverPluginTESTControl::status.pop_front();
    return s;
  }
  return EndpointQueryingStatus::UNKNOWN;
};


}
