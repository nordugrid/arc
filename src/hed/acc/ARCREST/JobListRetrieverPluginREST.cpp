// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/StringConv.h>
#include <arc/message/MCC.h>
#include <arc/message/PayloadRaw.h>
#include <arc/communication/ClientInterface.h>

#include "JobControllerPluginREST.h"
#include "JobListRetrieverPluginREST.h"

namespace Arc {

  Logger JobListRetrieverPluginREST::logger(Logger::getRootLogger(), "JobListRetrieverPlugin.REST");

  EndpointQueryingStatus JobListRetrieverPluginREST::Query(const UserConfig& usercfg, const Endpoint& endpoint, std::list<Job>& jobs, const EndpointQueryOptions<Job>&) const {
    EndpointQueryingStatus s(EndpointQueryingStatus::FAILED);

    URL url(CreateURL(endpoint.URLString));
    if (!url) {
      return s;
    }
    URL jobIDsUrl(url);
    url.ChangePath(url.Path()+"/rest/1.0/jobs");

    logger.msg(DEBUG, "Collecting Job (A-REX REST jobs) information.");

    Arc::MCCConfig cfg;
    usercfg.ApplyToConfig(cfg);
    Arc::ClientHTTP client(cfg, url);

    Arc::PayloadRaw request;
    Arc::PayloadRawInterface* response(NULL);
    Arc::HTTPClientInfo info;
    std::multimap<std::string,std::string> attributes;
    attributes.insert(std::pair<std::string, std::string>("Accept", "text/xml"));
    Arc::MCC_Status res = client.process(std::string("GET"), attributes, &request, &info, &response);
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
    std::list<std::string> IDs;
    std::list<Job*> idJobs;
    for(Arc::XMLNode job = jobs_list["job"]; (bool)job; ++job) {
      std::string id = job["id"];
      if(id.empty()) continue;

      Job j;
      URL jobIDURL = url;
      jobIDURL.ChangePath(jobIDURL.Path() + "/" + id);
      URL sessionURL = jobIDURL;
      sessionURL.ChangePath(sessionURL.Path() + "/session");

      // Proposed mandatory attributes for ARC 3.0
      j.JobID = jobIDURL.fullstr();
      j.ServiceInformationURL = endpoint.URLString;
      j.ServiceInformationInterfaceName = "org.nordugrid.arcrest";
      j.JobStatusURL = endpoint.URLString;
      j.JobStatusInterfaceName = "org.nordugrid.arcrest";
      j.JobManagementURL = endpoint.URLString;
      j.JobManagementInterfaceName = "org.nordugrid.arcrest";
      j.IDFromEndpoint = id;
      j.StageInDir = sessionURL;
      j.StageOutDir = sessionURL;
      j.SessionDir = sessionURL;
      // j.DelegationID.push_back(delegationId); - TODO: Implement through reading job.#.status

      jobs.push_back(j);

      idJobs.push_back(&(jobs.back()));
      IDs.push_back(id);
    }

    class JobDelegationsProcessor: public JobControllerPluginREST::InfoNodeProcessor {
     public:
      JobDelegationsProcessor(std::list<Job*>& jobs): jobs(jobs) {}

      virtual void operator()(std::string const& id, XMLNode node) {
        std::string job_id = node["id"];
        XMLNode job_delegation_id = node["delegation_id"];
        if((bool)job_delegation_id && !job_id.empty()) {
          for(std::list<Job*>::iterator itJob = jobs.begin(); itJob != jobs.end(); ++itJob) {
            std::string id = (*itJob)->JobID;
            std::string::size_type pos = id.rfind('/');
            if(pos != std::string::npos) id.erase(0,pos+1);
            if(job_id == id) {
              while(job_delegation_id) {
                (*itJob)->DelegationID.push_back((std::string)job_delegation_id);
                ++job_delegation_id;
              }
              break;
            }
          }
        }
      }

     private:
      std::list<Job*>& jobs;
    };

    std::list<std::string> processedIDs;
    std::list<std::string> notProcessedIDs;
    JobDelegationsProcessor delegationsProcessor(idJobs);
    JobControllerPluginREST::ProcessJobs(&usercfg, jobIDsUrl, "delegations", 200, IDs, processedIDs, notProcessedIDs, delegationsProcessor);

    // TODO: Because listing/obtaining content is too generic operation
    // maybe it is unsafe to claim that operation suceeded if nothing
    // was retrieved.
    s = EndpointQueryingStatus::SUCCESSFUL;

    return s;
  }

} // namespace Arc

