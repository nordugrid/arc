#ifndef __ARC_SERVICEENDPOINTRETRIEVER_H__
#define __ARC_SERVICEENDPOINTRETRIEVER_H__

#include <arc/client/Endpoint.h>
#include <arc/client/EndpointRetriever.h>

namespace Arc {

template<>
class EndpointFilter<ServiceEndpoint> {
public:
  EndpointFilter(bool recursive = false, const std::list<std::string>& capabilityFilter = std::list<std::string>()) : recursive(recursive), capabilityFilter(capabilityFilter) {}

  bool recursiveEnabled() const { return recursive; }
  const std::list<std::string>& getCapabilityFilter() const { return capabilityFilter; }

private:
  bool recursive;
  std::list<std::string> capabilityFilter;
};

class ServiceEndpointRetriever : public EndpointRetriever<RegistryEndpoint, ServiceEndpoint> {
public:
  ServiceEndpointRetriever(const UserConfig& uc, const EndpointFilter<ServiceEndpoint>& filter = EndpointFilter<ServiceEndpoint>()) : EndpointRetriever<RegistryEndpoint, ServiceEndpoint>(uc, filter) {}

  void addEndpoint(const ServiceEndpoint& endpoint);
  void addEndpoint(const RegistryEndpoint& endpoint) { EndpointRetriever<RegistryEndpoint, ServiceEndpoint>::addEndpoint(endpoint); }
};

} // namespace Arc

#endif // __ARC_SERVICEENDPOINTRETRIEVER_H__
