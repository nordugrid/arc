// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/Logger.h>
#include <arc/URL.h>
#include <arc/XMLNode.h>
#include <arc/client/ClientInterface.h>
#include <arc/message/MCC.h>
#include <arc/message/MCC_Status.h>
#include <arc/message/PayloadRaw.h>

#include "ServiceEndpointRetrieverPluginEMIR.h"

namespace Arc {

  Logger ServiceEndpointRetrieverPluginEMIR::logger(Logger::getRootLogger(), "ServiceEndpointRetrieverPlugin.EMIR");

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

  EndpointQueryingStatus ServiceEndpointRetrieverPluginEMIR::Query(const UserConfig& uc,
                                                                   const RegistryEndpoint& rEndpoint,
                                                                   std::list<ServiceEndpoint>& seList,
                                                                   const EndpointQueryOptions<ServiceEndpoint>&) const {
    EndpointQueryingStatus s(EndpointQueryingStatus::STARTED);

    URL url(rEndpoint.Endpoint + "/services/query.xml");
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
        se.EndpointCapabilities.push_back((std::string)n);
      }
      se.EndpointInterfaceName = (std::string)(*it)["Endpoint"]["InterfaceName"];

      seList.push_back(se);
    }
    logger.msg(VERBOSE, "Found %u execution services from the index service at %s", resp_xml.Size(), url.str());

    s = EndpointQueryingStatus::SUCCESSFUL;
    return s;
  }

} // namespace Arc
