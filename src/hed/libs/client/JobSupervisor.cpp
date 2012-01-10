// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <algorithm>
#include <iostream>

#include <unistd.h>

#include <arc/CheckSum.h>
#include <arc/Logger.h>
#include <arc/client/Broker.h>
#include <arc/client/JobController.h>
#include <arc/client/TargetGenerator.h>
#include <arc/UserConfig.h>

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

    std::map<std::string, JobController*>::iterator currentJC = loadedJCs.find(job.Flavour);
    if (currentJC == loadedJCs.end()) {
      JobController *jc = loader.load(job.Flavour, usercfg);
      currentJC = loadedJCs.insert(std::pair<std::string, JobController*>(job.Flavour, jc)).first;
      if (!jc) {
        logger.msg(WARNING, "Unable to load JobController %s plugin. Is the %s plugin installed?", job.Flavour, job.Flavour);
        return false;
      }
    }
    else if (!currentJC->second) {
      // Already tried to load JobController, and it failed.
      return false;
    }

    currentJC->second->FillJobStore(job);
    return true;
  }

  std::list<Job> JobSupervisor::GetJobs(bool includeJobsWithoutStateInfo) const {
    std::list<Job> jobs;
    for (std::list<JobController*>::const_iterator itJC = loader.GetJobControllers().begin();
         itJC != loader.GetJobControllers().end(); ++itJC) {
      for (std::list<Job>::const_iterator itJ = (*itJC)->jobstore.begin();
           itJ != (*itJC)->jobstore.end(); ++itJ) {
        if (includeJobsWithoutStateInfo || (!includeJobsWithoutStateInfo && itJ->State)) {
          jobs.push_back(*itJ);
        }
      }
    }

    return jobs;
  }

  void JobSupervisor::Update() {
    for (std::list<JobController*>::const_iterator itJC = loader.GetJobControllers().begin();
         itJC != loader.GetJobControllers().end(); ++itJC) {
      (*itJC)->GetJobInformation();
    }
  }

  bool JobSupervisor::RetrieveByStatus(const std::list<std::string>& status,
                                       const std::string& downloaddirprefix,
                                       bool usejobname,
                                       bool force,
                                       std::list<URL>& retrieved,
                                       std::list<std::string>& downloaddirectories,
                                       std::list<URL>& notretrieved) {
    bool ok = true;

    std::list<JobController*> jobConts = loader.GetJobControllers();
    for (std::list<JobController*>::iterator itJobC = jobConts.begin();
         itJobC != jobConts.end(); itJobC++) {
      (*itJobC)->GetJobInformation();

      std::list<Job*> downloadable;
      for (std::list<Job>::iterator it = (*itJobC)->jobstore.begin();
           it != (*itJobC)->jobstore.end(); it++) {
        if (!it->State || (!status.empty() &&
            std::find(status.begin(), status.end(), it->State()) == status.end() &&
            std::find(status.begin(), status.end(), it->State.GetGeneralState()) == status.end())) {
          continue;
        }

        if (it->State == JobState::DELETED) {
          logger.msg(WARNING, "Unable to get job (%s), job is deleted", it->JobID.fullstr());
          continue;
        }
        else if (!it->State.IsFinished()) {
          logger.msg(WARNING, "Unable to get job (%s), it has not finished yet", it->JobID.fullstr());
          continue;
        }

        downloadable.push_back(&(*it));
      }

      for (std::list<Job*>::iterator it = downloadable.begin();
           it != downloadable.end(); it++) {
        std::string downloaddir = downloaddirprefix;
        if (!(*itJobC)->RetrieveJob(**it, downloaddir, usejobname, force)) {
          logger.msg(ERROR, "Failed getting job (%s)", (*it)->JobID.fullstr());
          ok = false;
          notretrieved.push_back((*it)->JobID);
        }
        else {
          retrieved.push_back((*it)->JobID);
          downloaddirectories.push_back(downloaddir);
        }
      }
    }

    return ok;
  }

  bool JobSupervisor::RenewByStatus(const std::list<std::string>& status, std::list<URL>& renewed, std::list<URL>& notrenewed) {
    bool ok = true;

    std::list<JobController*> jobConts = loader.GetJobControllers();
    for (std::list<JobController*>::iterator itJobC = jobConts.begin();
         itJobC != jobConts.end(); itJobC++) {
      (*itJobC)->GetJobInformation();

      std::list<Job*> renewable;
      for (std::list<Job>::iterator it = (*itJobC)->jobstore.begin();
           it != (*itJobC)->jobstore.end(); it++) {
        if (!it->State || (!status.empty() &&
            std::find(status.begin(), status.end(), it->State()) == status.end() &&
            std::find(status.begin(), status.end(), it->State.GetGeneralState()) == status.end())) {
          continue;
        }

        if (it->State == JobState::FINISHED || it->State == JobState::KILLED || it->State == JobState::DELETED) {
          logger.msg(WARNING, "Unable to renew job (%s), job already finished", it->JobID.fullstr());
          continue;
        }

        renewable.push_back(&(*it));
      }

      for (std::list<Job*>::iterator it = renewable.begin();
           it != renewable.end(); it++) {
        if (!(*itJobC)->RenewJob(**it)) {
          ok = false;
          notrenewed.push_back((*it)->JobID);
        }
        else {
          renewed.push_back((*it)->JobID);
        }
      }
    }

    return ok;
  }

  bool JobSupervisor::ResumeByStatus(const std::list<std::string>& status, std::list<URL>& resumed, std::list<URL>& notresumed) {
    bool ok = true;

    std::list<JobController*> jobConts = loader.GetJobControllers();
    for (std::list<JobController*>::iterator itJobC = jobConts.begin();
         itJobC != jobConts.end(); itJobC++) {
      (*itJobC)->GetJobInformation();

      std::list<Job*> resumable;
      for (std::list<Job>::iterator it = (*itJobC)->jobstore.begin();
           it != (*itJobC)->jobstore.end(); it++) {
        if (!it->State || (!status.empty() &&
            std::find(status.begin(), status.end(), it->State()) == status.end() &&
            std::find(status.begin(), status.end(), it->State.GetGeneralState()) == status.end())) {
          continue;
        }

        if (it->State == JobState::FINISHED || it->State == JobState::KILLED || it->State == JobState::DELETED) {
          logger.msg(WARNING, "Unable to resume job (%s), job is %s and cannot be resumed", it->JobID.fullstr(), it->State.GetGeneralState());
          continue;
        }

        resumable.push_back(&(*it));
      }

      for (std::list<Job*>::iterator it = resumable.begin();
           it != resumable.end(); it++) {
        if (!(*itJobC)->ResumeJob(**it)) {
          ok = false;
          notresumed.push_back((*it)->JobID);
        }
        else {
          resumed.push_back((*it)->JobID);
        }
      }
    }

    return ok;
  }

  bool JobSupervisor::Resubmit(const std::list<std::string>& status, int destination,
                               std::list<Job>& resubmittedJobs, std::list<URL>& notresubmitted) {
    bool ok = true;

    std::list< std::list<Job>::iterator > resubmittableJobs;
    std::list<JobController*> jobCs = loader.GetJobControllers();
    for (std::list<JobController*>::iterator itJobC = jobCs.begin();
         itJobC != jobCs.end(); ++itJobC) {
      (*itJobC)->GetJobInformation();

      for (std::list<Job>::iterator it = (*itJobC)->jobstore.begin();
           it != (*itJobC)->jobstore.end(); ++it) {
        if (!it->State) {
          continue;
        }

        if (!status.empty() &&
            std::find(status.begin(), status.end(), it->State()) == status.end() &&
            std::find(status.begin(), status.end(), it->State.GetGeneralState()) == status.end()) {
          continue;
        }

        // If job description is not set, then try to fetch it from execution service.
        if (it->JobDescriptionDocument.empty() && !(*itJobC)->GetJobDescription(*it, it->JobDescriptionDocument)) {
          logger.msg(ERROR, "Unable to resubmit job (%s), job description could not be retrieved remotely", it->IDFromEndpoint.fullstr());
          notresubmitted.push_back(it->IDFromEndpoint);
          ok = false;
          continue;
        }

        // Verify checksums of local input files
        if (!it->LocalInputFiles.empty()) {
          std::map<std::string, std::string>::iterator itF = it->LocalInputFiles.begin();
          for (; itF != it->LocalInputFiles.end(); ++itF) {
            if (itF->second != CheckSumAny::FileChecksum(itF->first, CheckSumAny::cksum, true)) {
              break;
            }
          }

          if (itF != it->LocalInputFiles.end()) {
            logger.msg(ERROR, "Unable to resubmit job (%s), local input file (%s) has changed", it->IDFromEndpoint.fullstr(), itF->first);
            notresubmitted.push_back(it->IDFromEndpoint);
            ok = false;
            continue;
          }
        }

        resubmittableJobs.push_back(it);
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
      for (std::list< std::list<Job>::iterator >::iterator it = resubmittableJobs.begin();
           it != resubmittableJobs.end(); it++) {
        notresubmitted.push_back((*it)->IDFromEndpoint);
      }
      return false;
    }

    TargetGenerator* tg = NULL;
    if (destination != 1) { // Jobs should not go to same target, making a general information gathering.
      tg = new TargetGenerator(resubmitUsercfg);
      tg->RetrieveExecutionTargets();
      if (tg->GetExecutionTargets().empty()) {
        logger.msg(ERROR, "Job resubmission aborted because no resource returned any information");
        delete tg;
        for (std::list< std::list<Job>::iterator >::iterator it = resubmittableJobs.begin();
             it != resubmittableJobs.end(); it++) {
          notresubmitted.push_back((*it)->IDFromEndpoint);
        }
        return false;
      }

    }

    for (std::list< std::list<Job>::iterator >::iterator it = resubmittableJobs.begin();
         it != resubmittableJobs.end(); ++it) {
      resubmittedJobs.push_back(Job());

      std::list<JobDescription> jobdescs;
      if (!JobDescription::Parse((*it)->JobDescriptionDocument, jobdescs) || jobdescs.empty()) {
        logger.msg(ERROR, "Unable to resubmit job (%s), unable to parse obtained job description", (*it)->IDFromEndpoint.fullstr());
        resubmittedJobs.pop_back();
        notresubmitted.push_back((*it)->IDFromEndpoint);
        ok = false;
        continue;
      }
      jobdescs.front().Identification.ActivityOldID = (*it)->ActivityOldID;
      jobdescs.front().Identification.ActivityOldID.push_back((*it)->JobID.fullstr());

      // remove the queuename which was added during the original submission of the job
      jobdescs.front().Resources.QueueName = "";

      std::list<URL> rejectTargets;
      if (destination == 1) { // Jobs should be resubmitted to same target.
        std::list<std::string> sametarget;
        sametarget.push_back((*it)->Flavour + ":" + (*it)->Cluster.fullstr());

        resubmitUsercfg.ClearSelectedServices();
        resubmitUsercfg.AddServices(sametarget, COMPUTING);

        tg = new TargetGenerator(resubmitUsercfg);
        tg->RetrieveExecutionTargets();
        if (tg->GetExecutionTargets().empty()) {
          logger.msg(ERROR, "Unable to resubmit job (%s), target information retrieval failed for target: %s", (*it)->IDFromEndpoint.fullstr(), (*it)->Cluster.str());
          delete tg;
          resubmittedJobs.pop_back();
          notresubmitted.push_back((*it)->IDFromEndpoint);
          continue;
        }
      }
      else if (destination == 2) { // Jobs should NOT be resubmitted to same target.
        rejectTargets.push_back((*it)->Cluster);
      }

      if (!chosenBroker->Submit(tg->GetExecutionTargets(), jobdescs.front(), resubmittedJobs.back(), rejectTargets)) {
        resubmittedJobs.pop_back();
        notresubmitted.push_back((*it)->IDFromEndpoint);
        ok = false;
        logger.msg(ERROR, "Unable to resubmit job (%s), no targets applicable for submission", (*it)->IDFromEndpoint.fullstr());
      }

      if (destination == 1) {
        delete tg;
      }
    }

    if (destination != 1) {
      delete tg;
    }

    return ok;
  }

  bool JobSupervisor::Migrate(bool forcemigration,
                              std::list<Job>& migratedJobs, std::list<URL>& notmigrated) {
    bool ok = true;

    std::list< std::list<Job>::iterator > migratableJobs;
    std::list<JobController*> jobConts = loader.GetJobControllers();
    for (std::list<JobController*>::iterator itJobC = jobConts.begin();
         itJobC != jobConts.end(); itJobC++) {
      (*itJobC)->GetJobInformation();

      for (std::list<Job>::iterator it = (*itJobC)->jobstore.begin(); it != (*itJobC)->jobstore.end(); ++it) {
        if (it->State != JobState::QUEUING) {
          continue;
        }

        // If job description is not set, then try to fetch it from execution service.
        if (it->JobDescriptionDocument.empty() && !(*itJobC)->GetJobDescription(*it, it->JobDescriptionDocument)) {
          logger.msg(ERROR, "Unable to migrate job (%s), job description could not be retrieved remotely", it->IDFromEndpoint.fullstr());
          notmigrated.push_back(it->IDFromEndpoint);
          ok = false;
          continue;
        }

        migratableJobs.push_back(it);
      }
    }

    if (migratableJobs.empty()) {
      return ok;
    }

    TargetGenerator targetGen(usercfg);
    targetGen.RetrieveExecutionTargets();
    if (targetGen.GetExecutionTargets().empty()) {
      logger.msg(ERROR, "Job migration aborted, no resource returned any information");
      for (std::list< std::list<Job>::iterator >::const_iterator it = migratableJobs.begin();
           it != migratableJobs.end(); ++it) {
        notmigrated.push_back((*it)->IDFromEndpoint);
      }
      return false;
    }

    BrokerLoader brokerLoader;
    Broker *chosenBroker = brokerLoader.load(usercfg.Broker().first, usercfg);
    if (!chosenBroker) {
      logger.msg(ERROR, "Job migration aborted, unable to load broker (%s)", usercfg.Broker().first);
      for (std::list< std::list<Job>::iterator >::const_iterator it = migratableJobs.begin();
           it != migratableJobs.end(); ++it) {
        notmigrated.push_back((*it)->IDFromEndpoint);
      }
      return false;
    }

    for (std::list< std::list<Job>::iterator >::iterator itJ = migratableJobs.begin();
         itJ != migratableJobs.end(); ++itJ) {
      std::list<JobDescription> jobdescs;
      if (!JobDescription::Parse((*itJ)->JobDescriptionDocument, jobdescs) || jobdescs.empty()) {
        logger.msg(ERROR, "Unable to migrate job (%s), unable to parse obtained job description", (*itJ)->IDFromEndpoint.fullstr());
        continue;
      }
      jobdescs.front().Identification.ActivityOldID = (*itJ)->ActivityOldID;
      jobdescs.front().Identification.ActivityOldID.push_back((*itJ)->JobID.fullstr());

      // remove the queuename which was added during the original submission of the job
      jobdescs.front().Resources.QueueName = "";

      migratedJobs.push_back(Job());

      chosenBroker->PreFilterTargets(targetGen.GetExecutionTargets(), jobdescs.front());
      chosenBroker->Sort();

      for (const ExecutionTarget*& t = chosenBroker->GetReference(); !chosenBroker->EndOfList(); chosenBroker->Advance()) {
        if (t->Migrate(usercfg, (*itJ)->IDFromEndpoint, jobdescs.front(), forcemigration, migratedJobs.back())) {
          chosenBroker->RegisterJobsubmission();
          break;
        }
      }

      if (chosenBroker->EndOfList()) {
        logger.msg(ERROR, "Job migration failed for job (%s), no applicable targets", (*itJ)->IDFromEndpoint.fullstr());
        ok = false;
        migratedJobs.pop_back();
        notmigrated.push_back((*itJ)->IDFromEndpoint);
      }
    }

    return ok;
  }

  bool JobSupervisor::CancelByIDs(const std::list<URL>& jobids, std::list<URL>& cancelled, std::list<URL>& notcancelled) {
    bool ok = true;

    std::list<JobController*> jobCs = loader.GetJobControllers();
    for (std::list<URL>::const_iterator itID = jobids.begin();
         itID != jobids.end(); ++itID) {
      for (std::list<JobController*>::iterator itJobC = jobCs.begin();
           itJobC != jobCs.end(); ++itJobC) {
        std::list<Job>::iterator itJ = (*itJobC)->jobstore.begin();
        for (; itJ != (*itJobC)->jobstore.end(); ++itJ) {
          if ((*itID) == itJ->IDFromEndpoint) {
            if (itJ->State && (itJ->State.IsFinished() || (*itJobC)->CancelJob(*itJ))) {
              cancelled.push_back(*itID);
            }
            else {
              if (itJ->State) { ok = false; }
              notcancelled.push_back(*itID);
            }
            break;
          }
        }
        if (itJ != (*itJobC)->jobstore.end()) {
          break;
        }
      }
    }

    return ok;
  }

  bool JobSupervisor::CancelByStatus(const std::list<std::string>& status, std::list<URL>& cancelled, std::list<URL>& notcancelled) {
    bool ok = true;

    std::list<JobController*> jobConts = loader.GetJobControllers();
    for (std::list<JobController*>::iterator itJobC = jobConts.begin();
         itJobC != jobConts.end(); itJobC++) {
      (*itJobC)->GetJobInformation();

      std::list<Job*> cancellable;
      for (std::list<Job>::iterator it = (*itJobC)->jobstore.begin();
           it != (*itJobC)->jobstore.end(); it++) {
        if (!it->State || (!status.empty() &&
            std::find(status.begin(), status.end(), it->State()) == status.end() &&
            std::find(status.begin(), status.end(), it->State.GetGeneralState()) == status.end())) {
          continue;
        }

        if (it->State == JobState::DELETED) {
          logger.msg(WARNING, "Unable to kill job (%s), job is deleted", it->JobID.fullstr());
          continue;
        }
        else if (it->State.IsFinished()) {
          logger.msg(WARNING, "Unable to kill job (%s), job has already finished", it->JobID.fullstr());
          continue;
        }

        cancellable.push_back(&(*it));
      }

      for (std::list<Job*>::iterator it = cancellable.begin();
           it != cancellable.end(); it++) {
        if (!(*itJobC)->CancelJob(**it)) {
          ok = false;
          notcancelled.push_back((*it)->JobID);
        }
        else {
          cancelled.push_back((*it)->JobID);
        }
      }
    }

    return ok;
  }

  bool JobSupervisor::CleanByIDs(const std::list<URL>& jobids, std::list<URL>& cleaned, std::list<URL>& notcleaned) {
    bool ok = true;

    std::list<JobController*> jobCs = loader.GetJobControllers();
    for (std::list<URL>::const_iterator itID = jobids.begin();
         itID != jobids.end(); ++itID) {
      for (std::list<JobController*>::iterator itJobC = jobCs.begin();
           itJobC != jobCs.end(); ++itJobC) {
        std::list<Job>::iterator itJ = (*itJobC)->jobstore.begin();
        for (; itJ != (*itJobC)->jobstore.end(); ++itJ) {
          if ((*itID) == itJ->IDFromEndpoint) {
            if (itJ->State && (*itJobC)->CleanJob(*itJ)) {
              cleaned.push_back(*itID);
            }
            else {
              if (itJ->State) { ok = false; }
              notcleaned.push_back(*itID);
            }
            break;
          }
        }
        if (itJ != (*itJobC)->jobstore.end()) {
          break;
        }
      }
    }

    return ok;
  }

  bool JobSupervisor::CleanByStatus(const std::list<std::string>& status, std::list<URL>& cleaned, std::list<URL>& notcleaned) {
    bool ok = true;
    std::list<JobController*> jobConts = loader.GetJobControllers();
    for (std::list<JobController*>::iterator itJobC = jobConts.begin();
         itJobC != jobConts.end(); itJobC++) {
      (*itJobC)->GetJobInformation();

      std::list<Job*> cleanable;
      for (std::list<Job>::iterator it = (*itJobC)->jobstore.begin();
           it != (*itJobC)->jobstore.end(); it++) {
        if (!it->State) {
          continue;
        }

        if (!status.empty() &&
            std::find(status.begin(), status.end(), it->State()) == status.end() &&
            std::find(status.begin(), status.end(), it->State.GetGeneralState()) == status.end()) {
          continue;
        }

        if (!it->State.IsFinished()) {
          logger.msg(WARNING, "Unable to clean job (%s), job has not finished yet", it->JobID.str());
          continue;
        }

        cleanable.push_back(&(*it));
      }

      for (std::list<Job*>::iterator it = cleanable.begin();
           it != cleanable.end(); it++) {
        if (!(*itJobC)->CleanJob(**it)) {
          ok = false;
          notcleaned.push_back((*it)->JobID);
        }
        else {
          cleaned.push_back((*it)->JobID);
        }
      }
    }

    return ok;
  }

  bool JobSupervisor::JobsFound() const {
    for (std::list<JobController*>::const_iterator it = loader.GetJobControllers().begin();
         it != loader.GetJobControllers().end(); ++it) {
      if (!(*it)->jobstore.empty()) {
        return true;
      }
    }

    return false;
  }
} // namespace Arc
