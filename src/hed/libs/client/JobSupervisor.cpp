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
#include <arc/client/EntityRetriever.h>

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

  JobSupervisor::~JobSupervisor() {}

  bool JobSupervisor::AddJob(const Job& job) {
    if (job.Flavour.empty()) {
      logger.msg(VERBOSE, "Ignoring job (%s), the Job::Flavour attribute must be specified", job.IDFromEndpoint.fullstr());
      return false;
    }

    if (!job.JobID) {
      logger.msg(VERBOSE, "Ignoring job (%s), the job ID (%s) is not a valid URL", job.JobID.fullstr(), job.JobID.fullstr());
      return false;
    }

    if (!job.Cluster) {
      logger.msg(VERBOSE, "Ignoring job (%s), the resource URL is not a valid URL", job.JobID.fullstr(), job.Cluster.str());
      return false;
    }

    std::map<std::string, JobController*>::iterator currentJC = loadedJCs.find(job.Flavour);
    if (currentJC == loadedJCs.end()) {
      JobController *jc = loader.load(job.Flavour, usercfg);
      currentJC = loadedJCs.insert(std::pair<std::string, JobController*>(job.Flavour, jc)).first;
      if (!jc) {
        logger.msg(WARNING, "Unable to load JobController %s plugin. Is the %s plugin installed?", job.Flavour, job.Flavour);
        return false;
      }
      jcJobMap[jc] = std::pair< std::list<Job *>, std::list<Job*> >();
    }
    else if (!currentJC->second) {
      // Already tried to load JobController, and it failed.
      return false;
    }

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
      it->first->UpdateJobs(it->second.first);
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

        std::string downloaddir = downloaddirprefix;
        if (!it->first->RetrieveJob(**itJ, downloaddir, usejobname, force)) {
          ok = false;
          notprocessed.push_back((*itJ)->JobID);
          it->second.second.push_back(*itJ);
          itJ = it->second.first.erase(itJ);
        }
        else {
          processed.push_back((*itJ)->JobID);
          downloaddirectories.push_back(downloaddir);
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

        if (!it->first->RenewJob(**itJ)) {
          ok = false;
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

        if (!it->first->ResumeJob(**itJ)) {
          ok = false;
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
          notprocessed.push_back((*itJ)->IDFromEndpoint);
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
            notprocessed.push_back((*itJ)->IDFromEndpoint);
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
    BrokerLoader brokerLoader;
    Broker *chosenBroker = brokerLoader.load(resubmitUsercfg.Broker().first, resubmitUsercfg);
    if (!chosenBroker) {
      logger.msg(ERROR, "Job resubmission failed: Unable to load broker (%s)", resubmitUsercfg.Broker().first);
      for (std::list< std::list<Job*>::iterator >::iterator itJ = resubmittableJobs.begin();
           itJ != resubmittableJobs.end(); ++itJ) {
        notprocessed.push_back((**itJ)->IDFromEndpoint);
        jcJobMap[(**itJ)->jc].second.push_back(**itJ);
        jcJobMap[(**itJ)->jc].first.erase(*itJ);
      }
      return false;
    }

    ExecutionTargetRetriever* etr = NULL;
    if (destination != 1) { // Jobs should not go to same target, making a general information gathering.
      etr = new ExecutionTargetRetriever(resubmitUsercfg, services, rejectedURLs);
      etr->wait();
      if (etr->empty()) {
        logger.msg(ERROR, "Job resubmission aborted because no resource returned any information");
        delete etr;
        for (std::list< std::list<Job*>::iterator >::iterator itJ = resubmittableJobs.begin();
             itJ != resubmittableJobs.end(); ++itJ) {
          notprocessed.push_back((**itJ)->IDFromEndpoint);
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
        logger.msg(ERROR, "Unable to resubmit job (%s), unable to parse obtained job description", (**itJ)->IDFromEndpoint.fullstr());
        resubmittedJobs.pop_back();
        notprocessed.push_back((**itJ)->IDFromEndpoint);
        ok = false;
        jcJobMap[(**itJ)->jc].second.push_back(**itJ);
        jcJobMap[(**itJ)->jc].first.erase(*itJ);
        continue;
      }
      jobdescs.front().Identification.ActivityOldID = (**itJ)->ActivityOldID;
      jobdescs.front().Identification.ActivityOldID.push_back((**itJ)->JobID.fullstr());

      // remove the queuename which was added during the original submission of the job
      jobdescs.front().Resources.QueueName = "";

      std::list<URL> rejectTargets;
      if (destination == 1) { // Jobs should be resubmitted to same target.
        std::list<Endpoint> sametarget(1, Endpoint((**itJ)->Cluster.fullstr()));
        sametarget.front().Capability.push_back(Arc::Endpoint::GetStringForCapability(Arc::Endpoint::COMPUTINGINFO));

        etr = new ExecutionTargetRetriever(resubmitUsercfg, sametarget, rejectedURLs);
        etr->wait();
        if (etr->empty()) {
          logger.msg(ERROR, "Unable to resubmit job (%s), target information retrieval failed for target: %s", (**itJ)->IDFromEndpoint.fullstr(), (**itJ)->Cluster.str());
          delete etr;
          resubmittedJobs.pop_back();
          notprocessed.push_back((**itJ)->IDFromEndpoint);
          jcJobMap[(**itJ)->jc].second.push_back(**itJ);
          jcJobMap[(**itJ)->jc].first.erase(*itJ);
          continue;
        }
      }
      else if (destination == 2) { // Jobs should NOT be resubmitted to same target.
        rejectTargets.push_back((**itJ)->Cluster);
      }

      if (!chosenBroker->Submit(*etr, jobdescs.front(), resubmittedJobs.back(), rejectTargets)) {
        resubmittedJobs.pop_back();
        notprocessed.push_back((**itJ)->IDFromEndpoint);
        ok = false;
        logger.msg(ERROR, "Unable to resubmit job (%s), no targets applicable for submission", (**itJ)->IDFromEndpoint.fullstr());
        jcJobMap[(**itJ)->jc].second.push_back(**itJ);
        jcJobMap[(**itJ)->jc].first.erase(*itJ);
      }
      else {
        processed.push_back((**itJ)->IDFromEndpoint);
      }

      if (destination == 1) {
        delete etr;
      }
    }

    if (destination != 1) {
      delete etr;
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
          notprocessed.push_back((*itJ)->IDFromEndpoint);
          ok = false;
          it->second.second.push_back(*itJ);
          itJ = it->second.first.erase(itJ);
          continue;
        }

        // If job description is not set, then try to fetch it from execution service.
        if ((*itJ)->JobDescriptionDocument.empty() && !it->first->GetJobDescription((**itJ), (*itJ)->JobDescriptionDocument)) {
          logger.msg(ERROR, "Unable to migrate job (%s), job description could not be retrieved remotely", (*itJ)->IDFromEndpoint.fullstr());
          notprocessed.push_back((*itJ)->IDFromEndpoint);
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

    ExecutionTargetRetriever etr(usercfg, services, rejectedURLs);
    etr.wait();
    if (etr.empty()) {
      logger.msg(ERROR, "Job migration aborted, no resource returned any information");
      for (std::list< std::list<Job*>::iterator >::const_iterator itJ = migratableJobs.begin();
           itJ != migratableJobs.end(); ++itJ) {
        notprocessed.push_back((**itJ)->IDFromEndpoint);
        jcJobMap[(**itJ)->jc].second.push_back(**itJ);
        jcJobMap[(**itJ)->jc].first.erase(*itJ);
      }
      return false;
    }

    BrokerLoader brokerLoader;
    Broker *chosenBroker = brokerLoader.load(usercfg.Broker().first, usercfg);
    if (!chosenBroker) {
      logger.msg(ERROR, "Job migration aborted, unable to load broker (%s)", usercfg.Broker().first);
      for (std::list< std::list<Job*>::iterator >::const_iterator itJ = migratableJobs.begin();
           itJ != migratableJobs.end(); ++itJ) {
        notprocessed.push_back((**itJ)->IDFromEndpoint);
        jcJobMap[(**itJ)->jc].second.push_back(**itJ);
        jcJobMap[(**itJ)->jc].first.erase(*itJ);
      }
      return false;
    }

    for (std::list< std::list<Job*>::iterator >::iterator itJ = migratableJobs.begin();
         itJ != migratableJobs.end(); ++itJ) {
      std::list<JobDescription> jobdescs;
      if (!JobDescription::Parse((**itJ)->JobDescriptionDocument, jobdescs) || jobdescs.empty()) {
        logger.msg(ERROR, "Unable to migrate job (%s), unable to parse obtained job description", (**itJ)->IDFromEndpoint.fullstr());
        notprocessed.push_back((**itJ)->IDFromEndpoint);
        jcJobMap[(**itJ)->jc].second.push_back(**itJ);
        jcJobMap[(**itJ)->jc].first.erase(*itJ);
        continue;
      }
      jobdescs.front().Identification.ActivityOldID = (**itJ)->ActivityOldID;
      jobdescs.front().Identification.ActivityOldID.push_back((**itJ)->JobID.fullstr());

      // remove the queuename which was added during the original submission of the job
      jobdescs.front().Resources.QueueName = "";

      migratedJobs.push_back(Job());

      chosenBroker->PreFilterTargets(etr, jobdescs.front());
      chosenBroker->Sort();

      for (const ExecutionTarget*& t = chosenBroker->GetReference(); !chosenBroker->EndOfList(); chosenBroker->Advance()) {
        if (t->Migrate(usercfg, (**itJ)->IDFromEndpoint, jobdescs.front(), forcemigration, migratedJobs.back())) {
          chosenBroker->RegisterJobsubmission();
          break;
        }
      }

      if (chosenBroker->EndOfList()) {
        logger.msg(ERROR, "Job migration failed for job (%s), no applicable targets", (**itJ)->IDFromEndpoint.fullstr());
        ok = false;
        migratedJobs.pop_back();
        notprocessed.push_back((**itJ)->IDFromEndpoint);
        jcJobMap[(**itJ)->jc].second.push_back(**itJ);
        jcJobMap[(**itJ)->jc].first.erase(*itJ);
      }
      else {
        processed.push_back((**itJ)->IDFromEndpoint);
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

        if (!it->first->CancelJob(**itJ)) {
          ok = false;
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

        if (!it->first->CleanJob(**itJ)) {
          ok = false;
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

    return ok;
  }
} // namespace Arc
