#ifndef __ARC_ENDPOINT_H__
#define __ARC_ENDPOINT_H__

#include <string>
#include <list>
#include <algorithm>

namespace Arc {

class ConfigEndpoint;
  
class Endpoint {
public:
  enum CapabilityEnum { REGISTRY, COMPUTINGINFO, JOBLIST, JOBSUBMIT, JOBMANAGEMENT, ANY };
  
  static std::string GetStringForCapability(Endpoint::CapabilityEnum cap) {
    if (cap == Endpoint::REGISTRY) return "information.discovery.registry";
    if (cap == Endpoint::COMPUTINGINFO) return "information.discovery.resource";
    if (cap == Endpoint::JOBLIST) return "information.discovery.resource";
    if (cap == Endpoint::JOBSUBMIT) return "executionmanagement.jobexecution";
    if (cap == Endpoint::JOBMANAGEMENT) return "executionmanagement.jobmanager";
    return "";
  }
  
  Endpoint(const std::string& URLString = "",
           const std::string& InterfaceName = "",
           const std::list<std::string>& Capability = std::list<std::string>())
    : URLString(URLString), InterfaceName(InterfaceName), Capability(Capability) {}
  
  // This will call operator=
  Endpoint(const ConfigEndpoint& e) { *this = e; }
  
  bool HasCapability(Endpoint::CapabilityEnum cap) const;
  
  bool HasCapability(std::string) const;

  std::string str() const;
  
  // Needed for std::map to be able to sort the keys
  bool operator<(const Endpoint& other) const;
  
  Endpoint& operator=(const ConfigEndpoint& e);
  
  std::string URLString;
  std::string InterfaceName;  
  std::string HealthState;
  std::string HealthStateInfo;
  std::string QualityLevel;
  std::list<std::string> Capability;
  std::string PreferredJobInterfaceName;
};

} // namespace Arc

#endif // __ARC_ENDPOINT_H__
