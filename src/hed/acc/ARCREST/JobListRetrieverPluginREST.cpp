// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/StringConv.h>
#include <arc/data/DataBuffer.h>
#include <arc/data/DataHandle.h>

#include "JobListRetrieverPluginREST.h"

namespace Arc {

  Logger JobListRetrieverPluginREST::logger(Logger::getRootLogger(), "JobListRetrieverPlugin.REST");

  bool JobListRetrieverPluginREST::isEndpointNotSupported(const Endpoint& endpoint) const {
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
      service = "https://" + service + "/arex;
    } else {
      std::string proto = lower(service.substr(0,pos1));
      if((proto != "http") && (proto != "https")) return URL();
    }
    // Default port other than 443?
    // Default path?
    
    return service;
  }

  EndpointQueryingStatus JobListRetrieverPluginREST::Query(const UserConfig& uc, const Endpoint& endpoint, std::list<Job>& jobs, const EndpointQueryOptions<Job>&) const {
    EndpointQueryingStatus s(EndpointQueryingStatus::FAILED);

    URL url(CreateURL(endpoint.URLString));
    if (!url) {
      return s;
    }

    logger.msg(DEBUG, "Collecting Job (A-REX REST jobs) information.");

    DataHandle dir_url(url, uc);
    if (!dir_url) {
      logger.msg(INFO, "Failed retrieving job IDs: Unsupported url (%s) given", url.str());
      return s;
    }

    dir_url->SetSecure(false);
    std::list<FileInfo> files;
    if (!dir_url->List(files, DataPoint::INFO_TYPE_NAME)) {
      if (files.empty()) {
        logger.msg(INFO, "Failed retrieving job IDs");
        return s;
      }
      logger.msg(VERBOSE, "Error encoutered during job ID retrieval. All job IDs might not have been retrieved");
    }

    for (std::list<FileInfo>::const_iterator file = files.begin();
         file != files.end(); file++) {
      Job j;
      URL jobIDURL = url;
      std::string name = file->GetName();
      if(name.empty() || (name[0] == '*')) continue; // skip special names
      jobIDURL.ChangePath(jobIDURL.Path() + "/" + name);
      
      // Proposed mandatory attributes for ARC 3.0
      j.JobID = jobIDURL.fullstr();
      j.ServiceInformationURL = url;
      j.ServiceInformationInterfaceName = "org.nordugrid.arcrest";
      j.JobStatusURL = url;
      j.JobStatusInterfaceName = "org.nordugrid.arcrest";
      j.JobManagementURL = url;
      j.JobManagementInterfaceName = "org.nordugrid.arcrest";
      j.IDFromEndpoint = file->GetName();
      j.StageInDir = jobIDURL;
      j.StageOutDir = jobIDURL;
      j.SessionDir = jobIDURL;
      // j.DelegationID.push_back(delegationId); - TODO: Implement through reading job.#.status
 
      jobs.push_back(j);
    }

    // TODO: Because listing/obtaining content is too generic operation
    // maybe it is unsafe to claim that operation suceeded if nothing
    // was retrieved.
    s = EndpointQueryingStatus::SUCCESSFUL;

    return s;
  }

} // namespace Arc
