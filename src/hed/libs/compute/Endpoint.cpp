// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <utility>

#include <arc/UserConfig.h>
#include <arc/compute/EndpointQueryingStatus.h>
#include <arc/compute/ExecutionTarget.h>

#include "Endpoint.h"

namespace Arc {

  EndpointStatusMap::EndpointStatusMap() : std::map<Endpoint, EndpointQueryingStatus, EndpointCompareFn>(&Endpoint::ServiceIDCompare) {}

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
      Capability.insert(GetStringForCapability(Endpoint::REGISTRY));
      break;
    case ConfigEndpoint::COMPUTINGINFO:
      Capability.insert(GetStringForCapability(Endpoint::COMPUTINGINFO));
      break;
    case ConfigEndpoint::ANY:
      break;
    }
    
    return *this;
  }
  
  bool Endpoint::HasCapability(Endpoint::CapabilityEnum cap) const {
    return HasCapability(GetStringForCapability(cap));
  }
  
  bool Endpoint::HasCapability(const std::string& capability) const {
    return Capability.count(capability);
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
      for (std::set<std::string>::const_iterator it = Capability.begin(); it != Capability.end(); it++) {
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

  bool Endpoint::ServiceIDCompare(const Endpoint& a, const Endpoint& b) {
    if (a.ServiceID != b.ServiceID) {
      return a.ServiceID < b.ServiceID;
    }
    
    if (a.URLString != b.URLString) {
      return a.URLString < b.URLString;
    }
    
    return a.InterfaceName < b.InterfaceName;
  }

  std::pair<EndpointStatusMap::const_iterator, EndpointStatusMap::const_iterator> Endpoint::getServiceEndpoints(const Endpoint& e, const EndpointStatusMap& m) {
    Endpoint _e(e);
    EndpointStatusMap::const_iterator first  = m.lower_bound(_e);
    _e.ServiceID[_e.ServiceID.size()-1]++;
    EndpointStatusMap::const_iterator second = m.upper_bound(_e);
    return make_pair(first, second);
  }

} // namespace Arc

