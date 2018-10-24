// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/StringConv.h>

#include "JobListRetrieverPluginWSRFBES.h"

namespace Arc {

  Logger JobListRetrieverPluginWSRFBES::logger(Logger::getRootLogger(), "JobListRetrieverPlugin.WSRFBES");

  bool JobListRetrieverPluginWSRFBES::isEndpointNotSupported(const Endpoint& endpoint) const {
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
      service = "https://" + service + "/arex";
    } else {
      std::string proto = lower(service.substr(0,pos1));
      if((proto != "http") && (proto != "https")) return URL();
    }
    // Default port other than 443?
    // Default path?
    
    return service;
  }

  EndpointQueryingStatus JobListRetrieverPluginWSRFBES::Query(const UserConfig&, const Endpoint&, std::list<Job>&, const EndpointQueryOptions<Job>&) const {
    return EndpointQueryingStatus::FAILED;
  }

} // namespace Arc

extern Arc::PluginDescriptor const ARC_PLUGINS_TABLE_NAME[] = {
  { "WSRFBES", "HED:JobListRetrieverPlugin", "", 0, &Arc::JobListRetrieverPluginWSRFBES::Instance },
  { NULL, NULL, NULL, 0, NULL }
};
