// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/StringConv.h>
#include <arc/data/DataBuffer.h>
#include <arc/data/DataHandle.h>

#include "JobListRetrieverPluginWSRFGLUE2.h"

namespace Arc {

  Logger JobListRetrieverPluginWSRFGLUE2::logger(Logger::getRootLogger(), "JobListRetrieverPlugin.WSRFGLUE2");

  bool JobListRetrieverPluginWSRFGLUE2::isEndpointNotSupported(const ComputingInfoEndpoint& endpoint) const {
    const std::string::size_type pos = endpoint.URLString.find("://");
    if (pos != std::string::npos) {
      const std::string proto = lower(endpoint.URLString.substr(0, pos));
      return ((proto != "http") && (proto != "https"));
    }
    
    return false;
  }

  static bool CreateURL(std::string URLString, URL& url) {
    std::string::size_type pos1 = URLString.find("://");
    if (pos1 == std::string::npos) {
      URLString = "https://" + URLString;
    } else {
      std::string proto = lower(URLString.substr(0,pos1));
      if((proto != "http") && (proto != "https")) return false;
    }
    // Default port other than 443?
    // Default path?
    
    url = URLString;
    return true;
  }

  EndpointQueryingStatus JobListRetrieverPluginWSRFGLUE2::Query(const UserConfig& uc, const ComputingInfoEndpoint& endpoint, std::list<Job>& jobs, const EndpointQueryOptions<Job>&) const {
    EndpointQueryingStatus s(EndpointQueryingStatus::FAILED);

    URL url(endpoint.URLString);
    if (!url) {
      return s;
    }

    logger.msg(DEBUG, "Collecting Job (A-REX jobs) information.");

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
      j.JobID = url;
      j.JobID.ChangePath(j.JobID.Path() + "/" + file->GetName());
      //j.Flavour = "ARC1"; // TODO
      j.Cluster = url;
      jobs.push_back(j);
    }

    if (!files.empty()) {
      s = EndpointQueryingStatus::SUCCESSFUL;
    }

    return s;
  }

} // namespace Arc
