// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib.h>

#include <arc/StringConv.h>
#include <arc/UserConfig.h>
#include <arc/XMLNode.h>
#include <arc/compute/JobDescription.h>
#include <arc/data/DataMover.h>
#include <arc/data/DataHandle.h>
#include <arc/data/URLMap.h>
#include <arc/message/MCC.h>
#include <arc/Utils.h>
#include <arc/communication/ClientInterface.h>
#include <arc/delegation/DelegationInterface.h>

#include "JobControllerPluginREST.h"

namespace Arc {

  Logger JobControllerPluginREST::logger(Logger::getRootLogger(), "JobControllerPlugin.REST");

  class JobStateARCREST: public JobState {
  public:
    JobStateARCREST(const std::string& state): JobState(state, &StateMap) {}
    static JobState::StateType StateMap(const std::string& state);
  };

  bool JobControllerPluginREST::GetDelegation(Arc::URL url, std::string& delegationId) const {
/*
    std::string delegationRequest;
    Arc::MCCConfig cfg;
    usercfg->ApplyToConfig(cfg);
    std::string delegationPath = url.Path();
    if(!delegationId.empty()) delegationPath = delegationPath+"/"+delegationId;
    Arc::ClientHTTP client(cfg, url);
    {
      Arc::PayloadRaw request;
      Arc::PayloadRawInterface* response(NULL);
      Arc::HTTPClientInfo info;
      Arc::MCC_Status res = client.process(std::string("GET"), delegationPath, &request, &info, &response);
      if((!res) || (info.code != 200) || (info.reason.empty()) || (!response)) {
        delete response;
        return false;
      }
      delegationId = info.reason;
      for(unsigned int n = 0;response->Buffer(n);++n) {
        delegationRequest.append(response->Buffer(n),response->BufferSize(n));
      }
      delete response;
    }
    {
      DelegationProvider* deleg(NULL);
      if (!cfg.credential.empty()) {
        deleg = new DelegationProvider(cfg.credential);
      }
      else {
        const std::string& cert = (!cfg.proxy.empty() ? cfg.proxy : cfg.cert);
        const std::string& key  = (!cfg.proxy.empty() ? cfg.proxy : cfg.key);
        if (key.empty() || cert.empty()) return false;
        deleg = new DelegationProvider(cert, key);
      }
      std::string delegationResponse = deleg->Delegate(delegationRequest);
      delete deleg;

      Arc::PayloadRaw request;
      request.Insert(delegationResponse.c_str(),0,delegationResponse.length());
      Arc::PayloadRawInterface* response(NULL);
      Arc::HTTPClientInfo info;
      Arc::MCC_Status res = client.process(std::string("PUT"), url.Path()+"/"+delegationId, &request, &info, &response);
      delete response;
      if((!res) || (info.code != 200) || (!response)) return false;
    }
*/
    return true;
  }

  JobState::StateType JobStateARCREST::StateMap(const std::string& state) {
    if (state == "ACCEPTING")
      return JobState::ACCEPTED;
    else if (state == "ACCEPTED")
      return JobState::ACCEPTED;
    else if (state == "PREPARING")
      return JobState::PREPARING;
    else if (state == "PREPARED")
      return JobState::PREPARING;
    else if (state == "SUBMITTING")
      return JobState::SUBMITTING;
    else if (state == "QUEUING")
      return JobState::QUEUING;
    else if (state == "RUNNING")
      return JobState::RUNNING;
    else if (state == "HELD")
      return JobState::HOLD;
    else if (state == "EXITINGLRMS")
      return JobState::RUNNING;
    else if (state == "OTHER")
      return JobState::RUNNING;
    else if (state == "EXECUTED")
      return JobState::RUNNING;
    else if (state == "KILLING")
      return JobState::RUNNING;
    else if (state == "FINISHING")
      return JobState::FINISHING;
    else if (state == "FINISHED")
      return JobState::FINISHED;
    else if (state == "FAILED")
      return JobState::FAILED;
    else if (state == "KILLED")
      return JobState::KILLED;
    else if (state == "WIPED")
      return JobState::DELETED;
    else if (state == "")
      return JobState::UNDEFINED;
    else
      return JobState::OTHER;
  }

  URL JobControllerPluginREST::GetAddressOfResource(const Job& job) {
    return job.ServiceInformationURL;
  }

  bool JobControllerPluginREST::isEndpointNotSupported(const std::string& endpoint) const {
    const std::string::size_type pos = endpoint.find("://");
    return pos != std::string::npos && lower(endpoint.substr(0, pos)) != "http" && lower(endpoint.substr(0, pos)) != "https";
  }

