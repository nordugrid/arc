// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <algorithm>
#include <iostream>

#include <unistd.h>

#include <arc/CheckSum.h>
#include <arc/Logger.h>
#include <arc/UserConfig.h>
#include <arc/client/Broker.h>
#include <arc/client/ComputingServiceRetriever.h>

#include "JobSupervisor.h"

namespace Arc {

  Logger JobSupervisor::logger(Logger::getRootLogger(), "JobSupervisor");

  JobSupervisor::JobSupervisor(const UserConfig& usercfg, const std::list<Job>& jobs)
    : usercfg(usercfg) {
    for (std::list<Job>::const_iterator it = jobs.begin();
         it != jobs.end(); ++it) {
      AddJob(*it);
    }
  }

  bool JobSupervisor::AddJob(const Job& job) {
    if (!job.JobID) {
      logger.msg(VERBOSE, "Ignoring job (%s), the job ID (%s) is not a valid URL", job.JobID.fullstr(), job.JobID.fullstr());
      return false;
    }

    if (job.InterfaceName.empty()) {
      logger.msg(VERBOSE, "Ignoring job (%s), the Job::InterfaceName attribute must be specified", job.JobID.fullstr());
      return false;
    }

    if (!job.Cluster) {
      logger.msg(VERBOSE, "Ignoring job (%s), the resource URL is not a valid URL", job.JobID.fullstr());
      return false;
    }

    std::map<std::string, JobController*>::iterator currentJC = loadedJCs.find(job.InterfaceName);
    if (currentJC == loadedJCs.end()) {
      JobController *jc = Job::loader.loadByInterfaceName(job.InterfaceName, usercfg);
      currentJC = loadedJCs.insert(std::pair<std::string, JobController*>(job.InterfaceName, jc)).first;
      if (!jc) {
        logger.msg(VERBOSE, "Ignoring job (%s), unable to load JobController", job.JobID.fullstr());
        return false;
      }
      jcJobMap[jc] = std::pair< std::list<Job *>, std::list<Job*> >();
    }
    else if (!currentJC->second) {
      // Already tried to load JobController, and it failed.
      logger.msg(VERBOSE, "Ignoring job (%s), unable to load JobController", job.JobID.fullstr());
      return false;
    }

    // Because jobs are identified by ID there should probably be 
    // protection against duplicate job ID
    jobs.push_back(job);
    jobs.back().jc = currentJC->second;
    jcJobMap[currentJC->second].first.push_back(&jobs.back());
    return true;
  }

  void JobSupervisor::SelectValid() {
    processed.clear();
    notprocessed.clear();

    for (JobSelectionMap::iterator it = jcJobMap.begin();
         it != jcJobMap.end(); ++it) {
      for (std::list<Job*>::iterator itJ = it->second.first.begin();
           itJ != it->second.first.end();) {
        if (!(*itJ)->State) {
          notprocessed.push_back((*itJ)->JobID);
          it->second.second.push_back(*itJ);
          itJ = it->second.first.erase(itJ);
        }
        else {
          processed.push_back((*itJ)->JobID);
          ++itJ;
        }
      }
    }
  }

  void JobSupervisor::SelectByStatus(const std::list<std::string>& status) {
    processed.clear();
    notprocessed.clear();

    if (status.empty()) {
      return;
    }

    for (JobSelectionMap::iterator it = jcJobMap.begin();
         it != jcJobMap.end(); ++it) {
      for (std::list<Job*>::iterator itJ = it->second.first.begin();
           itJ != it->second.first.end();) {
        if (std::find(status.begin(), status.end(), (*itJ)->State()) == status.end() &&
            std::find(status.begin(), status.end(), (*itJ)->State.GetGeneralState()) == status.end()) {
          notprocessed.push_back((*itJ)->JobID);
          it->second.second.push_back(*itJ);
          itJ = it->second.first.erase(itJ);
        }
        else {
          processed.push_back((*itJ)->JobID);
          ++itJ;
        }
      }
    }
  }

