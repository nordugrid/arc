// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "JobListRetrieverPluginWSRFBES.h"

namespace Arc {

  Logger JobListRetrieverPluginWSRFBES::logger(Logger::getRootLogger(), "JobListRetrieverPlugin.WSRFBES");

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

  EndpointQueryingStatus JobListRetrieverPluginWSRFBES::Query(const UserConfig&, const ComputingInfoEndpoint&, std::list<Job>&, const EndpointFilter<Job>&) const {
    return EndpointQueryingStatus::FAILED;
  }

} // namespace Arc
