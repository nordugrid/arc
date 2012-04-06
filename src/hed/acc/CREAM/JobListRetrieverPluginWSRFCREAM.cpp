// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/StringConv.h>

#include "JobListRetrieverPluginWSRFCREAM.h"

namespace Arc {

  Logger JobListRetrieverPluginWSRFCREAM::logger(Logger::getRootLogger(), "JobListRetrieverPlugin.WSRFCREAM");

  bool JobListRetrieverPluginWSRFCREAM::isEndpointNotSupported(const Endpoint& endpoint) const {
    const std::string::size_type pos = endpoint.URLString.find("://");
    return pos != std::string::npos && lower(endpoint.URLString.substr(0, pos)) != "ldap";
  }

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
      service += "/o=Grid";
    }
    else if (pos2 == std::string::npos || pos2 > pos3)
      service.insert(pos3, ":2170");

    return service;
  }

  EndpointQueryingStatus JobListRetrieverPluginWSRFCREAM::Query(const UserConfig&, const Endpoint&, std::list<Job>&, const EndpointQueryOptions<Job>&) const {
    return EndpointQueryingStatus::FAILED;
  }

} // namespace Arc

Arc::PluginDescriptor PLUGINS_TABLE_NAME[] = {
  { "WSRFCREAM", "HED:JobListRetrieverPlugin", "", 0, &Arc::JobListRetrieverPluginWSRFCREAM::Instance },
  { NULL, NULL, NULL, 0, NULL }
};