  void JobSupervisor::SelectByID(const std::list<URL>& ids) {
    processed.clear();
    notprocessed.clear();

    if (ids.empty()) {
      return;
    }

    // For better performance on big lists
    std::list<std::string> ids_str;
    for(std::list<URL>::const_iterator id = ids.begin(); id != ids.end(); ++id) {
      ids_str.push_back(id->fullstr());
    }

    for (JobSelectionMap::iterator it = jcJobMap.begin();
         it != jcJobMap.end(); ++it) {
      for (std::list<Job*>::iterator itJ = it->second.first.begin();
           itJ != it->second.first.end();) {
        if (std::find(ids_str.begin(), ids_str.end(), (*itJ)->JobID.fullstr()) == ids_str.end()) {
          notprocessed.push_back((*itJ)->JobID);
          it->second.second.push_back(*itJ);
          itJ = it->second.first.erase(itJ);
        }
        else {
          processed.push_back((*itJ)->JobID);
          ++itJ;
        }
      }
    }
  }

  void JobSupervisor::ClearSelection() {
    for (JobSelectionMap::iterator it = jcJobMap.begin();
         it != jcJobMap.end(); ++it) {
      it->second.first.clear();
      it->second.second.clear();
    }

    for (std::list<Job>::iterator itJ = jobs.begin();
         itJ != jobs.end(); ++itJ) {
      jcJobMap[itJ->jc].first.push_back(&*itJ);
    }
  }

  std::list<Job> JobSupervisor::GetJobs(bool includeJobsWithoutStateInfo) const {
    if (includeJobsWithoutStateInfo) {
      return jobs;
    }
    std::list<Job> selectedJobs;
    for (std::list<Job>::const_iterator itJ = jobs.begin();
         itJ  != jobs.end(); ++itJ) {
      if (!itJ->State) {
        selectedJobs.push_back(*itJ);
      }
    }

    return selectedJobs;
  }

  void JobSupervisor::Update() {
    for (JobSelectionMap::iterator it = jcJobMap.begin();
         it != jcJobMap.end(); ++it) {
      it->first->UpdateJobs(it->second.first, processed, notprocessed);
    }
  }

  std::list<Job> JobSupervisor::GetSelectedJobs() const {
    std::list<Job> selectedJobs;
    for (JobSelectionMap::const_iterator it = jcJobMap.begin();
         it != jcJobMap.end(); ++it) {
      for (std::list<Job*>::const_iterator itJ = it->second.first.begin();
           itJ != it->second.first.end(); ++itJ) {
        selectedJobs.push_back(**itJ);
      }
    }

    return selectedJobs;
  }

  bool JobSupervisor::Retrieve(const std::string& downloaddirprefix, bool usejobname, bool force, std::list<std::string>& downloaddirectories) {
    notprocessed.clear();
    processed.clear();
    bool ok = true;

    for (JobSelectionMap::iterator it = jcJobMap.begin();
         it != jcJobMap.end(); ++it) {
      for (std::list<Job*>::iterator itJ = it->second.first.begin();
           itJ != it->second.first.end();) {
        if (!(*itJ)->State || (*itJ)->State == JobState::DELETED || !(*itJ)->State.IsFinished()) {
          notprocessed.push_back((*itJ)->JobID);
          it->second.second.push_back(*itJ);
          itJ = it->second.first.erase(itJ);
          continue;
        }

        std::string downloaddirname;
        if (usejobname && !(*itJ)->Name.empty()) {
          downloaddirname = (*itJ)->Name;
        } else {
          std::string path = (*itJ)->JobID.Path();
          std::string::size_type pos = path.rfind('/');
          downloaddirname = path.substr(pos + 1);
        }

        URL downloaddir;
        if (!downloaddirprefix.empty()) {
          downloaddir = downloaddirprefix;
          if (downloaddir.Protocol() == "file") {
            downloaddir.ChangePath(downloaddir.Path() + G_DIR_SEPARATOR_S + downloaddirname);
          } else {
            downloaddir.ChangePath(downloaddir.Path() + "/" + downloaddirname);
          }
        } else {
          downloaddir = downloaddirname;
        }

        if (!(*itJ)->Retrieve(usercfg, downloaddir, force)) {
          ok = false;
          notprocessed.push_back((*itJ)->JobID);
          it->second.second.push_back(*itJ);
          itJ = it->second.first.erase(itJ);
        }
        else {
          processed.push_back((*itJ)->JobID);
          if (downloaddir.Protocol() == "file") {
            std::string cwd = URL(".").Path();
            cwd.resize(cwd.size()-1);
            if (downloaddir.Path().substr(0, cwd.size()) == cwd) {
              downloaddirectories.push_back(downloaddir.Path().substr(cwd.size()));
            } else {
              downloaddirectories.push_back(downloaddir.Path());
            }
          } else {
            downloaddirectories.push_back(downloaddir.str());
          }
          ++itJ;
        }
      }
    }

    return ok;
  }

