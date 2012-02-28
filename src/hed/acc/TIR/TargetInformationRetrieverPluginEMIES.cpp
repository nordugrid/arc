// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/Logger.h>
#include <arc/URL.h>
#include <arc/UserConfig.h>
#include <arc/client/EndpointQueryingStatus.h>
#include <arc/client/ExecutionTarget.h>
#include <arc/client/GLUE2.h>
#include <arc/message/MCC.h>

// Temporary solution
#include "../EMIES/JobStateEMIES.cpp"
#include "../EMIES/EMIESClient.cpp"

#include "TargetInformationRetrieverPluginEMIES.h"

namespace Arc {

  Logger TargetInformationRetrieverPluginEMIES::logger(Logger::getRootLogger(), "TargetInformationRetrieverPlugin.EMIES");

  /*
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
  */

  EndpointQueryingStatus TargetInformationRetrieverPluginEMIES::Query(const UserConfig& uc, const ComputingInfoEndpoint& cie, std::list<ExecutionTarget>& etList, const EndpointQueryOptions<ExecutionTarget>&) const {
    EndpointQueryingStatus s(EndpointQueryingStatus::FAILED);

    URL url(cie.Endpoint);
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

    ExtractTargets(url, servicesQueryResponse, etList);

    if (!etList.empty()) s = EndpointQueryingStatus::SUCCESSFUL;
    return s;
  }

  void TargetInformationRetrieverPluginEMIES::ExtractTargets(const URL& url, XMLNode response, std::list<ExecutionTarget>& targets) {
    targets.clear();
    logger.msg(VERBOSE, "Generating EMIES targets");
    GLUE2::ParseExecutionTargets(response, targets, "EMI-ES");
    GLUE2::ParseExecutionTargets(response, targets, "org.ogf.emies");
    for(std::list<ExecutionTarget>::iterator target = targets.begin();
                           target != targets.end(); ++target) {
      if(target->GridFlavour.empty()) target->GridFlavour = "EMIES"; // ?
      if(!(target->Cluster)) target->Cluster = url;
      if(!(target->url)) target->url = url;
      if(target->InterfaceName.empty()) target->InterfaceName = "EMI-ES";
      if(target->DomainName.empty()) target->DomainName = url.Host();
      logger.msg(VERBOSE, "Generated EMIES target: %s", target->Cluster.str());
    }
  }

} // namespace Arc
