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

#include "SubmitterPluginREST.h"
#include "JobControllerPluginREST.h"

namespace Arc {

  Logger JobControllerPluginREST::logger(Logger::getRootLogger(), "JobControllerPlugin.REST");

  class JobStateARCREST: public JobState {
  public:
    JobStateARCREST(const std::string& state): JobState(state, &StateMap) {}
    static JobState::StateType StateMap(const std::string& state);
  };

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

      virtual void operator()(std::string const& id, XMLNode node, URL const& query_url) {
        std::string job_id = node["id"];
        std::string job_state = node["state"];
        if(!job_state.empty() && !job_id.empty()) {
          for(std::list<Job*>::iterator itJob = jobs.begin(); itJob != jobs.end(); ++itJob) {
            std::string id = (*itJob)->JobID;
            std::string::size_type pos = id.rfind('/');
            if(pos != std::string::npos) id.erase(0,pos+1);
            if(job_id == id) {
              (*itJob)->State = JobStateARCREST(job_state);
              // (*itJob)->RestartState = ;
              std::string baseUrl = query_url.ConnectionURL()+query_url.Path()+"/"+job_id;
              (*itJob)->StageInDir = baseUrl;
              (*itJob)->StageOutDir = baseUrl;
              (*itJob)->SessionDir = baseUrl;
              // (*itJob)->DelegationID.push_back ;
              break;
            }
          }
        }
      }

