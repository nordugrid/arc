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

  #define HTTP_OK (200)


  Logger TargetInformationRetrieverPluginREST::logger(Logger::getRootLogger(), "TargetInformationRetrieverPlugin.REST");

  Arc::EndpointQueryingStatus TargetInformationRetrieverPluginREST::Query(const Arc::UserConfig& uc, const Arc::Endpoint& cie, std::list<Arc::ComputingServiceType>& csList, const Arc::EndpointQueryOptions<Arc::ComputingServiceType>&) const {
    logger.msg(DEBUG, "Querying WSRF GLUE2 computing info endpoint.");

    Arc::URL url(CreateURL(cie.URLString));
    if (!url) {
      return EndpointQueryingStatus(EndpointQueryingStatus::FAILED,"URL "+cie.URLString+" can't be processed");
    }

    Arc::MCCConfig cfg;
    uc.ApplyToConfig(cfg);
    // Fetch from information sub-path
    Arc::URL infoUrl(url);
    infoUrl.ChangePath(infoUrl.Path()+"/*info");
    Arc::ClientHTTP client(cfg, infoUrl);
    Arc::PayloadRaw request;
    Arc::PayloadRawInterface* response(NULL);
    Arc::HTTPClientInfo info;
    Arc::MCC_Status res = client.process(std::string("GET"), &request, &info, &response);
    if(!res) {
      delete response;
      return Arc::EndpointQueryingStatus(EndpointQueryingStatus::FAILED,res.getExplanation());
    }
    if(info.code != HTTP_OK) {
      delete response;
      return Arc::EndpointQueryingStatus(EndpointQueryingStatus::FAILED, "Error "+Arc::tostring(info.code)+": "+info.reason);
    }
    if((response == NULL) || (response->Buffer(0) == NULL)) {
      delete response;
      return Arc::EndpointQueryingStatus(EndpointQueryingStatus::FAILED,"No response");
    }
    logger.msg(VERBOSE, "CONTENT %u: ", response->BufferSize(0), std::string(response->Buffer(0),response->BufferSize(0)));
    Arc::XMLNode servicesQueryResponse(response->Buffer(0),response->BufferSize(0));
    delete response;
    if(!servicesQueryResponse) {
      return Arc::EndpointQueryingStatus(EndpointQueryingStatus::FAILED,"Response is not XML");
    }

    //TargetInformationRetrieverPluginWSRFGLUE2::ExtractTargets(url, servicesQueryResponse["Domains"]["AdminDomain"]["Services"], csList);
    GLUE2::ParseExecutionTargets(servicesQueryResponse["Domains"]["AdminDomain"]["Services"], csList);
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

