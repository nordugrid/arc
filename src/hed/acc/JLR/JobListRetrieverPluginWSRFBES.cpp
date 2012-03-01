// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/StringConv.h>

#include "JobListRetrieverPluginWSRFBES.h"

namespace Arc {

  Logger JobListRetrieverPluginWSRFBES::logger(Logger::getRootLogger(), "JobListRetrieverPlugin.WSRFBES");

  bool JobListRetrieverPluginWSRFBES::isEndpointNotSupported(const ComputingInfoEndpoint& endpoint) const {
    const std::string::size_type pos = endpoint.URLString.find("://");
    if (pos != std::string::npos) {
      const std::string proto = lower(endpoint.URLString.substr(0, pos));
      return ((proto != "http") && (proto != "https"));
    }
    
    return false;
  }

  static bool CreateURL(std::string service, URL& url) {
    std::string::size_type pos1 = service.find("://");
    if (pos1 == std::string::npos) {
      service = "https://" + service;
    } else {
      std::string proto = lower(service.substr(0,pos1));
      if((proto != "http") && (proto != "https")) return false;
    }
    // Default port other than 443?
    // Default path?
    
    url = service;
    return true;
  }

  EndpointQueryingStatus JobListRetrieverPluginWSRFBES::Query(const UserConfig&, const ComputingInfoEndpoint&, std::list<Job>&, const EndpointQueryOptions<Job>&) const {
    return EndpointQueryingStatus::FAILED;
  }

} // namespace Arc
