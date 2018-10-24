// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <utility>

#include <arc/Logger.h>
#include <arc/StringConv.h>
#include <arc/URL.h>
#include <arc/XMLNode.h>
#include <arc/communication/ClientInterface.h>
#include <arc/message/MCC.h>
#include <arc/message/MCC_Status.h>
#include <arc/message/PayloadRaw.h>

#include "ServiceEndpointRetrieverPluginEMIR.h"

namespace Arc {

  Logger ServiceEndpointRetrieverPluginEMIR::logger(Logger::getRootLogger(), "ServiceEndpointRetrieverPlugin.EMIR");

  EndpointQueryingStatus ServiceEndpointRetrieverPluginEMIR::Query(const UserConfig& uc,
                                                                   const Endpoint& rEndpoint,
                                                                   std::list<Endpoint>& seList,
                                                                   const EndpointQueryOptions<Endpoint>&) const {
    EndpointQueryingStatus s(EndpointQueryingStatus::STARTED);
    
    int currentSkip = 0;
    MCCConfig cfg;
    uc.ApplyToConfig(cfg);
    
    if (isEndpointNotSupported(rEndpoint)) {
      return EndpointQueryingStatus::FAILED;
    }
    URL url((rEndpoint.URLString.find("://") == std::string::npos ? "http://" : "") + rEndpoint.URLString, false, 9126, "/services/query.xml");
    if (!url) {
      s = EndpointQueryingStatus::FAILED;
      return s;
    }

    // Limitation: Max number of services the below loop will fetch, according to parameters:
    // ServiceEndpointRetrieverPluginEMIR::maxEntries * 100 = 500.000 (currently)
    for (int iAvoidInfiniteLoop = 0; iAvoidInfiniteLoop < 100; ++iAvoidInfiniteLoop) {
      if (maxEntries > 0)  url.AddHTTPOption("limit", tostring(maxEntries),  true);
      if (currentSkip > 0) url.AddHTTPOption("skip",  tostring(currentSkip), true);

      // increment the starting point of the fetched DB 
      currentSkip += maxEntries;

      ClientHTTP httpclient(cfg, url);
      httpclient.RelativeURI(true);

      // Set Accept HTTP header tag, in order not to receive JSON output.
      std::multimap<std::string, std::string> acceptHeaderTag;
      acceptHeaderTag.insert(std::make_pair("Accept", "text/xml"));

      PayloadRaw http_request;
      PayloadRawInterface *http_response = NULL;
      HTTPClientInfo http_info;

      // send query message to the EMIRegistry
      MCC_Status status = httpclient.process("GET", acceptHeaderTag, &http_request, &http_info, &http_response);

      if (http_info.code != 200 || !status) {
        s = EndpointQueryingStatus::FAILED;
        return s;
      }
      
      XMLNode resp_xml(http_response->Content());
      XMLNodeList services = resp_xml.Path("Service");
      if (services.empty()) {
        delete http_response;
        break; // No more services - exit from loop.
      }
      for (XMLNodeList::const_iterator it = services.begin(); it != services.end(); ++it) {
        if (!(*it)["Endpoint"] || !(*it)["Endpoint"]["URL"]) {
          continue;
        }

        Endpoint se((std::string)(*it)["Endpoint"]["URL"]);
        for (XMLNode n = (*it)["Endpoint"]["Capability"]; n; ++n) {
          se.Capability.insert((std::string)n);
        }
        se.InterfaceName = lower((std::string)(*it)["Endpoint"]["InterfaceName"]);

        se.ServiceID = (std::string)(*it)["ID"];
        seList.push_back(se);
      }
      logger.msg(VERBOSE, "Found %u service endpoints from the index service at %s", resp_xml.Size(), url.str());
      if (http_response != NULL) {
        delete http_response;
      }
    }

    s = EndpointQueryingStatus::SUCCESSFUL;
    return s;
  }

} // namespace Arc
