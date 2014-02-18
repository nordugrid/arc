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

  EndpointQueryingStatus JobListRetrieverPluginWSRFCREAM::Query(const UserConfig& uc, const Endpoint& e, std::list<Job>& jobs, const EndpointQueryOptions<Job>&) const {
    EndpointQueryingStatus s(EndpointQueryingStatus::FAILED);
    URL url((e.URLString.find("://") == std::string::npos ? "https://" : "") + e.URLString, false, 8443);
    URL infourl("ldap://" + url.Host(), false, 2170, "/o=grid");
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
