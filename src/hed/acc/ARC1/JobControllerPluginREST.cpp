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

#include "JobControllerPluginREST.h"
#include "JobStateARC1.h"

namespace Arc {

  Logger JobControllerPluginREST::logger(Logger::getRootLogger(), "JobControllerPlugin.REST");

  URL JobControllerPluginREST::GetAddressOfResource(const Job& job) {
    return job.ServiceInformationURL;
  }

  bool JobControllerPluginREST::isEndpointNotSupported(const std::string& endpoint) const {
    const std::string::size_type pos = endpoint.find("://");
    return pos != std::string::npos && lower(endpoint.substr(0, pos)) != "http" && lower(endpoint.substr(0, pos)) != "https";
  }

  void JobControllerPluginREST::UpdateJobs(std::list<Job*>& jobs, std::list<std::string>& IDsProcessed, std::list<std::string>& IDsNotProcessed, bool isGrouped) const {
    for (std::list<Job*>::iterator it = jobs.begin(); it != jobs.end(); ++it) {
      Arc::URL statusUrl(GetAddressOfResource(**it));
      std::string id((*it)->JobID);
      std::string::size_type pos = id.rfind('/');
      if(pos != std::string::npos) id.erase(0,pos+1);
      statusUrl.ChangePath(statusUrl.Path()+"/*logs/"+id+"/status"); // simple state
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
      (*it)->State = JobStateARC1(std::string(response->Buffer(0),response->BufferSize(0)));
      delete response;
      IDsProcessed.push_back((*it)->JobID);
    }
  }

  bool JobControllerPluginREST::CleanJobs(const std::list<Job*>& jobs, std::list<std::string>& IDsProcessed, std::list<std::string>& IDsNotProcessed, bool isGrouped) const {
    bool ok = true;
    for (std::list<Job*>::const_iterator it = jobs.begin(); it != jobs.end(); ++it) {
      Job& job = **it;
      /*
      AutoPointer<AREXClient> ac(clients.acquire(GetAddressOfResource(job), true));
      std::string idstr;
      AREXClient::createActivityIdentifier(URL(job.JobID), idstr);
      if (!ac->clean(idstr)) {
      */
        ok = false;
        IDsNotProcessed.push_back(job.JobID);
      //  ((AREXClients&)clients).release(ac.Release());
        continue;
      /*
      }
      IDsProcessed.push_back(job.JobID);
      ((AREXClients&)clients).release(ac.Release());
      */
    }

    return ok;
  }

  bool JobControllerPluginREST::CancelJobs(const std::list<Job*>& jobs, std::list<std::string>& IDsProcessed, std::list<std::string>& IDsNotProcessed, bool isGrouped) const {
    bool ok = true;
    for (std::list<Job*>::const_iterator it = jobs.begin(); it != jobs.end(); ++it) {
      Job& job = **it;
      /*
      AutoPointer<AREXClient> ac(clients.acquire(GetAddressOfResource(job), true));
      std::string idstr;
      AREXClient::createActivityIdentifier(URL(job.JobID), idstr);
      if (!ac->kill(idstr)) {
      */
        ok = false;
        IDsNotProcessed.push_back(job.JobID);
      //  ((AREXClients&)clients).release(ac.Release());
        continue;
      /*
      }
      job.State = JobStateARC1("killed");
      IDsProcessed.push_back(job.JobID);
      ((AREXClients&)clients).release(ac.Release());
      */
    }

    return ok;
  }

  bool JobControllerPluginREST::RenewJobs(const std::list<Job*>& jobs, std::list<std::string>& IDsProcessed, std::list<std::string>& IDsNotProcessed, bool isGrouped) const {
    for (std::list<Job*>::const_iterator it = jobs.begin(); it != jobs.end(); ++it) {
      logger.msg(INFO, "Renewal of REST jobs is not supported");
      IDsNotProcessed.push_back((*it)->JobID);
    }
    return false;
  }

  bool JobControllerPluginREST::ResumeJobs(const std::list<Job*>& jobs, std::list<std::string>& IDsProcessed, std::list<std::string>& IDsNotProcessed, bool isGrouped) const {
    bool ok = true;
    for (std::list<Job*>::const_iterator it = jobs.begin(); it != jobs.end(); ++it) {
      Job& job = **it;
      if (!job.RestartState) {
        logger.msg(INFO, "Job %s does not report a resumable state", job.JobID);
        ok = false;
        IDsNotProcessed.push_back(job.JobID);
        continue;
      }

      logger.msg(VERBOSE, "Resuming job: %s at state: %s (%s)", job.JobID, job.RestartState.GetGeneralState(), job.RestartState());

      /*
      AutoPointer<AREXClient> ac(clients.acquire(GetAddressOfResource(job), true));
      std::string idstr;
      AREXClient::createActivityIdentifier(URL(job.JobID), idstr);
      if (!ac->resume(idstr)) {
      */
        ok = false;
        IDsNotProcessed.push_back(job.JobID);
      //  ((AREXClients&)clients).release(ac.Release());
        continue;
      /*
      }

      IDsProcessed.push_back(job.JobID);
      ((AREXClients&)clients).release(ac.Release());
      logger.msg(VERBOSE, "Job resuming successful");
      */
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
      url.ChangePath(url.Path() + '/' + job.StdIn);
      break;
    case Job::STDOUT:
      url.ChangePath(url.Path() + '/' + job.StdOut);
      break;
    case Job::STDERR:
      url.ChangePath(url.Path() + '/' + job.StdErr);
      break;
    case Job::STAGEINDIR:
    case Job::STAGEOUTDIR:
    case Job::SESSIONDIR:
      break;
    case Job::JOBLOG:
    case Job::JOBDESCRIPTION:
      std::string path = url.Path();
      path.insert(path.rfind('/'), "/*logs");
      url.ChangePath(path + (Job::JOBLOG ? "/errors" : "/description"));
      break;
    }

    return true;
  }

  bool JobControllerPluginREST::GetJobDescription(const Job& job, std::string& desc_str) const {
    /*
    MCCConfig cfg;
    usercfg->ApplyToConfig(cfg);
    AutoPointer<AREXClient> ac(clients.acquire(GetAddressOfResource(job), true));
    std::string idstr;
    AREXClient::createActivityIdentifier(URL(job.JobID), idstr);
    if (ac->getdesc(idstr, desc_str)) {
      std::list<JobDescription> descs;
      if (JobDescription::Parse(desc_str, descs) && !descs.empty()) {
        ((AREXClients&)clients).release(ac.Release());
        return true;
      }
    }
    ((AREXClients&)clients).release(ac.Release());
    */

    logger.msg(ERROR, "Failed retrieving job description for job: %s", job.JobID);
    return false;
  }
} // namespace Arc