  void JobControllerPluginREST::UpdateJobs(std::list<Job*>& jobs, std::list<std::string>& IDsProcessed, std::list<std::string>& IDsNotProcessed, bool isGrouped) const {
    class JobStateProcessor: public InfoNodeProcessor {
     public:
      JobStateProcessor(std::list<Job*>& jobs): jobs(jobs) {}

      virtual void operator()(std::string const& id, XMLNode node) {
        std::string job_id = node["id"];
        std::string job_state = node["state"];
        if(!job_state.empty() && !job_id.empty()) {
          for(std::list<Job*>::iterator itJob = jobs.begin(); itJob != jobs.end(); ++itJob) {
            std::string id = (*itJob)->JobID;
            std::string::size_type pos = id.rfind('/');
            if(pos != std::string::npos) id.erase(0,pos+1);
            if(job_id == id) {
              (*itJob)->State = JobStateARCREST(job_state);
              break;
            }
          }
        }
      }

     private:
      std::list<Job*>& jobs;
    };

    JobStateProcessor infoNodeProcessor(jobs);
    Arc::URL currentServiceUrl;
    std::list<std::string> IDs;
    for (std::list<Job*>::const_iterator it = jobs.begin(); it != jobs.end(); ++it) {
      if(!currentServiceUrl || (currentServiceUrl != GetAddressOfResource(**it))) {
        if(!IDs.empty()) {
          ProcessJobs(currentServiceUrl, "status", 200, IDs, IDsProcessed, IDsNotProcessed, infoNodeProcessor);
        }
        currentServiceUrl = GetAddressOfResource(**it);
      }

      IDs.push_back((*it)->JobID);
    }
    if(!IDs.empty()) {
      ProcessJobs(currentServiceUrl, "status", 200, IDs, IDsProcessed, IDsNotProcessed, infoNodeProcessor);
    }
  }

  bool JobControllerPluginREST::CleanJobs(const std::list<Job*>& jobs, std::list<std::string>& IDsProcessed, std::list<std::string>& IDsNotProcessed, bool isGrouped) const {
    bool ok = true;
    
    InfoNodeProcessor infoNodeProcessor;
    Arc::URL currentServiceUrl;
    std::list<std::string> IDs;
    for (std::list<Job*>::const_iterator it = jobs.begin(); it != jobs.end(); ++it) {
      if(!currentServiceUrl || (currentServiceUrl != GetAddressOfResource(**it))) {
        if(!IDs.empty()) {
          if (!ProcessJobs(currentServiceUrl, "clean", 202, IDs, IDsProcessed, IDsNotProcessed, infoNodeProcessor))
            ok = false;
        }
        currentServiceUrl = GetAddressOfResource(**it);
      }

      IDs.push_back((*it)->JobID);
    }
    if(!IDs.empty()) {
      if (!ProcessJobs(currentServiceUrl, "clean", 202, IDs, IDsProcessed, IDsNotProcessed, infoNodeProcessor)) {
        ok = false;
      }
    }

    return ok;
  }

  bool JobControllerPluginREST::CancelJobs(const std::list<Job*>& jobs, std::list<std::string>& IDsProcessed, std::list<std::string>& IDsNotProcessed, bool isGrouped) const {
    bool ok = true;
    
    InfoNodeProcessor infoNodeProcessor;
    Arc::URL currentServiceUrl;
    std::list<std::string> IDs;
    for (std::list<Job*>::const_iterator it = jobs.begin(); it != jobs.end(); ++it) {
      if(!currentServiceUrl || (currentServiceUrl != GetAddressOfResource(**it))) {
        if(!IDs.empty()) {
          if (!ProcessJobs(currentServiceUrl, "kill", 202, IDs, IDsProcessed, IDsNotProcessed, infoNodeProcessor))
            ok = false;
        }
        currentServiceUrl = GetAddressOfResource(**it);
      }

      IDs.push_back((*it)->JobID);
    }
    if(!IDs.empty()) {
      if (!ProcessJobs(currentServiceUrl, "kill", 202, IDs, IDsProcessed, IDsNotProcessed, infoNodeProcessor)) {
        ok = false;
      }
    }

    return ok;
  }

