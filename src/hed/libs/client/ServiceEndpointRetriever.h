#ifndef __ARC_SERVICEENDPOINTRETRIEVER_H__
#define __ARC_SERVICEENDPOINTRETRIEVER_H__

#include <arc/client/EndpointRetriever.h>

namespace Arc {

///
/**
 *
 **/
// Combination of the GLUE 2 Service and Endpoint classes.
class ServiceEndpoint {
public:
  ServiceEndpoint(std::string EndpointURL = "",
                  std::list<std::string> EndpointCapabilities = std::list<std::string>(),
                  std::string EndpointInterfaceName = "",
                  std::string HealthState = "",
                  std::string HealthStateInfo = "",
                  std::string QualityLevel = "")
    : EndpointURL(EndpointURL),
    EndpointCapabilities(EndpointCapabilities),
    EndpointInterfaceName(EndpointInterfaceName),
    HealthState(HealthState),
    HealthStateInfo(HealthStateInfo),
    QualityLevel(QualityLevel) {}

  std::string EndpointURL;
  std::list<std::string> EndpointCapabilities;
  std::string EndpointInterfaceName;
  std::string HealthState;
  std::string HealthStateInfo;
  std::string QualityLevel;
};

///
/**
 *
 **/
class RegistryEndpoint {
public:
  RegistryEndpoint(std::string Endpoint = "", std::string InterfaceName = "") : Endpoint(Endpoint), InterfaceName(InterfaceName) {}
  RegistryEndpoint(ServiceEndpoint service) : Endpoint(service.EndpointURL), InterfaceName(service.EndpointInterfaceName) {}

  static bool isRegistry(ServiceEndpoint service) {
    return (std::find(service.EndpointCapabilities.begin(), service.EndpointCapabilities.end(), RegistryCapability) != service.EndpointCapabilities.end());
  }

  std::string str() const {
    return Endpoint + " (" + (InterfaceName.empty() ? "<unspecified>" : InterfaceName) + ")";
  }

  // Needed for std::map ('statuses' in ServiceEndpointRetriever) to be able to sort the keys
  bool operator<(const RegistryEndpoint& other) const {
    return Endpoint + InterfaceName < other.Endpoint + other.InterfaceName;
  }

  std::string Endpoint;
  std::string InterfaceName;
  static const std::string RegistryCapability;
};

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

class ServiceEndpointRetrieverPluginTESTControl {
public:
  static float delay;
  static EndpointQueryingStatus status;
  static std::list<ServiceEndpoint> endpoints;
};

} // namespace Arc

#endif // __ARC_SERVICEENDPOINTRETRIEVER_H__
