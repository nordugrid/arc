// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "JobListRetrieverPluginWSRFCREAM.h"

namespace Arc {

  Logger JobListRetrieverPluginWSRFCREAM::logger(Logger::getRootLogger(), "JobListRetrieverPlugin.WSRFCREAM");

  /*
  static URL CreateURL(std::string service) {
    std::string::size_type pos1 = service.find("://");
    if (pos1 == std::string::npos) {
      service = "ldap://" + service;
      pos1 = 4;
    } else {
      if(lower(service.substr(0,pos1)) != "ldap") return URL();
    }
    std::string::size_type pos2 = service.find(":", pos1 + 3);
    std::string::size_type pos3 = service.find("/", pos1 + 3);
    if (pos3 == std::string::npos) {
      if (pos2 == std::string::npos)
        service += ":2170";
      // Is this a good default path?
      // Different for computing and index?
      service += "/o=Grid";
    }
    else if (pos2 == std::string::npos || pos2 > pos3)
      service.insert(pos3, ":2170");
    return service;
  }
  */

  EndpointQueryingStatus JobListRetrieverPluginWSRFCREAM::Query(const UserConfig&, const ComputingInfoEndpoint&, std::list<Job>&, const EndpointFilter<Job>&) const {
    return EndpointQueryingStatus::FAILED;
  }

} // namespace Arc
