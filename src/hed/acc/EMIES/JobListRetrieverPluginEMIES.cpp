// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/Logger.h>
#include <arc/StringConv.h>
#include <arc/URL.h>
#include <arc/message/MCC.h>

#include "JobStateEMIES.h"
#include "EMIESClient.h"
#include "JobListRetrieverPluginEMIES.h"

namespace Arc {

  Logger JobListRetrieverPluginEMIES::logger(Logger::getRootLogger(), "JobListRetrieverPlugin.EMIES");

  bool JobListRetrieverPluginEMIES::isEndpointNotSupported(const Endpoint& endpoint) const {
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
      service = "https://" + service;
    } else {
      std::string proto = lower(service.substr(0,pos1));
      if((proto != "http") && (proto != "https")) return URL();
    }
    // Default port other than 443?
    // Default path?
    
    return service;
  }

  EndpointQueryingStatus JobListRetrieverPluginEMIES::Query(const UserConfig& uc, const Endpoint& endpoint, std::list<Job>& jobs, const EndpointQueryOptions<Job>&) const {
    EndpointQueryingStatus s(EndpointQueryingStatus::FAILED);

    URL url(CreateURL(endpoint.URLString));
    if (!url) {
      return s;
    }

    MCCConfig cfg;
    uc.ApplyToConfig(cfg);
    EMIESClient ac(url, cfg, uc.Timeout());

    std::list<EMIESJob> jobids;
    if (!ac.list(jobids)) {
      return s;
    }

    for(std::list<EMIESJob>::iterator jobid = jobids.begin(); jobid != jobids.end(); ++jobid) {
      Job j;
      if(!jobid->manager) jobid->manager = url;
      j.InterfaceName = supportedInterfaces.front();
      j.Cluster = jobid->manager;
      jobid->ToXML().GetXML(j.IDFromEndpoint);
      // URL-izing job id
      URL jobidu(jobid->manager);
      jobidu.AddOption("emiesjobid",jobid->id,true);
      j.JobID = jobidu;
      jobs.push_back(j);
    };

    if (!jobids.empty()) {
      s = EndpointQueryingStatus::SUCCESSFUL;
    }
    return s;
  }
} // namespace Arc
