// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/Logger.h>
#include <arc/URL.h>
#include <arc/UserConfig.h>
#include <arc/client/ExecutionTarget.h>
#include <arc/client/EndpointQueryingStatus.h>

#include "TargetInformationRetrieverPluginBES.h"

namespace Arc {

  Logger TargetInformationRetrieverPluginBES::logger(Logger::getRootLogger(), "TargetInformationRetrieverPlugin.BES");

  /*
  static URL CreateURL(std::string service) {
    std::string::size_type pos1 = service.find("://");
    if (pos1 == std::string::npos) {
      service = "https://" + service;
    } else {
      std::string proto = lower(service.substr(0,pos1));
      if((proto != "http") && (proto != "https")) return URL();
    }
    // Default port other than 443?
    // Default path?
    return service;
  }
  */

  EndpointQueryingStatus TargetInformationRetrieverPluginBES::Query(const UserConfig& uc, const ComputingInfoEndpoint& cie, std::list<ExecutionTarget>& etList) const {
    EndpointQueryingStatus s(s = EndpointQueryingStatus::FAILED);

    URL url(cie.EndpointURL);

    if (!url) {
      return s;
    }

    // TODO: Need to open a remote connection in order to verify a running service.
    //if ( /* No service running at 'url' */ ) {
    // return s;
    //}

    ExecutionTarget target;
    //target.GridFlavour = ;
    target.Cluster = url;
    target.url = url;
    //target.InterfaceName = flavour;
    target.Implementor = "NorduGrid";
    target.DomainName = url.Host();
    target.HealthState = "ok";

    etList.push_back(target);

    s = EndpointQueryingStatus::SUCCESSFUL;
    return s;
  }

} // namespace Arc
