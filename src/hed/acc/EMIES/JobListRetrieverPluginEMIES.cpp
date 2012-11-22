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
      XMLNode infoxml;
      ac.info(*jobid, infoxml);
      bool submittedviaother = false;
      while (infoxml["OtherInfo"]) {
        std::string otherinfo = (std::string)infoxml["OtherInfo"];
        std::string submittedvia = "SubmittedVia=";
        if (otherinfo.compare(0, submittedvia.length(), submittedvia) == 0) {
          std::string interfacename = otherinfo.substr(submittedvia.length());
          if (interfacename != "org.ogf.emies") {
            logger.msg(DEBUG, "Skipping retrieved job (%s) because it was submitted via another interface (%s).", url.fullstr() + "/" + jobid->id, interfacename);
            submittedviaother = true;
          }
        }
        infoxml["OtherInfo"].Destroy();
      }
      if (submittedviaother) continue;
      
      Job j;
      if(!jobid->manager) jobid->manager = url;
      // Proposed mandatory attributes for ARC 3.0
      j.JobID = jobid->manager.str() + "/" + jobid->id;
      j.ServiceInformationURL = url.fullstr();
      j.ServiceInformationInterfaceName = "org.ogf.glue.emies.resourceinfo";
      j.JobStatusURL = jobid->manager;
      j.JobStatusInterfaceName = "org.ogf.glue.emies.activitymanagement";
      j.JobManagementURL = jobid->manager;
      j.JobManagementInterfaceName = "org.ogf.glue.emies.activitymanagement";
      j.IDFromEndpoint = jobid->id;
      
      jobs.push_back(j);
    };

    s = EndpointQueryingStatus::SUCCESSFUL;
    return s;
  }
} // namespace Arc