  bool JobControllerPluginREST::RenewJobs(const std::list<Job*>& jobs, std::list<std::string>& IDsProcessed, std::list<std::string>& IDsNotProcessed, bool isGrouped) const {
    bool ok = true;
/*
    for (std::list<Job*>::const_iterator it = jobs.begin(); it != jobs.end(); ++it) {
      Arc::URL delegationUrl(GetAddressOfResource(**it));
      delegationUrl.ChangePath(delegationUrl.Path()+DelegPrefix);
      // 1. Fetch/find delegation ids for each job
      if((*it)->DelegationID.empty()) {
        logger.msg(INFO, "Job %s has no delegation associated. Can't renew such job.", (*it)->JobID);
        IDsNotProcessed.push_back((*it)->JobID);
        continue;
      }
      // 2. Leave only unique IDs - not needed yet because current code uses
      //    different delegations for each job.
      // 3. Renew credentials for every ID
      std::list<std::string>::const_iterator did = (*it)->DelegationID.begin();
      for(;did != (*it)->DelegationID.end();++did) {
        std::string delegationId(*did);
        if(!delegationId.empty()) {
          if(!GetDelegation(delegationUrl, delegationId)) {
            logger.msg(INFO, "Job %s failed to renew delegation %s.", (*it)->JobID, *did);
            break;
          }
        }
      }
      if(did != (*it)->DelegationID.end()) {
        IDsNotProcessed.push_back((*it)->JobID);
        ok = false;
        continue;
      }
      IDsProcessed.push_back((*it)->JobID);
    }
*/
    ok = false; // not implemented yet
    return ok;
  }

  bool JobControllerPluginREST::ResumeJobs(const std::list<Job*>& jobs, std::list<std::string>& IDsProcessed, std::list<std::string>& IDsNotProcessed, bool isGrouped) const {
    bool ok = true;
    
    InfoNodeProcessor infoNodeProcessor;
    Arc::URL currentServiceUrl;
    std::list<std::string> IDs;
    for (std::list<Job*>::const_iterator it = jobs.begin(); it != jobs.end(); ++it) {
      if(!currentServiceUrl || (currentServiceUrl != GetAddressOfResource(**it))) {
        if(!IDs.empty()) {
          if (!ProcessJobs(currentServiceUrl, "restart", 202, IDs, IDsProcessed, IDsNotProcessed, infoNodeProcessor)) {
            ok = false;
          }
        }
        currentServiceUrl = GetAddressOfResource(**it);
      }

      IDs.push_back((*it)->JobID);
    }
    if(!IDs.empty()) {
      if (!ProcessJobs(currentServiceUrl, "restart", 202, IDs, IDsProcessed, IDsNotProcessed, infoNodeProcessor)) {
        ok = false;
      }
    }

    return ok;
  }

  bool JobControllerPluginREST::ProcessJobs(Arc::URL const & resourceUrl, std::string const & action, int successCode,
          std::list<std::string>& IDs, std::list<std::string>& IDsProcessed, std::list<std::string>& IDsNotProcessed,
          InfoNodeProcessor& infoNodeProcessor) const {
    Arc::URL statusUrl(resourceUrl);
    statusUrl.ChangePath(statusUrl.Path()+"/rest/1.0/jobs");
    statusUrl.AddHTTPOption("action",action);

    Arc::MCCConfig cfg;
    usercfg->ApplyToConfig(cfg);
    Arc::ClientHTTP client(cfg, statusUrl);
    Arc::PayloadRaw request;
    Arc::PayloadRawInterface* response(NULL);
    Arc::HTTPClientInfo info;
    {
      XMLNode jobs_id_list("<jobs/>");
      for (std::list<std::string>::const_iterator it = IDs.begin(); it != IDs.end(); ++it) {
        std::string id(*it);
        std::string::size_type pos = id.rfind('/');
        if(pos != std::string::npos) id.erase(0,pos+1);
        Arc::XMLNode job = jobs_id_list.NewChild("job");
        job.NewChild("id") = id;
      }
      std::string jobs_id_str;
      jobs_id_list.GetXML(jobs_id_str);
      request.Insert(jobs_id_str.c_str(),0,jobs_id_str.length());
    }
    std::multimap<std::string,std::string> attributes;
    attributes.insert(std::pair<std::string, std::string>("Accept", "text/xml"));
    Arc::MCC_Status res = client.process(std::string("POST"), attributes, &request, &info, &response);
    if((!res) || (info.code != 201)) {
      logger.msg(WARNING, "Failed to process jobs - wrong response: %u", info.code);
      if(response) logger.msg(DEBUG, "Content: %s", response->Content());
      delete response; response = NULL;
      for (std::list<std::string>::const_iterator it = IDs.begin(); it != IDs.end(); ++it) {
        logger.msg(WARNING, "Failed to process job: %s", *it);
        IDsNotProcessed.push_back(*it);
      }
      return false;
    }

    logger.msg(DEBUG, "Content: %s", response->Content());
    Arc::XMLNode jobs_list(response->Content());
    delete response; response = NULL;
    if(!jobs_list || (jobs_list.Name() != "jobs")) {
      logger.msg(WARNING, "Failed to process jobs - failed to parse response");
      for (std::list<std::string>::const_iterator it = IDs.begin(); it != IDs.end(); ++it) {
        logger.msg(WARNING, "Failed to process job: %s", *it);
        IDsNotProcessed.push_back(*it);
      }
      return false;
    }

    bool ok = true;
    Arc::XMLNode job_item = jobs_list["job"];
    for (;; ++job_item) {
      if(!job_item) { // no more jobs returned 
        for (std::list<std::string>::const_iterator it = IDs.begin(); it != IDs.end(); ++it) {
          logger.msg(WARNING, "No response returned: %s", *it);
          IDsNotProcessed.push_back(*it);
          ok = false;
        }
        break;
      }

      std::string jcode = job_item["status-code"];
      std::string jreason = job_item["reason"];
      std::string jid = job_item["id"];
      if(jid.empty()) {
        // hmm
      } else {
        std::list<std::string>::iterator it = IDs.begin();
        for (; it != IDs.end(); ++it) {
          std::string id(*it);
          std::string::size_type pos = id.rfind('/');
          if(pos != std::string::npos) id.erase(0,pos+1);
          if (id == jid) break;
        }
        if(it == IDs.end()) {
          // hmm again
        } else {
          if(jcode != Arc::tostring(successCode)) {
            logger.msg(WARNING, "Failed to process job: %s - %s %s", jid, jcode, jreason);
            IDsNotProcessed.push_back(*it);
            ok = false;
          } else {
            IDsProcessed.push_back(*it);
          }
          infoNodeProcessor(*it, job_item);
          IDs.erase(it);
        }
      }
    }
    return ok;
  }

