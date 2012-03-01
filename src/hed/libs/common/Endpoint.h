#ifndef __ARC_ENDPOINT_H__
#define __ARC_ENDPOINT_H__

#include <string>
#include <list>
#include <algorithm>

namespace Arc {
  
class Endpoint {
public:
  enum EndpointType { REGISTRY, COMPUTINGINFO, JOBSUBMIT, JOBMANAGEMENT, ANY };
  
  Endpoint(const std::string& URLString = "",
           const std::string& InterfaceName = "",
           const std::list<std::string>& Capability = std::list<std::string>())
    : URLString(URLString), InterfaceName(InterfaceName), Capability(Capability) {}
  
  std::string str() const {
    return URLString + " (" + (InterfaceName.empty() ? "<unspecified>" : InterfaceName) + ")";
  }
  
  // Needed for std::map ('statuses' in ServiceEndpointRetriever) to be able to sort the keys
  bool operator<(const Endpoint& other) const {
    return str() < other.str();
  }
  
  std::string URLString;
  std::string InterfaceName;  
  std::string HealthState;
  std::string HealthStateInfo;
  std::string QualityLevel;
  std::list<std::string> Capability;
  std::string PreferredJobInterfaceName;
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
                  std::list<std::string> Capability = std::list<std::string>())
    : Endpoint(URLString, InterfaceName, Capability) {}
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
    return (std::find(service.Capability.begin(), service.Capability.end(), RegistryCapability) != service.Capability.end());
  }

  static const std::string RegistryCapability;
};

class ComputingInfoEndpoint : public Endpoint {
public:
  ComputingInfoEndpoint(std::string URLString = "", std::string InterfaceName = "") : Endpoint(URLString, InterfaceName) {}
  ComputingInfoEndpoint(ServiceEndpoint service) : Endpoint(service.URLString, service.InterfaceName) {}

  static bool isComputingInfo(ServiceEndpoint service) {
    return (std::find(service.Capability.begin(), service.Capability.end(), ComputingInfoCapability) != service.Capability.end());
  }

  static const std::string ComputingInfoCapability;
};

} // namespace Arc

#endif // __ARC_ENDPOINT_H__
