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
      service = "https://" + service + "/arex";
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
    
    logger.msg(DEBUG, "Listing jobs succeeded, %d jobs found", jobids.size());
    
    std::list<EMIESResponse*> responses;
    ac.info(jobids, responses);
    
    std::list<EMIESJob>::iterator itID = jobids.begin();
    std::list<EMIESResponse*>::iterator itR = responses.begin();
    for(; itR != responses.end() && itID != jobids.end(); ++itR, ++itID) {
      EMIESJobInfo* jInfo = dynamic_cast<EMIESJobInfo*>(*itR);
      if (!jInfo) {
        // TODO: Handle ERROR
        continue;
      }
      
      std::string submittedVia = jInfo->getSubmittedVia();
      if (submittedVia != "org.ogf.glue.emies.activitycreation") {
        logger.msg(DEBUG, "Skipping retrieved job (%s) because it was submitted via another interface (%s).", url.fullstr() + "/" + itID->id, submittedVia);
        continue;
      }
      
      Job j;
      if(!itID->manager) itID->manager = url;
      itID->toJob(j);
      jInfo->toJob(j);
      jobs.push_back(j);
    };

    s = EndpointQueryingStatus::SUCCESSFUL;
    return s;
  }
} // namespace Arc
