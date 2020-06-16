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
    return true;
  }

  JobState::StateType JobStateARCREST::StateMap(const std::string& state) {
    /// \mapname ARCBES ARC extended BES
    /// \mapnote The mapping is case-insensitive, and prefix "pending:" are ignored when performing the mapping.
    std::string state_ = Arc::lower(state);
    std::string::size_type p = state_.find("pending:");
    if(p != std::string::npos) {
      state_.erase(p,8);
    }
    /// \mapattr accepted -> ACCEPTED
    if (state_ == "accepted")
      return JobState::ACCEPTED;
    /// \mapattr preparing -> PREPARING
    /// \mapattr prepared -> PREPARING
    else if (state_ == "preparing")
      return JobState::PREPARING;
    /// \mapattr submit -> SUBMITTING
    else if (state_ == "submit")
      return JobState::SUBMITTING;
    /// \mapattr inlrms -> RUNNING
    else if (state_ == "inlrms")
      return JobState::RUNNING;
    /// \mapattr canceling -> RUNNING
    else if (state_ == "canceling")
      return JobState::RUNNING;
    /// \mapattr finishing -> FINISHING
    else if (state_ == "finishing")
      return JobState::FINISHING;
    /// \mapattr finished -> FINISHED
    else if (state_ == "finished")
      return JobState::FINISHED;
    /// \mapattr deleted -> DELETED
    else if (state_ == "deleted")
      return JobState::DELETED;
    /// \mapattr "" -> UNDEFINED
    else if (state_ == "")
      return JobState::UNDEFINED;
    /// \mapattr Any other state -> OTHER
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

  static char const * LogsPrefix = "/*logs";
  static char const * DelegPrefix = "/*deleg";

  void JobControllerPluginREST::UpdateJobs(std::list<Job*>& jobs, std::list<std::string>& IDsProcessed, std::list<std::string>& IDsNotProcessed, bool isGrouped) const {
    for (std::list<Job*>::iterator it = jobs.begin(); it != jobs.end(); ++it) {
      Arc::URL statusUrl(GetAddressOfResource(**it));
      std::string id((*it)->JobID);
      std::string::size_type pos = id.rfind('/');
      if(pos != std::string::npos) id.erase(0,pos+1);
      statusUrl.ChangePath(statusUrl.Path()+LogsPrefix+"/"+id+"/status"); // simple state
      Arc::MCCConfig cfg;
      usercfg->ApplyToConfig(cfg);
      Arc::ClientHTTP client(cfg, statusUrl);
      Arc::PayloadRaw request;
      Arc::PayloadRawInterface* response(NULL);
      Arc::HTTPClientInfo info;
      Arc::MCC_Status res = client.process(std::string("GET"), &request, &info, &response);
      if((!res) || (info.code != 200) || (response == NULL) || (response->Buffer(0) == NULL)) {
        delete response;
        logger.msg(WARNING, "Job information not found in the information system: %s", (*it)->JobID);
        IDsNotProcessed.push_back((*it)->JobID);
        continue;
      }
      (*it)->State = JobStateARCREST(std::string(response->Buffer(0),response->BufferSize(0)));
      delete response;
      (*it)->LogDir = std::string(LogsPrefix);
      IDsProcessed.push_back((*it)->JobID);
    }
  }

  bool JobControllerPluginREST::CleanJobs(const std::list<Job*>& jobs, std::list<std::string>& IDsProcessed, std::list<std::string>& IDsNotProcessed, bool isGrouped) const {
    bool ok = true;
    for (std::list<Job*>::const_iterator it = jobs.begin(); it != jobs.end(); ++it) {
      Arc::URL statusUrl(GetAddressOfResource(**it));
      std::string id((*it)->JobID);
      std::string::size_type pos = id.rfind('/');
      if(pos != std::string::npos) id.erase(0,pos+1);
      statusUrl.ChangePath(statusUrl.Path()+LogsPrefix+"/"+id+"/status");
      Arc::MCCConfig cfg;
      usercfg->ApplyToConfig(cfg);
      Arc::ClientHTTP client(cfg, statusUrl);
      Arc::PayloadRaw request;
      std::string const new_state("DELETED");
      request.Insert(new_state.c_str(),0,new_state.length());
      Arc::PayloadRawInterface* response(NULL);
      Arc::HTTPClientInfo info;
      Arc::MCC_Status res = client.process(std::string("PUT"), &request, &info, &response);
      delete response;
      if((!res) || (info.code != 200)) {
        logger.msg(WARNING, "Failed to clean job: %s", (*it)->JobID);
        IDsNotProcessed.push_back((*it)->JobID);
        ok = false;
        continue;
      }
      IDsProcessed.push_back((*it)->JobID);
    }

    return ok;
  }

  bool JobControllerPluginREST::CancelJobs(const std::list<Job*>& jobs, std::list<std::string>& IDsProcessed, std::list<std::string>& IDsNotProcessed, bool isGrouped) const {
    bool ok = true;
    for (std::list<Job*>::const_iterator it = jobs.begin(); it != jobs.end(); ++it) {
      Arc::URL statusUrl(GetAddressOfResource(**it));
      std::string id((*it)->JobID);
      std::string::size_type pos = id.rfind('/');
      if(pos != std::string::npos) id.erase(0,pos+1);
      statusUrl.ChangePath(statusUrl.Path()+LogsPrefix+"/"+id+"/status");
      Arc::MCCConfig cfg;
      usercfg->ApplyToConfig(cfg);
      Arc::ClientHTTP client(cfg, statusUrl);
      Arc::PayloadRaw request;
      std::string const new_state("FINISHED");
      request.Insert(new_state.c_str(),0,new_state.length());
      Arc::PayloadRawInterface* response(NULL);
      Arc::HTTPClientInfo info;
      Arc::MCC_Status res = client.process(std::string("PUT"), &request, &info, &response);
      delete response;
      if((!res) || (info.code != 200)) {
        logger.msg(WARNING, "Failed to cancel job: %s", (*it)->JobID);
        IDsNotProcessed.push_back((*it)->JobID);
        ok = false;
        continue;
      }
      (*it)->State = JobStateARCREST("FINISHED");
      IDsProcessed.push_back((*it)->JobID);
    }

    return ok;
  }

  bool JobControllerPluginREST::RenewJobs(const std::list<Job*>& jobs, std::list<std::string>& IDsProcessed, std::list<std::string>& IDsNotProcessed, bool isGrouped) const {
    bool ok = true;
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
    return ok;
  }

  bool JobControllerPluginREST::ResumeJobs(const std::list<Job*>& jobs, std::list<std::string>& IDsProcessed, std::list<std::string>& IDsNotProcessed, bool isGrouped) const {
    bool ok = true;
    for (std::list<Job*>::const_iterator it = jobs.begin(); it != jobs.end(); ++it) {
      Arc::URL statusUrl(GetAddressOfResource(**it));
      std::string id((*it)->JobID);
      std::string::size_type pos = id.rfind('/');
      if(pos != std::string::npos) id.erase(0,pos+1);
      statusUrl.ChangePath(statusUrl.Path()+LogsPrefix+"/"+id+"/status");
      Arc::MCCConfig cfg;
      usercfg->ApplyToConfig(cfg);
      Arc::ClientHTTP client(cfg, statusUrl);
      Arc::PayloadRaw request;
      // It is not really important which state is requested.
      // Server will handle moving job to last failed state anyway.
      std::string const new_state("PREPARING");
      request.Insert(new_state.c_str(),0,new_state.length());
      Arc::PayloadRawInterface* response(NULL);
      Arc::HTTPClientInfo info;
      Arc::MCC_Status res = client.process(std::string("PUT"), &request, &info, &response);
      delete response;
      if((!res) || (info.code != 200)) {
        logger.msg(WARNING, "Failed to cancel job: %s", (*it)->JobID);
        IDsNotProcessed.push_back((*it)->JobID);
        ok = false;
        continue;
      }
      (*it)->State = JobStateARCREST("FINISHED");
      IDsProcessed.push_back((*it)->JobID);
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
      url.ChangePath(url.Path() + '/' + job.StdIn);
      break;
    case Job::STDOUT:
      if(job.StdOut.empty()) return false;
      url.ChangePath(url.Path() + '/' + job.StdOut);
      break;
    case Job::STDERR:
      if(job.StdErr.empty()) return false;
      url.ChangePath(url.Path() + '/' + job.StdErr);
      break;
    case Job::STAGEINDIR:
    case Job::STAGEOUTDIR:
    case Job::SESSIONDIR:
      break;
    case Job::JOBLOG:
    case Job::JOBDESCRIPTION:
      std::string path = url.Path();
      path.insert(path.rfind('/'), LogsPrefix);
      url.ChangePath(path + ((resource == Job::JOBLOG) ? "/errors" : "/description"));
      break;
    }

    return true;
  }

  bool JobControllerPluginREST::GetJobDescription(const Job& job, std::string& desc_str) const {
    Arc::URL statusUrl(GetAddressOfResource(job));
    std::string id(job.JobID);
    std::string::size_type pos = id.rfind('/');
    if(pos != std::string::npos) id.erase(0,pos+1);
    statusUrl.ChangePath(statusUrl.Path()+LogsPrefix+"/"+id+"/description");
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