  bool JobSupervisor::Renew() {
    notprocessed.clear();
    processed.clear();
    bool ok = true;

    for (JobSelectionMap::iterator it = jcJobMap.begin();
         it != jcJobMap.end(); ++it) {
      for (std::list<Job*>::iterator itJ = it->second.first.begin();
           itJ != it->second.first.end();) {
        if (!(*itJ)->State || (*itJ)->State == JobState::FINISHED || (*itJ)->State == JobState::KILLED || (*itJ)->State == JobState::DELETED) {
          notprocessed.push_back((*itJ)->JobID);
          it->second.second.push_back(*itJ);
          itJ = it->second.first.erase(itJ);
          continue;
        }

        if (!it->first->RenewJobs(std::list<Job*>(1, *itJ), processed, notprocessed)) {
          ok = false;
          it->second.second.push_back(*itJ);
          itJ = it->second.first.erase(itJ);
        }
        else {
          ++itJ;
        }
      }
    }

    return ok;
  }

  bool JobSupervisor::Resume() {
    notprocessed.clear();
    processed.clear();
    bool ok = true;

    for (JobSelectionMap::iterator it = jcJobMap.begin();
         it != jcJobMap.end(); ++it) {
      for (std::list<Job*>::iterator itJ = it->second.first.begin();
           itJ != it->second.first.end();) {
        if (!(*itJ)->State || (*itJ)->State == JobState::FINISHED || (*itJ)->State == JobState::KILLED || (*itJ)->State == JobState::DELETED) {
          notprocessed.push_back((*itJ)->JobID);
          it->second.second.push_back(*itJ);
          itJ = it->second.first.erase(itJ);
          continue;
        }

        if (!it->first->ResumeJobs(std::list<Job*>(1, *itJ), processed, notprocessed)) {
          ok = false;
          it->second.second.push_back(*itJ);
          itJ = it->second.first.erase(itJ);
        }
        else {
          ++itJ;
        }
      }
    }

    return ok;
  }

