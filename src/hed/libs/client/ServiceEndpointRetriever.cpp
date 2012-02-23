// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "ServiceEndpointRetriever.h"

namespace Arc {

  const std::string RegistryEndpoint::RegistryCapability = "information.discovery.registry";

  template<>
  const std::string EndpointRetrieverPlugin<RegistryEndpoint, ServiceEndpoint>::kind("HED:ServiceEndpointRetrieverPlugin");

  template<>
  Logger EndpointRetrieverPluginLoader<RegistryEndpoint, ServiceEndpoint>::logger(Logger::getRootLogger(), "ServiceEndpointRetrieverPluginLoader");

  template<>
  Logger EndpointRetriever<RegistryEndpoint, ServiceEndpoint>::logger(Logger::getRootLogger(), "ServiceEndpointRetriever");

  void ServiceEndpointRetriever::addEndpoint(const ServiceEndpoint& endpoint) {
    if (filter.recursiveEnabled() && RegistryEndpoint::isRegistry(endpoint)) {
      RegistryEndpoint registry(endpoint);
      logger.msg(Arc::DEBUG, "Found a registry, will query it recursively: %s", registry.str());
      EndpointRetriever<RegistryEndpoint, ServiceEndpoint>::addEndpoint(registry);
    }

    bool match = false;
    for (std::list<std::string>::const_iterator it = filter.getCapabilityFilter().begin(); it != filter.getCapabilityFilter().end(); it++) {
      if (std::find(endpoint.EndpointCapabilities.begin(), endpoint.EndpointCapabilities.end(), *it) != endpoint.EndpointCapabilities.end()) {
        match = true;
        break;
      }
    }
    if (filter.getCapabilityFilter().empty() || match) {
      EndpointRetriever<RegistryEndpoint, ServiceEndpoint>::addEndpoint(endpoint);
    }
  }

  float ServiceEndpointRetrieverPluginTESTControl::delay = 0;
  EndpointQueryingStatus ServiceEndpointRetrieverPluginTESTControl::status;
  std::list<ServiceEndpoint> ServiceEndpointRetrieverPluginTESTControl::endpoints = std::list<ServiceEndpoint>();

} // namespace Arc
