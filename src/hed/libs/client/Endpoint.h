#ifndef __ARC_ENDPOINT_H__
#define __ARC_ENDPOINT_H__

#include <string>
#include <list>
#include <algorithm>

namespace Arc {
  
class Endpoint {
public:
  enum EndpointType { REGISTRY, COMPUTINGINFO, JOBSUBMIT, JOBMANAGEMENT };
  
  Endpoint(std::string URLString = "", std::string InterfaceName = "") : URLString(URLString), InterfaceName(InterfaceName) {}
  
  std::string str() const {
    return URLString + " (" + (InterfaceName.empty() ? "<unspecified>" : InterfaceName) + ")";
  }
  
  // Needed for std::map ('statuses' in ServiceEndpointRetriever) to be able to sort the keys
  bool operator<(const Endpoint& other) const {
    return str() < other.str();
  }
  
  std::string URLString;
  std::string InterfaceName;  
};

///
/**
 *
 **/
// Combination of the GLUE 2 Service and Endpoint classes.
class ServiceEndpoint : public Endpoint {
public:
  ServiceEndpoint(std::string URLString = "",
                  std::string InterfaceName = "",
                  std::list<std::string> EndpointCapabilities = std::list<std::string>())
    : Endpoint(URLString, InterfaceName), EndpointCapabilities(EndpointCapabilities) {}

  std::list<std::string> EndpointCapabilities;
  std::string HealthState;
  std::string HealthStateInfo;
  std::string QualityLevel;
};

///
/**
 *
 **/
class RegistryEndpoint : public Endpoint {
public:
  RegistryEndpoint(std::string URLString = "", std::string InterfaceName = "") : Endpoint(URLString, InterfaceName) {}
  RegistryEndpoint(ServiceEndpoint service) : Endpoint(service.URLString, service.InterfaceName) {}

  static bool isRegistry(ServiceEndpoint service) {
    return (std::find(service.EndpointCapabilities.begin(), service.EndpointCapabilities.end(), RegistryCapability) != service.EndpointCapabilities.end());
  }

  static const std::string RegistryCapability;
};

class ComputingInfoEndpoint : public Endpoint {
public:
  ComputingInfoEndpoint(std::string URLString = "", std::string InterfaceName = "") : Endpoint(URLString, InterfaceName) {}
  ComputingInfoEndpoint(ServiceEndpoint service) : Endpoint(service.URLString, service.InterfaceName) {}

  static bool isComputingInfo(ServiceEndpoint service) {
    return (std::find(service.EndpointCapabilities.begin(), service.EndpointCapabilities.end(), ComputingInfoCapability) != service.EndpointCapabilities.end());
  }

  static const std::string ComputingInfoCapability;
};

} // namespace Arc

#endif // __ARC_ENDPOINT_H__