  bool JobSupervisor::Resubmit(int destination, const std::list<Endpoint>& services, std::list<Job>& resubmittedJobs, const std::list<std::string>& rejectedURLs) {
    notprocessed.clear();
    processed.clear();
    bool ok = true;

    std::list< std::list<Job*>::iterator > resubmittableJobs;
    for (JobSelectionMap::iterator it = jcJobMap.begin();
         it != jcJobMap.end(); ++it) {
      for (std::list<Job*>::iterator itJ = it->second.first.begin();
           itJ != it->second.first.end();) {
        // If job description is not set, then try to fetch it from execution service.
        if ((*itJ)->JobDescriptionDocument.empty() && !it->first->GetJobDescription(**itJ, (*itJ)->JobDescriptionDocument)) {
          notprocessed.push_back((*itJ)->JobID);
          ok = false;
          it->second.second.push_back(*itJ);
          itJ = it->second.first.erase(itJ);
          continue;
        }

        // Verify checksums of local input files
        if (!(*itJ)->LocalInputFiles.empty()) {
          std::map<std::string, std::string>::iterator itF = (*itJ)->LocalInputFiles.begin();
          for (; itF != (*itJ)->LocalInputFiles.end(); ++itF) {
            if (itF->second != CheckSumAny::FileChecksum(itF->first, CheckSumAny::cksum, true)) {
              break;
            }
          }

          if (itF != (*itJ)->LocalInputFiles.end()) {
            notprocessed.push_back((*itJ)->JobID);
            ok = false;
            it->second.second.push_back(*itJ);
            itJ = it->second.first.erase(itJ);
            continue;
          }
        }

        resubmittableJobs.push_back(itJ);
        ++itJ;
      }
    }

    if (resubmittableJobs.empty()) {
      return ok;
    }

    UserConfig resubmitUsercfg = usercfg; // UserConfig object might need to be modified.
    Broker broker(resubmitUsercfg, resubmitUsercfg.Broker().first);
    if (!broker.isValid()) {
      logger.msg(ERROR, "Job resubmission failed: Unable to load broker (%s)", resubmitUsercfg.Broker().first);
      for (std::list< std::list<Job*>::iterator >::iterator itJ = resubmittableJobs.begin();
           itJ != resubmittableJobs.end(); ++itJ) {
        notprocessed.push_back((**itJ)->JobID);
        jcJobMap[(**itJ)->jc].second.push_back(**itJ);
        jcJobMap[(**itJ)->jc].first.erase(*itJ);
      }
      return false;
    }

    ComputingServiceRetriever* csr = NULL;
    if (destination != 1) { // Jobs should not go to same target, making a general information gathering.
      csr = new ComputingServiceRetriever(resubmitUsercfg, services, rejectedURLs);
      csr->wait();
      if (csr->empty()) {
        logger.msg(ERROR, "Job resubmission aborted because no resource returned any information");
        delete csr;
        for (std::list< std::list<Job*>::iterator >::iterator itJ = resubmittableJobs.begin();
             itJ != resubmittableJobs.end(); ++itJ) {
          notprocessed.push_back((**itJ)->JobID);
          jcJobMap[(**itJ)->jc].second.push_back(**itJ);
          jcJobMap[(**itJ)->jc].first.erase(*itJ);
        }
        return false;
      }

    }

    for (std::list< std::list<Job*>::iterator >::iterator itJ = resubmittableJobs.begin();
         itJ != resubmittableJobs.end(); ++itJ) {
      resubmittedJobs.push_back(Job());

      std::list<JobDescription> jobdescs;
      if (!JobDescription::Parse((**itJ)->JobDescriptionDocument, jobdescs) || jobdescs.empty()) {
        logger.msg(ERROR, "Unable to resubmit job (%s), unable to parse obtained job description", (**itJ)->JobID.fullstr());
        resubmittedJobs.pop_back();
        notprocessed.push_back((**itJ)->JobID);
        ok = false;
        jcJobMap[(**itJ)->jc].second.push_back(**itJ);
        jcJobMap[(**itJ)->jc].first.erase(*itJ);
        continue;
      }
      jobdescs.front().Identification.ActivityOldID = (**itJ)->ActivityOldID;
      jobdescs.front().Identification.ActivityOldID.push_back((**itJ)->JobID.fullstr());

      // remove the queuename which was added during the original submission of the job
      jobdescs.front().Resources.QueueName = "";

      std::list<URL> rejectEndpoints;
      if (destination == 1) { // Jobs should be resubmitted to same target.
        std::list<Endpoint> sametarget(1, Endpoint((**itJ)->Cluster.fullstr()));
        sametarget.front().Capability.push_back(Arc::Endpoint::GetStringForCapability(Arc::Endpoint::COMPUTINGINFO));

        csr = new ComputingServiceRetriever(resubmitUsercfg, sametarget, rejectedURLs);
        csr->wait();
        if (csr->empty()) {
          logger.msg(ERROR, "Unable to resubmit job (%s), target information retrieval failed for target: %s", (**itJ)->JobID.fullstr(), (**itJ)->Cluster.str());
          delete csr;
          resubmittedJobs.pop_back();
          notprocessed.push_back((**itJ)->JobID);
          jcJobMap[(**itJ)->jc].second.push_back(**itJ);
          jcJobMap[(**itJ)->jc].first.erase(*itJ);
          continue;
        }
      }
      else if (destination == 2) { // Jobs should NOT be resubmitted to same target.
        rejectEndpoints.push_back((**itJ)->Cluster);
      }

      broker.set(jobdescs.front());
      ExecutionTargetSet ets(broker, *csr, rejectEndpoints);
      ExecutionTargetSet::iterator it = ets.begin();
      for (; it != ets.end(); ++it) {
        if (it->Submit(resubmitUsercfg, jobdescs.front(), resubmittedJobs.back())) {
          it->RegisterJobSubmission(jobdescs.front());
          processed.push_back((**itJ)->JobID);
          break;
        }
      }
      
      if (it == ets.end()) {
        resubmittedJobs.pop_back();
        notprocessed.push_back((**itJ)->JobID);
        ok = false;
        logger.msg(ERROR, "Unable to resubmit job (%s), no targets applicable for submission", (**itJ)->JobID.fullstr());
        jcJobMap[(**itJ)->jc].second.push_back(**itJ);
        jcJobMap[(**itJ)->jc].first.erase(*itJ);
      }

      if (destination == 1) {
        delete csr;
      }
    }

    if (destination != 1) {
      delete csr;
    }

    return ok;
  }