     private:
      std::list<Job*>& jobs;
    };

    class JobInfoProcessor: public InfoNodeProcessor {
     public:
      JobInfoProcessor(std::list<Job*>& jobs): jobs(jobs) {}

      virtual void operator()(std::string const& id, XMLNode node, URL const& query_url) {
        std::string job_id = node["id"];
        XMLNode job_info = node["info_document"];
        if(job_info && !job_id.empty()) {
          for(std::list<Job*>::iterator itJob = jobs.begin(); itJob != jobs.end(); ++itJob) {
            std::string id = (*itJob)->JobID;
            std::string::size_type pos = id.rfind('/');
            if(pos != std::string::npos) id.erase(0,pos+1);
            if(job_id == id) {
              (*itJob)->SetFromXML(job_info["ComputingActivity"]);              
              std::string baseUrl = query_url.ConnectionURL()+query_url.Path()+"/"+job_id;
              (*itJob)->StageInDir = baseUrl;
              (*itJob)->StageOutDir = baseUrl;
              (*itJob)->SessionDir = baseUrl;
              for(XMLNode state = job_info["ComputingActivity"]["State"]; (bool)state; ++state) {
                std::string stateStr = state;
                if(strncmp(stateStr.c_str(), "arcrest:", 8) == 0) {
                  (*itJob)->State = JobStateARCREST(stateStr.substr(8));
                  break;
                }
              }
              break;
            }
          }
        }
      }

     private:
      std::list<Job*>& jobs;
    };

    JobInfoProcessor infoProcessor(jobs);
    Arc::URL currentServiceUrl;
    std::list<std::string> IDs;
    for (std::list<Job*>::const_iterator it = jobs.begin(); it != jobs.end(); ++it) {
      if(!currentServiceUrl || (currentServiceUrl != GetAddressOfResource(**it))) {
        if(!IDs.empty()) {
          std::list<std::string> fakeIDs = IDs;
          ProcessJobs(usercfg, currentServiceUrl, "info", 200, IDs, IDsProcessed, IDsNotProcessed, infoProcessor);
        }
        currentServiceUrl = GetAddressOfResource(**it);
      }

      IDs.push_back((*it)->JobID);
    }
    if(!IDs.empty()) {
      std::list<std::string> fakeIDs = IDs;
      ProcessJobs(usercfg, currentServiceUrl, "info", 200, IDs, IDsProcessed, IDsNotProcessed, infoProcessor);
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
          if (!ProcessJobs(usercfg, currentServiceUrl, "clean", 202, IDs, IDsProcessed, IDsNotProcessed, infoNodeProcessor))
            ok = false;
        }
        currentServiceUrl = GetAddressOfResource(**it);
      }

      IDs.push_back((*it)->JobID);
    }
    if(!IDs.empty()) {
      if (!ProcessJobs(usercfg, currentServiceUrl, "clean", 202, IDs, IDsProcessed, IDsNotProcessed, infoNodeProcessor)) {
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
          if (!ProcessJobs(usercfg, currentServiceUrl, "kill", 202, IDs, IDsProcessed, IDsNotProcessed, infoNodeProcessor))
            ok = false;
        }
        currentServiceUrl = GetAddressOfResource(**it);
      }

      IDs.push_back((*it)->JobID);
    }
    if(!IDs.empty()) {
      if (!ProcessJobs(usercfg, currentServiceUrl, "kill", 202, IDs, IDsProcessed, IDsNotProcessed, infoNodeProcessor)) {
        ok = false;
      }
    }

    return ok;
  }

  bool JobControllerPluginREST::RenewJobs(const std::list<Job*>& jobs, std::list<std::string>& IDsProcessed, std::list<std::string>& IDsNotProcessed, bool isGrouped) const {
    bool ok = true;
    for (std::list<Job*>::const_iterator it = jobs.begin(); it != jobs.end(); ++it) {
      Arc::URL delegationUrl(GetAddressOfResource(**it));
      delegationUrl.ChangePath(delegationUrl.Path()+"/rest/1.0/delegations");
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
          if(!SubmitterPluginREST::GetDelegationX509(*usercfg, delegationUrl, delegationId)) {
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
          if (!ProcessJobs(usercfg, currentServiceUrl, "restart", 202, IDs, IDsProcessed, IDsNotProcessed, infoNodeProcessor)) {
            ok = false;
          }
        }
        currentServiceUrl = GetAddressOfResource(**it);
      }

      IDs.push_back((*it)->JobID);
    }
    if(!IDs.empty()) {
      if (!ProcessJobs(usercfg, currentServiceUrl, "restart", 202, IDs, IDsProcessed, IDsNotProcessed, infoNodeProcessor)) {
        ok = false;
      }
    }

    return ok;
  }

  bool JobControllerPluginREST::ProcessJobs(const UserConfig* usercfg, Arc::URL const & resourceUrl, std::string const & action, int successCode,
          std::list<std::string>& IDs, std::list<std::string>& IDsProcessed, std::list<std::string>& IDsNotProcessed,
          InfoNodeProcessor& infoNodeProcessor) {
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
      if (!res) {
        logger.msg(WARNING, "Failed to process jobs - error response: %s", std::string(res));
      } else {
        logger.msg(WARNING, "Failed to process jobs - wrong response: %u", info.code);
      }
      if(response && response->Content()) logger.msg(DEBUG, "Content: %s", response->Content());
      delete response; response = NULL;
      for (std::list<std::string>::const_iterator it = IDs.begin(); it != IDs.end(); ++it) {
        logger.msg(WARNING, "Failed to process job: %s", *it);
        IDsNotProcessed.push_back(*it);
      }
      return false;
    }

    if(response->Content()) logger.msg(DEBUG, "Content: %s", response->Content());
    Arc::XMLNode jobs_list(response->Content()?response->Content():"");
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
          infoNodeProcessor(*it, job_item, statusUrl);
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
    case Job::LOGDIR:
      url.ChangePath(url.Path() + "/diagnose");
      {
      URL file(url);
      file.ChangePath(url.Path() + "/failed"); url.AddLocation(URLLocation(file, "failed"));
      file.ChangePath(url.Path() + "/local"); url.AddLocation(URLLocation(file, "local"));
      file.ChangePath(url.Path() + "/errors"); url.AddLocation(URLLocation(file, "errors"));
      file.ChangePath(url.Path() + "/description"); url.AddLocation(URLLocation(file, "description"));
      file.ChangePath(url.Path() + "/diag"); url.AddLocation(URLLocation(file, "diag"));
      file.ChangePath(url.Path() + "/comment"); url.AddLocation(URLLocation(file, "comment"));
      file.ChangePath(url.Path() + "/status"); url.AddLocation(URLLocation(file, "status"));
      file.ChangePath(url.Path() + "/acl"); url.AddLocation(URLLocation(file, "acl"));
      file.ChangePath(url.Path() + "/xml"); url.AddLocation(URLLocation(file, "xml"));
      file.ChangePath(url.Path() + "/input"); url.AddLocation(URLLocation(file, "input"));
      file.ChangePath(url.Path() + "/output"); url.AddLocation(URLLocation(file, "output"));
      file.ChangePath(url.Path() + "/input_status"); url.AddLocation(URLLocation(file, "input_status"));
      file.ChangePath(url.Path() + "/output_status"); url.AddLocation(URLLocation(file, "output_status"));
      file.ChangePath(url.Path() + "/statistics"); url.AddLocation(URLLocation(file, "statistics"));
      }
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

