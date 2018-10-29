// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/Logger.h>
#include <arc/StringConv.h>
#include <arc/URL.h>
#include <arc/UserConfig.h>
#include <arc/compute/EndpointQueryingStatus.h>
#include <arc/compute/ExecutionTarget.h>
#include <arc/compute/GLUE2.h>
#include <arc/message/MCC.h>

#include "JobStateEMIES.h"
#include "EMIESClient.h"
#include "TargetInformationRetrieverPluginEMIES.h"

namespace Arc {

  Logger TargetInformationRetrieverPluginEMIES::logger(Logger::getRootLogger(), "TargetInformationRetrieverPlugin.EMIES");

  EndpointQueryingStatus TargetInformationRetrieverPluginEMIES::Query(const UserConfig& uc, const Endpoint& cie, std::list<ComputingServiceType>& csList, const EndpointQueryOptions<ComputingServiceType>&) const {
    URL url(CreateURL(cie.URLString));
    if (!url) {
      return EndpointQueryingStatus::FAILED;
    }

    logger.msg(DEBUG, "Collecting EMI-ES GLUE2 computing info endpoint information.");
    MCCConfig cfg;
    uc.ApplyToConfig(cfg);
    EMIESClient ac(url, cfg, uc.Timeout());
    XMLNode servicesQueryResponse;
    if (!ac.sstat(servicesQueryResponse)) {
      return EndpointQueryingStatus(EndpointQueryingStatus::FAILED, ac.failure());
    }

    ExtractTargets(url, servicesQueryResponse, csList);
    
    for (std::list<ComputingServiceType>::iterator it = csList.begin(); it != csList.end(); it++) {
      (*it)->InformationOriginEndpoint = cie;
    }

    if (csList.empty()) return EndpointQueryingStatus::FAILED;
    return EndpointQueryingStatus::SUCCESSFUL;
  }

  void TargetInformationRetrieverPluginEMIES::ExtractTargets(const URL& url, XMLNode response, std::list<ComputingServiceType>& csList) {
    logger.msg(VERBOSE, "Generating EMIES targets");
    GLUE2::ParseExecutionTargets(response, csList);
    for(std::list<ComputingServiceType>::iterator cs = csList.begin(); cs != csList.end(); ++cs) {
      for (std::map<int, ComputingEndpointType>::iterator ce = cs->ComputingEndpoint.begin();
           ce != cs->ComputingEndpoint.end(); ++ce) {
        if(ce->second->URLString.empty()) ce->second->URLString = url.str();
        if(ce->second->InterfaceName.empty()) ce->second->InterfaceName = "org.ogf.glue.emies.activitycreation";
      }
      if(cs->AdminDomain->Name.empty()) cs->AdminDomain->Name = url.Host();
      logger.msg(VERBOSE, "Generated EMIES target: %s", cs->AdminDomain->Name);
    }
  }

} // namespace Arc
