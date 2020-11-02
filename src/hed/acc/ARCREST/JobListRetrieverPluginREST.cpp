// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/StringConv.h>
#include <arc/message/MCC.h>
#include <arc/message/PayloadRaw.h>
#include <arc/communication/ClientInterface.h>

#include "JobListRetrieverPluginREST.h"

namespace Arc {

  Logger JobListRetrieverPluginREST::logger(Logger::getRootLogger(), "JobListRetrieverPlugin.REST");

  EndpointQueryingStatus JobListRetrieverPluginREST::Query(const UserConfig& usercfg, const Endpoint& endpoint, std::list<Job>& jobs, const EndpointQueryOptions<Job>&) const {
    EndpointQueryingStatus s(EndpointQueryingStatus::FAILED);

    URL url(endpoint.URLString);
    if (!url) {
      return s;
    }
    url.ChangePath(url.Path()+"/jobs");

    logger.msg(DEBUG, "Collecting Job (A-REX REST jobs) information.");

    Arc::MCCConfig cfg;
    usercfg.ApplyToConfig(cfg);
    Arc::ClientHTTP client(cfg, url);

    Arc::PayloadRaw request;
    Arc::PayloadRawInterface* response(NULL);
    Arc::HTTPClientInfo info;
    Arc::MCC_Status res = client.process(std::string("GET"), &request, &info, &response);
    if((!res) || (info.code != 200) || (!response)) {
      delete response;
      return s;
    }
    std::string jobsResponse;
    for(unsigned int n = 0;response->Buffer(n);++n) jobsResponse.append(response->Buffer(n),response->BufferSize(n));
    delete response;
    Arc::XMLNode jobs_list(jobsResponse);
    if(!jobs_list)
      return s;
    if(jobs_list.Name() != "jobs")
      return s;
    for(Arc::XMLNode job = jobs_list["job"]; (bool)job; ++job) {
      std::string id = job["id"];
      if(id.empty()) continue;

      Job j;
      URL jobIDURL = url;
      jobIDURL.ChangePath(jobIDURL.Path() + "/" + id);
      URL sessionURL = url;
      sessionURL.ChangePath(sessionURL.Path() + "/session");

      // Proposed mandatory attributes for ARC 3.0
      j.JobID = jobIDURL.fullstr();
      j.ServiceInformationURL = url;
      j.ServiceInformationInterfaceName = "org.nordugrid.arcrest";
      j.JobStatusURL = url;
      j.JobStatusInterfaceName = "org.nordugrid.arcrest";
      j.JobManagementURL = url;
      j.JobManagementInterfaceName = "org.nordugrid.arcrest";
      j.IDFromEndpoint = id;
      j.StageInDir = sessionURL;
      j.StageOutDir = sessionURL;
      j.SessionDir = sessionURL;
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

