// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/Logger.h>
#include <arc/StringConv.h>
#include <arc/URL.h>
#include <arc/XMLNode.h>
#include <arc/client/ClientInterface.h>
#include <arc/message/MCC.h>
#include <arc/message/MCC_Status.h>
#include <arc/message/PayloadRaw.h>

#include "ServiceEndpointRetrieverPluginEMIR.h"

namespace Arc {

  Logger ServiceEndpointRetrieverPluginEMIR::logger(Logger::getRootLogger(), "ServiceEndpointRetrieverPlugin.EMIR");

  bool ServiceEndpointRetrieverPluginEMIR::isEndpointNotSupported(const RegistryEndpoint& endpoint) const {
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
    // Default port other than 443?
    std::string::size_type pos3 = service.find("/", pos1 + 3);
    if (pos3 == std::string::npos || pos3 == service.size()-1) {
      service += "/services/query.xml";
    }
    return service;
  }

  EndpointQueryingStatus ServiceEndpointRetrieverPluginEMIR::Query(const UserConfig& uc,
                                                                   const RegistryEndpoint& rEndpoint,
                                                                   std::list<ServiceEndpoint>& seList,
                                                                   const EndpointQueryOptions<ServiceEndpoint>&) const {
    EndpointQueryingStatus s(EndpointQueryingStatus::STARTED);

    URL url(CreateURL(rEndpoint.URLString));
    if (!url) {
      s = EndpointQueryingStatus::FAILED;
      return s;
    }

    MCCConfig cfg;
    uc.ApplyToConfig(cfg);
    ClientHTTP httpclient(cfg, url);
    httpclient.RelativeURI(true);

    PayloadRaw http_request;
    PayloadRawInterface *http_response = NULL;
    HTTPClientInfo http_info;

    // send query message to the EMIRegistry
    MCC_Status status = httpclient.process("GET", &http_request, &http_info, &http_response);

    if (http_info.code != 200 || !status) {
      s = EndpointQueryingStatus::FAILED;
      return s;
    }

    XMLNode resp_xml(http_response->Content());
    XMLNodeList services = resp_xml.Path("Service");
    for (XMLNodeList::const_iterator it = services.begin(); it != services.end(); ++it) {
      if (!(*it)["Endpoint"] || !(*it)["Endpoint"]["URL"]) {
        continue;
      }

      ServiceEndpoint se((std::string)(*it)["Endpoint"]["URL"]);
      for (XMLNode n = (*it)["Endpoint"]["Capability"]; n; ++n) {
        se.Capability.push_back((std::string)n);
      }
      se.InterfaceName = (std::string)(*it)["Endpoint"]["InterfaceName"];

      seList.push_back(se);
    }
    logger.msg(VERBOSE, "Found %u execution services from the index service at %s", resp_xml.Size(), url.str());

    s = EndpointQueryingStatus::SUCCESSFUL;
    return s;
  }

} // namespace Arc
