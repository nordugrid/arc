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
      j.InterfaceName = "org.ogf.glue.emies.activitymanagement";
      j.Cluster = jobid->manager;
      j.IDFromEndpoint = jobid->ToXML();
      // URL-izing job id
      j.JobID = URL(jobid->manager.str() + "/" + jobid->id);
      
      // Proposed mandatory attributes for ARC 3.0
      j.ID = jobid->manager.str() + "/" + jobid->id;
      j.ServiceInformationURL = url.fullstr();
      j.ServiceInformationInterfaceName = "org.ogf.glue.emies.resourceinfo";
      j.JobStatusURL = jobid->manager;
      j.JobStatusInterfaceName = "org.ogf.glue.emies.activitymanagement";
      j.JobManagementURL = jobid->manager;
      j.JobManagementInterfaceName = "org.ogf.glue.emies.activitymanagement";
      j.IDOnService = jobid->id;
      
      jobs.push_back(j);
    };

    s = EndpointQueryingStatus::SUCCESSFUL;
    return s;
  }
} // namespace Arc
