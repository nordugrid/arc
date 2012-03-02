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
    switch (e.t) {
    case ConfigEndpoint::REGISTRY:
      Capability.push_back(RegistryEndpoint::RegistryCapability);
      break;
    case ConfigEndpoint::COMPUTINGINFO:
      Capability.push_back(ComputingInfoEndpoint::ComputingInfoCapability);
      break;
    }
    
    return *this;
  }


  const std::string RegistryEndpoint::RegistryCapability = "information.discovery.registry";
  const std::string ComputingInfoEndpoint::ComputingInfoCapability = "information.discovery.resource";

} // namespace Arc

