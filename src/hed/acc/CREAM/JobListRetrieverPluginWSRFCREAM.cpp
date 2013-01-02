// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/StringConv.h>
#include <arc/UserConfig.h>
#include <arc/message/MCC.h>

#include "CREAMClient.h"

#include "JobListRetrieverPluginWSRFCREAM.h"

namespace Arc {

  Logger JobListRetrieverPluginWSRFCREAM::logger(Logger::getRootLogger(), "JobListRetrieverPlugin.WSRFCREAM");

  bool JobListRetrieverPluginWSRFCREAM::isEndpointNotSupported(const Endpoint& endpoint) const {
    const std::string::size_type pos = endpoint.URLString.find("://");
    return pos != std::string::npos && lower(endpoint.URLString.substr(0, pos)) != "http" && lower(endpoint.URLString.substr(0, pos)) != "https";
  }

  static URL CreateURL(std::string str) {
    std::string::size_type pos1 = str.find("://");
    if (pos1 == std::string::npos) {
      str = "https://" + str;
      pos1 = 5;
    } else {
      if(lower(str.substr(0,pos1)) != "https" && lower(str.substr(0,pos1)) != "http" ) return URL();
    }
    std::string::size_type pos2 = str.find(":", pos1 + 3);
    std::string::size_type pos3 = str.find("/", pos1 + 3);
    if (pos3 == std::string::npos && pos2 == std::string::npos) str += ":8443";
    else if (pos2 == std::string::npos || pos2 > pos3) str.insert(pos3, ":8443");

    return str;
  }

  static URL CreateInfoURL(std::string str) {
    std::string::size_type pos1 = str.find("://");
    if (pos1 == std::string::npos) {
      str = "ldap://" + str;
      pos1 = 4;
    } else {
      if(lower(str.substr(0,pos1)) != "ldap") return URL();
    }
    std::string::size_type pos2 = str.find(":", pos1 + 3);
    std::string::size_type pos3 = str.find("/", pos1 + 3);
    if (pos3 == std::string::npos) {
      if (pos2 == std::string::npos) str += ":2170";
      str += "/o=grid";
    }
    else if (pos2 == std::string::npos || pos2 > pos3)
      str.insert(pos3, ":2170");

    return str;
  }

  EndpointQueryingStatus JobListRetrieverPluginWSRFCREAM::Query(const UserConfig& uc, const Endpoint& e, std::list<Job>& jobs, const EndpointQueryOptions<Job>&) const {
    EndpointQueryingStatus s(EndpointQueryingStatus::FAILED);
    
    URL url = CreateURL(e.URLString);
    URL infourl = CreateInfoURL(url.Host());
    URL queryURL(url);
    queryURL.ChangePath(queryURL.Path() + "/CREAM2");

    MCCConfig cfg;
    uc.ApplyToConfig(cfg);
    CREAMClient creamClient(queryURL, cfg, uc.Timeout());
    std::list<creamJobInfo> cJobs;
    
    if (!creamClient.listJobs(cJobs)) return s;
    
    for (std::list<creamJobInfo>::const_iterator it = cJobs.begin();
         it != cJobs.end(); ++it) {
      Job j;
      j.JobID = queryURL.str() + '/' + it->id;
      j.ServiceInformationURL = infourl;
      j.ServiceInformationURL.ChangeLDAPFilter("");
      j.ServiceInformationInterfaceName = "org.nordugrid.ldapng";
      j.JobStatusURL = url;
      j.JobStatusInterfaceName = "org.glite.ce.cream";
      j.JobManagementURL = url;
      j.JobManagementInterfaceName = "org.glite.ce.cream";
      j.IDFromEndpoint = it->id;
      jobs.push_back(j);
    }

    s = EndpointQueryingStatus::SUCCESSFUL;
    
    return s;
  }

} // namespace Arc
