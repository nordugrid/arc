// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/Logger.h>
#include <arc/StringConv.h>
#include <arc/URL.h>
#include <arc/message/MCC.h>

#include "../grid-manager/files/ControlFileContent.h"
#include "../grid-manager/files/ControlFileHandling.h"


#include "JobStateLOCAL.h"
#include "LOCALClient.h"
#include "JobListRetrieverPluginLOCAL.h"

using namespace Arc;
namespace ARexLOCAL {

  Logger JobListRetrieverPluginLOCAL::logger(Logger::getRootLogger(), "JobListRetrieverPlugin.LOCAL");

  bool JobListRetrieverPluginLOCAL::isEndpointNotSupported(const Endpoint& endpoint) const {
    const std::string::size_type pos = endpoint.URLString.find("://");
    if (pos != std::string::npos) {
      const std::string proto = lower(endpoint.URLString.substr(0, pos));
      return ((proto != "file"));
    }
    
    return false;
  }

  static URL CreateURL(std::string service) {
    std::string::size_type pos1 = service.find("://");
    if (pos1 == std::string::npos) {
      service = "file://" + service;
    } else {
      std::string proto = lower(service.substr(0,pos1));
      if(proto != "file") return URL();
    }
   
    return service;
  }

  EndpointQueryingStatus JobListRetrieverPluginLOCAL::Query(const UserConfig& uc, const Endpoint& endpoint, std::list<Job>& jobs, const EndpointQueryOptions<Job>&) const {
    EndpointQueryingStatus s(EndpointQueryingStatus::FAILED);


    //this can all be simplified I think - TO-DO 
    URL url(CreateURL(endpoint.URLString));
    if (!url) {
      return s;
    }


    LOCALClient ac(uc);

    std::list<LOCALJob> localjobs;
    if (!ac.list(localjobs)) {
      return s;
    }
    
    logger.msg(DEBUG, "Listing localjobs succeeded, %d localjobs found", localjobs.size());

    //checks that the job is in state other than undefined
    std::list<LOCALJob> jobids_found;
    ac.info(localjobs,jobids_found);
    
    std::list<LOCALJob>::iterator itID = jobids_found.begin();
    for(; itID != jobids_found.end(); ++itID) {

      //read job description to get hold of submission-interface
      ARex::JobLocalDescription job_desc;
      ARex::JobId jobid((*itID).id);
      ARex::job_local_read_file(jobid, *(ac.config), job_desc);

      std::string submittedVia = job_desc.interface;
      if (submittedVia != "org.nordugrid.local") {
        logger.msg(DEBUG, "Skipping retrieved job (%s) because it was submitted via another interface (%s).", url.fullstr() + "/" + itID->id, submittedVia);
        continue;
      }


      LOCALJob localjob;      
      Job j;
      itID->toJob(&ac, &localjob, j);
      jobs.push_back(j);
    };

    s = EndpointQueryingStatus::SUCCESSFUL;
    return s;
  }
} // namespace Arc