  bool JobControllerPluginREST::GetURLToJobResource(const Job& job, Job::ResourceType resource, URL& url) const {
    url = URL(job.JobID);
    // compensate for time between request and response on slow networks
    url.AddOption("threads=2",false);
    url.AddOption("encryption=optional",false);
    url.AddOption("httpputpartial=yes",false);
    switch (resource) {
    case Job::STDIN:
      if(job.StdIn.empty()) return false;
      url.ChangePath(url.Path() + "/session/" + job.StdIn);
      break;
    case Job::STDOUT:
      if(job.StdOut.empty()) return false;
      url.ChangePath(url.Path() + "/session/" + job.StdOut);
      break;
    case Job::STDERR:
      if(job.StdErr.empty()) return false;
      url.ChangePath(url.Path() + "/session/" + job.StdErr);
      break;
    case Job::STAGEINDIR:
    case Job::STAGEOUTDIR:
    case Job::SESSIONDIR:
      url.ChangePath(url.Path() + "/session");
      break;
    case Job::JOBLOG:
      url.ChangePath(url.Path() + "/diagnose/errors");
      break;
    case Job::JOBDESCRIPTION:
      url.ChangePath(url.Path() + "/diagnose/description");
      break;
    }

    return true;
  }

  bool JobControllerPluginREST::GetJobDescription(const Job& job, std::string& desc_str) const {
    //Arc::URL statusUrl(job.JobID);
    //statusUrl.ChangePath(statusUrl.Path()+"/diagnose/description");
    Arc::URL statusUrl(GetAddressOfResource(job));
    std::string id(job.JobID);
    std::string::size_type pos = id.rfind('/');
    if(pos != std::string::npos) id.erase(0,pos+1);
    statusUrl.ChangePath(statusUrl.Path()+"/rest/1.0/jobs/"+id+"/diagnose/description");

    Arc::MCCConfig cfg;
    usercfg->ApplyToConfig(cfg);
    Arc::ClientHTTP client(cfg, statusUrl);
    Arc::PayloadRaw request;
    Arc::PayloadRawInterface* response(NULL);
    Arc::HTTPClientInfo info;
    Arc::MCC_Status res = client.process(std::string("GET"), &request, &info, &response);
    if((!res) || (info.code != 200) || (response == NULL) || (response->Buffer(0) == NULL)) {
      delete response;
      logger.msg(ERROR, "Failed retrieving job description for job: %s", job.JobID);
      return false;
    }
    desc_str.assign(response->Buffer(0),response->BufferSize(0));
    delete response;
    return true;
  }

} // namespace Arc

