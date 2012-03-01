// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/Logger.h>
#include <arc/StringConv.h>
#include <arc/URL.h>
#include <arc/UserConfig.h>
#include <arc/client/ExecutionTarget.h>
#include <arc/client/EndpointQueryingStatus.h>

#include "TargetInformationRetrieverPluginBES.h"

namespace Arc {

  Logger TargetInformationRetrieverPluginBES::logger(Logger::getRootLogger(), "TargetInformationRetrieverPlugin.BES");

  bool TargetInformationRetrieverPluginBES::isEndpointNotSupported(const ComputingInfoEndpoint& endpoint) const {
    const std::string::size_type pos = endpoint.URLString.find("://");
    if (pos != std::string::npos) {
      const std::string proto = lower(endpoint.URLString.substr(0, pos));
      return ((proto != "http") && (proto != "https"));
    }
    
    return false;
  }

  static bool CreateURL(std::string service, URL& url) {
    std::string::size_type pos1 = service.find("://");
    if (pos1 == std::string::npos) {
      service = "https://" + service;
    } else {
      std::string proto = lower(service.substr(0,pos1));
      if((proto != "http") && (proto != "https")) return false;
    }
    // Default port other than 443?
    // Default path?
    
    url = service;
    return true;
  }

  EndpointQueryingStatus TargetInformationRetrieverPluginBES::Query(const UserConfig& uc, const ComputingInfoEndpoint& cie, std::list<ExecutionTarget>& etList, const EndpointQueryOptions<ExecutionTarget>&) const {
    EndpointQueryingStatus s(EndpointQueryingStatus::FAILED);
    // Return FAILED while the implementation is not complete
    return s;

    URL url(cie.URLString);

    if (!url) {
      return s;
    }

    // TODO: Need to open a remote connection in order to verify a running service.
    //if ( /* No service running at 'url' */ ) {
    // return s;
    //}
    

    ExecutionTarget target;
    target.GridFlavour = "BES"; // TODO: Use interface name instead.
    target.Cluster = url;
    target.ComputingEndpoint.URLString = url.str();
    //target.InterfaceName = flavour;
    target.ComputingEndpoint.Implementor = "NorduGrid";
    target.DomainName = url.Host();
    target.ComputingEndpoint.HealthState = "ok";

    etList.push_back(target);

    s = EndpointQueryingStatus::SUCCESSFUL;
    return s;
  }

} // namespace Arc
