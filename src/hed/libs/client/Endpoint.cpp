// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/UserConfig.h>
#include <arc/client/ExecutionTarget.h>

#include "Endpoint.h"

namespace Arc {

  Endpoint::Endpoint(const ExecutionTarget& e, const std::string& rsi)
    : URLString(e.ComputingEndpoint->URLString), InterfaceName(e.ComputingEndpoint->InterfaceName),
      HealthState(e.ComputingEndpoint->HealthState), HealthStateInfo(e.ComputingEndpoint->HealthStateInfo),
      QualityLevel(e.ComputingEndpoint->QualityLevel), Capability(e.ComputingEndpoint->Capability),
      RequestedSubmissionInterfaceName(rsi) {}

  Endpoint::Endpoint(const ComputingEndpointAttributes& cea, const std::string& rsi)
    : URLString(cea.URLString), InterfaceName(cea.InterfaceName),
      HealthState(cea.HealthState), HealthStateInfo(cea.HealthStateInfo),
      QualityLevel(cea.QualityLevel), Capability(cea.Capability),
      RequestedSubmissionInterfaceName(rsi) {}

  Endpoint& Endpoint::operator=(const ConfigEndpoint& e) {
    URLString = e.URLString;
    InterfaceName = e.InterfaceName;
    RequestedSubmissionInterfaceName = e.RequestedSubmissionInterfaceName;
    
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

  std::string Endpoint::getServiceName() const {
    if (URLString.find("://") == std::string::npos)
      return URLString;
    else {
      URL url(URLString);
      if (url.Host().empty()) return URLString;
      else return url.Host();
    }
  }
  
  std::string Endpoint::str() const {
    std::string capabilities = "";
    if (!Capability.empty()) {
      capabilities = ", capabilities:";
      for (std::list<std::string>::const_iterator it = Capability.begin(); it != Capability.end(); it++) {
        capabilities = capabilities + " " + *it;        
      }
    }
    std::string interfaceNameToPrint = "<empty InterfaceName>";
    if (!InterfaceName.empty()) {
      interfaceNameToPrint = InterfaceName;
    }
    return URLString + " (" + interfaceNameToPrint + capabilities + ")";
  }
  
  bool Endpoint::operator<(const Endpoint& other) const {
    return str() < other.str();
  }

} // namespace Arc

