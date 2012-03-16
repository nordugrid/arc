// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/Logger.h>
#include <arc/StringConv.h>
#include <arc/URL.h>
#include <arc/UserConfig.h>
#include <arc/client/EndpointQueryingStatus.h>
#include <arc/client/ExecutionTarget.h>
#include <arc/client/GLUE2.h>
#include <arc/message/MCC.h>

#include "JobStateEMIES.h"
#include "EMIESClient.h"
#include "TargetInformationRetrieverPluginEMIES.h"

namespace Arc {

  Logger TargetInformationRetrieverPluginEMIES::logger(Logger::getRootLogger(), "TargetInformationRetrieverPlugin.EMIES");

  bool TargetInformationRetrieverPluginEMIES::isEndpointNotSupported(const Endpoint& endpoint) const {
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
      service = "https://" + service;
    } else {
      std::string proto = lower(service.substr(0,pos1));
      if((proto != "http") && (proto != "https")) return URL();
    }

    return service;
  }

  EndpointQueryingStatus TargetInformationRetrieverPluginEMIES::Query(const UserConfig& uc, const Endpoint& cie, std::list<ComputingServiceType>& csList, const EndpointQueryOptions<ComputingServiceType>&) const {
    EndpointQueryingStatus s(EndpointQueryingStatus::FAILED);

    URL url(CreateURL(cie.URLString));
    if (!url) {
      return s;
    }

    logger.msg(DEBUG, "Collecting EMI-ES GLUE2 computing info endpoint information.");
    MCCConfig cfg;
    uc.ApplyToConfig(cfg);
    EMIESClient ac(url, cfg, uc.Timeout());
    XMLNode servicesQueryResponse;
    if (!ac.sstat(servicesQueryResponse)) {
      return s;
    }

    ExtractTargets(url, servicesQueryResponse, csList);

    if (!csList.empty()) s = EndpointQueryingStatus::SUCCESSFUL;
    return s;
  }

  void TargetInformationRetrieverPluginEMIES::ExtractTargets(const URL& url, XMLNode response, std::list<ComputingServiceType>& csList) {
    logger.msg(VERBOSE, "Generating EMIES targets");
    GLUE2::ParseExecutionTargets(response, csList);
    for(std::list<ComputingServiceType>::iterator cs = csList.begin(); cs != csList.end(); ++cs) {
      if(!((*cs)->Cluster)) (*cs)->Cluster = url;
      for (std::map<int, ComputingEndpointType>::iterator ce = cs->ComputingEndpoint.begin();
           ce != cs->ComputingEndpoint.end(); ++ce) {
        if(ce->second->URLString.empty()) ce->second->URLString = url.str();
        if(ce->second->InterfaceName.empty()) ce->second->InterfaceName = "org.ogf.emies";
      }
      if(cs->AdminDomain->Name.empty()) cs->AdminDomain->Name = url.Host();
      logger.msg(VERBOSE, "Generated EMIES target: %s", (*cs)->Cluster.str());
    }
  }

} // namespace Arc
