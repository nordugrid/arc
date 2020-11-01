// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/Logger.h>
#include <arc/StringConv.h>
#include <arc/URL.h>
#include <arc/UserConfig.h>
#include <arc/message/MCC.h>
#include <arc/compute/ExecutionTarget.h>
#include <arc/compute/EndpointQueryingStatus.h>
#include <arc/compute/GLUE2.h>
#include <arc/communication/ClientInterface.h>

#include "TargetInformationRetrieverPluginREST.h"

namespace Arc {

  using namespace Arc;


  Logger TargetInformationRetrieverPluginREST::logger(Logger::getRootLogger(), "TargetInformationRetrieverPlugin.REST");

  Arc::EndpointQueryingStatus TargetInformationRetrieverPluginREST::Query(const Arc::UserConfig& uc, const Arc::Endpoint& cie, std::list<Arc::ComputingServiceType>& csList, const Arc::EndpointQueryOptions<Arc::ComputingServiceType>&) const {
    logger.msg(DEBUG, "Querying WSRF GLUE2 computing info endpoint.");

    // TODO: autoversion
    URL url(cie.URLString);
    if (!url) {
      return EndpointQueryingStatus(EndpointQueryingStatus::FAILED,"URL "+cie.URLString+" can't be processed");
    }

    Arc::MCCConfig cfg;
    uc.ApplyToConfig(cfg);
    // Fetch from information sub-path
    Arc::URL infoUrl(url);
    infoUrl.ChangePath(infoUrl.Path()+"/rest/1.0/info");
    infoUrl.AddOption("schema=glue2",false);
    Arc::ClientHTTP client(cfg, infoUrl);
    Arc::PayloadRaw request;
    Arc::PayloadRawInterface* response(NULL);
    Arc::HTTPClientInfo info;
    std::multimap<std::string,std::string> attributes;
    attributes.insert(std::pair<std::string, std::string>("Accept", "text/xml"));
    Arc::MCC_Status res = client.process(std::string("GET"), attributes, &request, &info, &response);
    if(!res) {
      delete response;
      return Arc::EndpointQueryingStatus(EndpointQueryingStatus::FAILED,res.getExplanation());
    }
    if(info.code != 200) {
      delete response;
      return Arc::EndpointQueryingStatus(EndpointQueryingStatus::FAILED, "Error "+Arc::tostring(info.code)+": "+info.reason);
    }
    if((response == NULL) || (response->Buffer(0) == NULL)) {
      delete response;
      return Arc::EndpointQueryingStatus(EndpointQueryingStatus::FAILED,"No response");
    }
    logger.msg(VERBOSE, "CONTENT %u: %s", response->BufferSize(0), std::string(response->Buffer(0),response->BufferSize(0)));
    Arc::XMLNode servicesQueryResponse(response->Buffer(0),response->BufferSize(0));
    delete response;
    if(!servicesQueryResponse) {
      logger.msg(VERBOSE, "Response is not XML");
      return Arc::EndpointQueryingStatus(EndpointQueryingStatus::FAILED,"Response is not XML");
    }

    GLUE2::ParseExecutionTargets(servicesQueryResponse["Domains"]["AdminDomain"]["Services"], csList);
    logger.msg(VERBOSE, "Parsed domains: %u",csList.size());
    for(std::list<Arc::ComputingServiceType>::iterator cs = csList.begin(); cs != csList.end(); ++cs) {
      cs->AdminDomain->Name = url.Host();
    }

    for (std::list<ComputingServiceType>::iterator it = csList.begin(); it != csList.end(); it++) {
      (*it)->InformationOriginEndpoint = cie;
    }

    if (!csList.empty()) return Arc::EndpointQueryingStatus(EndpointQueryingStatus::SUCCESSFUL);
    return Arc::EndpointQueryingStatus(EndpointQueryingStatus::FAILED,"Query returned no endpoints");
  }

} // namespace Arc