  bool JobSupervisor::Migrate(bool forcemigration, const std::list<Endpoint>& services, std::list<Job>& migratedJobs, const std::list<std::string>& rejectedURLs) {
    bool ok = true;

    std::list< std::list<Job*>::iterator > migratableJobs;
    for (JobSelectionMap::iterator it = jcJobMap.begin();
         ++it != jcJobMap.end(); ++it) {
      for (std::list<Job*>::iterator itJ = it->second.first.begin();
           itJ != it->second.first.end();) {
        if ((*itJ)->State != JobState::QUEUING) {
          notprocessed.push_back((*itJ)->JobID);
          ok = false;
          it->second.second.push_back(*itJ);
          itJ = it->second.first.erase(itJ);
          continue;
        }

        // If job description is not set, then try to fetch it from execution service.
        if ((*itJ)->JobDescriptionDocument.empty() && !it->first->GetJobDescription((**itJ), (*itJ)->JobDescriptionDocument)) {
          logger.msg(ERROR, "Unable to migrate job (%s), job description could not be retrieved remotely", (*itJ)->JobID.fullstr());
          notprocessed.push_back((*itJ)->JobID);
          ok = false;
          it->second.second.push_back(*itJ);
          itJ = it->second.first.erase(itJ);
          continue;
        }

        migratableJobs.push_back(itJ);
        ++itJ;
      }
    }

    if (migratableJobs.empty()) {
      return ok;
    }

    ComputingServiceRetriever csr(usercfg, services, rejectedURLs);
    csr.wait();
    if (csr.empty()) {
      logger.msg(ERROR, "Job migration aborted, no resource returned any information");
      for (std::list< std::list<Job*>::iterator >::const_iterator itJ = migratableJobs.begin();
           itJ != migratableJobs.end(); ++itJ) {
        notprocessed.push_back((**itJ)->JobID);
        jcJobMap[(**itJ)->jc].second.push_back(**itJ);
        jcJobMap[(**itJ)->jc].first.erase(*itJ);
      }
      return false;
    }

    Broker broker(usercfg, usercfg.Broker().first);
    if (!broker.isValid()) {
      logger.msg(ERROR, "Job migration aborted, unable to load broker (%s)", usercfg.Broker().first);
      for (std::list< std::list<Job*>::iterator >::const_iterator itJ = migratableJobs.begin();
           itJ != migratableJobs.end(); ++itJ) {
        notprocessed.push_back((**itJ)->JobID);
        jcJobMap[(**itJ)->jc].second.push_back(**itJ);
        jcJobMap[(**itJ)->jc].first.erase(*itJ);
      }
      return false;
    }

    for (std::list< std::list<Job*>::iterator >::iterator itJ = migratableJobs.begin();
         itJ != migratableJobs.end(); ++itJ) {
      std::list<JobDescription> jobdescs;
      if (!JobDescription::Parse((**itJ)->JobDescriptionDocument, jobdescs) || jobdescs.empty()) {
        logger.msg(ERROR, "Unable to migrate job (%s), unable to parse obtained job description", (**itJ)->JobID.fullstr());
        notprocessed.push_back((**itJ)->JobID);
        jcJobMap[(**itJ)->jc].second.push_back(**itJ);
        jcJobMap[(**itJ)->jc].first.erase(*itJ);
        continue;
      }
      jobdescs.front().Identification.ActivityOldID = (**itJ)->ActivityOldID;
      jobdescs.front().Identification.ActivityOldID.push_back((**itJ)->JobID.fullstr());

      // remove the queuename which was added during the original submission of the job
      jobdescs.front().Resources.QueueName = "";

      migratedJobs.push_back(Job());

      broker.set(jobdescs.front());
      ExecutionTargetSet ets(broker, csr);
      ExecutionTargetSet::iterator it = ets.begin();
      for (; it != ets.end(); ++it) {
        if (it->Migrate(usercfg, (**itJ)->JobID, jobdescs.front(), forcemigration, migratedJobs.back())) {
          it->RegisterJobSubmission(jobdescs.front());
          break;
        }
      }

      if (it == ets.end()) {
        logger.msg(ERROR, "Job migration failed for job (%s), no applicable targets", (**itJ)->JobID.fullstr());
        ok = false;
        migratedJobs.pop_back();
        notprocessed.push_back((**itJ)->JobID);
        jcJobMap[(**itJ)->jc].second.push_back(**itJ);
        jcJobMap[(**itJ)->jc].first.erase(*itJ);
      }
      else {
        processed.push_back((**itJ)->JobID);
      }
    }

    return ok;
  }

