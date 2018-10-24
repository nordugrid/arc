// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/Logger.h>
#include <arc/StringConv.h>
#include <arc/URL.h>
#include <arc/UserConfig.h>
#include <arc/compute/ExecutionTarget.h>
#include <arc/compute/EndpointQueryingStatus.h>

#include "TargetInformationRetrieverPluginBES.h"

namespace Arc {

  Logger TargetInformationRetrieverPluginBES::logger(Logger::getRootLogger(), "TargetInformationRetrieverPlugin.BES");

  bool TargetInformationRetrieverPluginBES::isEndpointNotSupported(const Endpoint& endpoint) const {
    const std::string::size_type pos = endpoint.URLString.find("://");
    if (pos != std::string::npos) {
      const std::string proto = lower(endpoint.URLString.substr(0, pos));
      return ((proto != "http") && (proto != "https"));
    }

    return false;
  }

  static URL CreateURL(std::string service) {
    std::string::size_type pos1 = service.find("://");
    if (pos1 == std::string::npos) {
      service = "https://" + service + "/arex";
    } else {
      std::string proto = lower(service.substr(0,pos1));
      if((proto != "http") && (proto != "https")) return URL();
    }
    // Default port other than 443?
    // Default path?

    return service;
  }

  EndpointQueryingStatus TargetInformationRetrieverPluginBES::Query(const UserConfig& uc, const Endpoint& cie, std::list<ComputingServiceType>& csList, const EndpointQueryOptions<ComputingServiceType>&) const {
    EndpointQueryingStatus s(EndpointQueryingStatus::FAILED);
    // Return FAILED while the implementation is not complete
    return s;

    URL url(CreateURL(cie.URLString));

    if (!url) {
      return s;
    }

    // TODO: Need to open a remote connection in order to verify a running service.
    //if ( /* No service running at 'url' */ ) {
    // return s;
    //}


    ComputingServiceType cs;
    cs.AdminDomain->Name = url.Host();

    ComputingEndpointType ComputingEndpoint;
    ComputingEndpoint->URLString = url.str();
    ComputingEndpoint->InterfaceName = "org.ogf.bes";
    ComputingEndpoint->Implementor = "NorduGrid";
    ComputingEndpoint->HealthState = "ok";

    cs.ComputingEndpoint.insert(std::pair<int, ComputingEndpointType>(0, ComputingEndpoint));
    // TODO: ComputingServiceType object must be filled with ComputingManager, ComputingShare and ExecutionEnvironment before it is valid.

    csList.push_back(cs);

    s = EndpointQueryingStatus::SUCCESSFUL;
    return s;
  }

} // namespace Arc
