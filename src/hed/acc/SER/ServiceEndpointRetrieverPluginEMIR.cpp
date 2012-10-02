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

  bool ServiceEndpointRetrieverPluginEMIR::isEndpointNotSupported(const Endpoint& endpoint) const {
    const std::string::size_type pos = endpoint.URLString.find("://");
    if (pos != std::string::npos) {
      const std::string proto = lower(endpoint.URLString.substr(0, pos));
      return ((proto != "http") && (proto != "https"));
    }
    
    return false;
  }

  static URL CreateURL(std::string service, int entryPerMessage, int skipOffset) {
    std::string::size_type pos1 = service.find("://");
    if (pos1 == std::string::npos) {
      service = "https://" + service;
    } else {
      std::string proto = lower(service.substr(0,pos1));
      if((proto != "http") && (proto != "https")) return URL();
    }
    // Default port other than 443?
    /* 
     *  URL structure and response format: 
     *    * in JSON: http[s]://<host_name>:<port>/services?limit=<nr_of_entries_per_message>[&skip=<offset>]
     *
     *    * in XML:  http[s]://<host_name>:<port>/services/query.xml?limit=<nr_of_entries_per_message>[&skip=<offset>]
     */
    std::string::size_type pos3 = service.find("/", pos1 + 3);
    if (pos3 == std::string::npos || pos3 == service.size()-1) {
      service += "/services/query.xml?limit=";
      std::stringstream ss;
      ss << entryPerMessage;
      service += ss.str();
      if (skipOffset > 0) {
        service += "&skip=";
        std::stringstream sskip;
        sskip << skipOffset;
        service += sskip.str();
      }
    }
    return service;
  }

  EndpointQueryingStatus ServiceEndpointRetrieverPluginEMIR::Query(const UserConfig& uc,
                                                                   const Endpoint& rEndpoint,
                                                                   std::list<Endpoint>& seList,
                                                                   const EndpointQueryOptions<Endpoint>&) const {
    EndpointQueryingStatus s(EndpointQueryingStatus::STARTED);
    
    int currentSkip = 0;
    MCCConfig cfg;
    uc.ApplyToConfig(cfg);
    while (true) {
      URL url(CreateURL(rEndpoint.URLString, maxEntries, currentSkip));
      if (!url) {
        s = EndpointQueryingStatus::FAILED;
        return s;
      }
      // increment the starting point of the fetched DB 
      currentSkip += maxEntries;

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
      if (services.empty()) {
        break;        // exit from the infinite loop, don't get more information from the EMIR server
      }
      for (XMLNodeList::const_iterator it = services.begin(); it != services.end(); ++it) {
        if (!(*it)["Endpoint"] || !(*it)["Endpoint"]["URL"]) {
          continue;
        }

        Endpoint se((std::string)(*it)["Endpoint"]["URL"]);
        for (XMLNode n = (*it)["Endpoint"]["Capability"]; n; ++n) {
          se.Capability.push_back((std::string)n);
        }
        se.InterfaceName = (std::string)(*it)["Endpoint"]["InterfaceName"];

        se.ServiceID = (std::string)(*it)["ID"];
        seList.push_back(se);
      }
      logger.msg(VERBOSE, "Found %u execution services from the index service at %s", resp_xml.Size(), url.str());
      if (http_response != NULL) {
        delete http_response;
      }
    }

    s = EndpointQueryingStatus::SUCCESSFUL;
    return s;
  }

} // namespace Arc