  bool JobSupervisor::Cancel() {
    notprocessed.clear();
    processed.clear();
    bool ok = true;

    for (JobSelectionMap::iterator it = jcJobMap.begin();
         it != jcJobMap.end(); ++it) {
      for (std::list<Job*>::iterator itJ = it->second.first.begin();
           itJ != it->second.first.end();) {
        if (!(*itJ)->State || (*itJ)->State == JobState::DELETED || (*itJ)->State.IsFinished()) {
          notprocessed.push_back((*itJ)->JobID);
          it->second.second.push_back(*itJ);
          itJ = it->second.first.erase(itJ);
          continue;
        }

        if (!it->first->CancelJobs(std::list<Job*>(1, *itJ), processed, notprocessed)) {
          ok = false;
          it->second.second.push_back(*itJ);
          itJ = it->second.first.erase(itJ);
        }
        else {
          ++itJ;
        }
      }
    }

    return ok;
  }

  bool JobSupervisor::Clean() {
    notprocessed.clear();
    processed.clear();
    bool ok = true;

    for (JobSelectionMap::iterator it = jcJobMap.begin();
         it != jcJobMap.end(); ++it) {
      for (std::list<Job*>::iterator itJ = it->second.first.begin();
           itJ != it->second.first.end();) {
        if (!(*itJ)->State || !(*itJ)->State.IsFinished()) {
          notprocessed.push_back((*itJ)->JobID);
          it->second.second.push_back(*itJ);
          itJ = it->second.first.erase(itJ);
          continue;
        }

        if (!it->first->CleanJobs(std::list<Job*>(1, *itJ), processed, notprocessed)) {
          ok = false;
          it->second.second.push_back(*itJ);
          itJ = it->second.first.erase(itJ);
        }
        else {
          ++itJ;
        }
      }
    }

    return ok;
  }
} // namespace Arc
