// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/UserConfig.h>

#include "Endpoint.h"

namespace Arc {

  Endpoint& Endpoint::operator=(const ConfigEndpoint& e) {
    URLString = e.URLString;
    InterfaceName = e.InterfaceName;
    PreferredJobInterfaceName = e.PreferredJobInterfaceName;
    
    Capability.clear();
    switch (e.type) {
    case ConfigEndpoint::REGISTRY:
      Capability.push_back(GetStringForCapability(Endpoint::REGISTRY));
      break;
    case ConfigEndpoint::COMPUTINGINFO:
      Capability.push_back(GetStringForCapability(Endpoint::COMPUTINGINFO));
      break;
    }
    
    return *this;
  }
  
  bool Endpoint::HasCapability(Endpoint::CapabilityEnum cap) const {
    return HasCapability(GetStringForCapability(cap));
  }
  
  bool Endpoint::HasCapability(std::string capability) const {
    return (std::find(Capability.begin(), Capability.end(), capability) != Capability.end());
  }
  
  std::string Endpoint::str() const {
    std::string mainCapability = "<no Capability>";
    size_t lastDotPos;
    if (!Capability.empty()) {
      mainCapability = Capability.front();
      lastDotPos = mainCapability.rfind(".");
      if (lastDotPos != mainCapability.npos) {
        mainCapability = mainCapability.substr(lastDotPos+1);
      }      
    }
    std::string strippedInterfaceName = "<empty InterfaceName>";
    if (!InterfaceName.empty()) {
      strippedInterfaceName = InterfaceName;
      lastDotPos = strippedInterfaceName.rfind(".");
      if (lastDotPos != strippedInterfaceName.npos) {
        strippedInterfaceName = strippedInterfaceName.substr(lastDotPos+1);
      }
    }
    return URLString + " (" + mainCapability + ", " + strippedInterfaceName + ")";
  }
  
  bool Endpoint::operator<(const Endpoint& other) const {
    return str() < other.str();
  }

} // namespace Arc

