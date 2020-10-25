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

/*
  JobState::StateType JobStateARCREST::StateMap(const std::string& state) {
ACCEPTING	This is the initial job state. The job has reached the cluster, a session directory was created, the submission client can optionally upload files to the sessiondir. The job waits to be detected by the A-REX, the job processing on the CE hasn’t started yet	ACCEPTED
ACCEPTED	In the ACCEPTED state the newly created job has been detected by A-REX but can’t go to the next state due to an internal A-REX limit. The submission client can optionally upload files to the sessiondir.	PENDING:ACCEPTED
PREPARING	The job is undergoing the data stage-in process, input data is being gathered into the session directory (via external downloads or making cached copies available). During this state the submission client still can upload files to the session directory. This is an I/O heavy job state.	PREPARING
PREPARED	The job successfully completed the data stage-in process and is being held waiting in A-REX’s internal queue before it can be passed over to the batch system	PENDING:PREPARING
SUBMITTING	The job environment (via using RTEs) and the job batch submission script is being prepared to be followed by the submission to the batch system via using the available batch submission client interface	SUBMIT
QUEUING	The job is under the control of the local batch system and is “queuing in the batch system”, waiting for a node/available slot	INLRMS
RUNNING	The job is under the control of the local batch system and is “running in the batch system”, executing on an allocated node under the control of the batch system	INLRMS
HELD	The job is under the control of the local batch system and is being put on hold or being suspended, for some reason the job is in a “pending state” of the batch system	INLRMS
EXITINGLRMS	The job is under the control of the local batch system and is finishing its execution on the worker node, the job is “exiting” from the batch system either because the job is completed or because it was terminated	INLRMS
OTHER	The job is under the control of the local batch system and is in some “other” native batch system state which can not be mapped to any of the previously described batch systems states.	INLRMS
EXECUTED	The job has successfully completed in the batch system. The job is waiting to be picked up by the A-REX for further processing or waiting for an available data stage-out slot.	PENDING:INLRMS
FINISHING	The job is undergoing the data stage-out process, A-REX is moving output data to the specified output file locations, the session directory is being cleaned up. Note that failed or terminated jobs can also undergo the FINISHING state. This is an I/O heavy job state	FINISHING
FINISHED	Successful completion of the job on the cluster. The job has finished ALL its activity on the cluster AND no errors occurred during the job’s lifetime.	FINISHED
FAILED	Unsuccessful completion of the job. The job failed during one of the processing stages. The job has finished ALL its activity on the cluster and there occurred some problems during the lifetime of the job.	FINISHED
KILLING	The job was requested to be terminated by an authorized user and as a result it is being killed. A-REX is terminating any active process related to the job, e.g. it interacts with the LRMS by running the job-cancel script or stops data staging processes. Once the job has finished ALL its activity on the cluster it will be moved to the KILLED state.	CANCELLING
KILLED	The job was terminated as a result of an authorized user request. The job has finished ALL its activity on the cluster.	FINISHED
WIPED	The generated result of jobs are kept available in the session directory on the cluster for a while after the job reaches its final state (FINISHED, FAILED or KILLED). Later, the job’s session directory and most of the job related data are going to be deleted from the cluster when an expiration time is exceeded. Jobs with expired session directory lifetime are “deleted” from the cluster in the sense that only a minimal set of info is kept about such a job and their state is changed to WIPED	DELETED

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
*/

  URL JobControllerPluginREST::GetAddressOfResource(const Job& job) {
    return job.ServiceInformationURL;
  }

  bool JobControllerPluginREST::isEndpointNotSupported(const std::string& endpoint) const {
    const std::string::size_type pos = endpoint.find("://");
    return pos != std::string::npos && lower(endpoint.substr(0, pos)) != "http" && lower(endpoint.substr(0, pos)) != "https";
  }

  void JobControllerPluginREST::UpdateJobs(std::list<Job*>& jobs, std::list<std::string>& IDsProcessed, std::list<std::string>& IDsNotProcessed, bool isGrouped) const {
    class JobInfoNodeProcessor: public InfoNodeProcessor {
     public:
      JobInfoNodeProcessor(std::list<Job*>& jobs): jobs(jobs) {}

      virtual void operator()(std::string const& id, XMLNode node) {
        XMLNode info_document = node["info_document"];
        if(info_document) {






        }
      }

     private:
      std::list<Job*>& jobs;
    };

    JobInfoNodeProcessor infoNodeProcessor(jobs);
    Arc::URL currentServiceUrl;
    std::list<std::string> IDs;
    for (std::list<Job*>::const_iterator it = jobs.begin(); it != jobs.end(); ++it) {
      if(!currentServiceUrl || (currentServiceUrl != GetAddressOfResource(**it))) {
        if(!IDs.empty()) {
          ProcessJobs(currentServiceUrl, "info", IDs, IDsProcessed, IDsNotProcessed, infoNodeProcessor);
        }
        currentServiceUrl = GetAddressOfResource(**it);
      }

      IDs.push_back((*it)->JobID);
    }
    if(!IDs.empty()) {
      ProcessJobs(currentServiceUrl, "info", IDs, IDsProcessed, IDsNotProcessed, infoNodeProcessor);
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
          if (!ProcessJobs(currentServiceUrl, "clean", IDs, IDsProcessed, IDsNotProcessed, infoNodeProcessor))
            ok = false;
        }
        currentServiceUrl = GetAddressOfResource(**it);
      }

      IDs.push_back((*it)->JobID);
    }
    if(!IDs.empty()) {
      if (!ProcessJobs(currentServiceUrl, "clean", IDs, IDsProcessed, IDsNotProcessed, infoNodeProcessor)) {
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
          if (!ProcessJobs(currentServiceUrl, "kill", IDs, IDsProcessed, IDsNotProcessed, infoNodeProcessor))
            ok = false;
        }
        currentServiceUrl = GetAddressOfResource(**it);
      }

      IDs.push_back((*it)->JobID);
    }
    if(!IDs.empty()) {
      if (!ProcessJobs(currentServiceUrl, "kill", IDs, IDsProcessed, IDsNotProcessed, infoNodeProcessor)) {
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
          if (!ProcessJobs(currentServiceUrl, "restart", IDs, IDsProcessed, IDsNotProcessed, infoNodeProcessor)) {
            ok = false;
          }
        }
        currentServiceUrl = GetAddressOfResource(**it);
      }

      IDs.push_back((*it)->JobID);
    }
    if(!IDs.empty()) {
      if (!ProcessJobs(currentServiceUrl, "restart", IDs, IDsProcessed, IDsNotProcessed, infoNodeProcessor)) {
        ok = false;
      }
    }

    return ok;
  }

  bool JobControllerPluginREST::ProcessJobs(Arc::URL const & resourceUrl, std::string const & action,
          std::list<std::string>& IDs, std::list<std::string>& IDsProcessed, std::list<std::string>& IDsNotProcessed,
          InfoNodeProcessor& infoNodeProcessor) const {
    Arc::URL statusUrl(resourceUrl);
    statusUrl.ChangePath(statusUrl.Path()+"/jobs");
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
    Arc::MCC_Status res = client.process(std::string("POST"), &request, &info, &response);
    if((!res) || (info.code != 201)) {
      logger.msg(WARNING, "Failed to process jobs - wrong response");
      delete response; response = NULL;
      for (std::list<std::string>::const_iterator it = IDs.begin(); it != IDs.end(); ++it) {
        logger.msg(WARNING, "Failed to process job: %s", *it);
        IDsNotProcessed.push_back(*it);
      }
      return false;
    }

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
          break;
        }
      }

      std::string jcode = job_item["status-code"];
      std::string jreason = job_item["reason"];
      std::string jid = job_item["id"];
      if(jid.empty()) {
        // hmm
      } else {
        std::list<std::string>::const_iterator it = IDs.begin();
        for (; it != IDs.end(); ++it) {
          std::string id(*it);
          std::string::size_type pos = id.rfind('/');
          if(pos != std::string::npos) id.erase(0,pos+1);
          if (id == jid) break;
        }
        if(it == IDs.end()) {
          // hmm again
        } else {
          if(jcode != "201") {
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
    statusUrl.ChangePath(statusUrl.Path()+"/jobs/"+id+"/diagnose/description");

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

