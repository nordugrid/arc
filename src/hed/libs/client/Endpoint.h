#ifndef __ARC_ENDPOINT_H__
#define __ARC_ENDPOINT_H__

#include <string>
#include <list>
#include <algorithm>

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

class ComputingInfoEndpoint {
public:
  ComputingInfoEndpoint(std::string Endpoint = "", std::string InterfaceName = "") : Endpoint(Endpoint), InterfaceName(InterfaceName) {}
  ComputingInfoEndpoint(ServiceEndpoint service) : Endpoint(service.EndpointURL), InterfaceName(service.EndpointInterfaceName) {}

  static bool isComputingInfo(ServiceEndpoint service) {
    return (std::find(service.EndpointCapabilities.begin(), service.EndpointCapabilities.end(), ComputingInfoCapability) != service.EndpointCapabilities.end());
  }

  std::string str() const {
    return Endpoint + " (" + (InterfaceName.empty() ? "<unspecified>" : InterfaceName) + ")";
  }

  // Needed for std::map ('statuses' in TargetInformationRetriever) to be able to sort the keys
  bool operator<(const ComputingInfoEndpoint& other) const {
    return Endpoint + InterfaceName < other.Endpoint + other.InterfaceName;
  }

  std::string Endpoint;
  std::string InterfaceName;
  static const std::string ComputingInfoCapability;
};

} // namespace Arc

#endif // __ARC_ENDPOINT_H__
