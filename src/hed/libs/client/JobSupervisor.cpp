// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <algorithm>
#include <iostream>

#include <unistd.h>

#include <arc/ArcConfig.h>
#include <arc/CheckSum.h>
#include <arc/FileLock.h>
#include <arc/Logger.h>
#include <arc/StringConv.h>
#include <arc/XMLNode.h>
#include <arc/client/Broker.h>
#include <arc/client/JobController.h>
#include <arc/client/JobSupervisor.h>
#include <arc/client/ClientInterface.h>
#include <arc/UserConfig.h>

namespace Arc {

  Logger JobSupervisor::logger(Logger::getRootLogger(), "JobSupervisor");

  JobSupervisor::JobSupervisor(const UserConfig& usercfg,
                               const std::list<std::string>& jobIDsAndNames)
    : usercfg(usercfg) {
    if (usercfg.JobListFile().empty()) {
      logger.msg(WARNING, "Job list file not specified.");
      return;
    }

    std::list<Job> jobs;
    std::list<Job> alreadyFoundJobs;
    Job::ReadAllJobsFromFile(usercfg.JobListFile(), jobs);

    // Add jobs explicitly specified.
    for (std::list<std::string>::const_iterator it = jobIDsAndNames.begin();
         it != jobIDsAndNames.end(); ++it) {
      std::list<Job>::iterator itJ = jobs.begin();
      for (; itJ != jobs.end(); ++itJ) {
        if (*it == itJ->IDFromEndpoint.str() || *it == itJ->Name) {
          // If the job wasn't already selected in a previous cycle...
          bool alreadySelectedJob = false;
          for (std::list<Job>::iterator itAJ = alreadyFoundJobs.begin(); itAJ != alreadyFoundJobs.end(); itAJ++) {
            if (*itAJ == *itJ) {
              alreadySelectedJob = true;
            }
          }
          // Add it!
          if (!alreadySelectedJob && AddJob(*itJ)) {
            // Job was added to JobController, remove it from list.
            // jobs.erase(itJ);
            alreadyFoundJobs.push_back(*itJ);
          } else {
            logger.msg(WARNING, "Unable to handle job (%s), no suitable JobController plugin found.", *it);
          }
        }
      }

      if (alreadyFoundJobs.empty()) {
        logger.msg(WARNING, "Job not found in job list: %s", *it);
      }
    }

    // Exclude jobs on rejected services.
    if (!usercfg.GetRejectedServices(COMPUTING).empty() && !jobs.empty()) {
      for (std::list<std::string>::const_iterator itC = usercfg.GetRejectedServices(COMPUTING).begin();
           itC != usercfg.GetRejectedServices(COMPUTING).end(); ++itC) {
        std::size_t pos = itC->find(":");
        std::string cFlavour = itC->substr(0, pos), service = itC->substr(pos+1);
        logger.msg(DEBUG, "cFlavour = %s; service = %s", cFlavour, service);
        for (std::list<Job>::iterator itJ = jobs.begin(); itJ != jobs.end();) {
          if ((cFlavour == "*" || cFlavour == itJ->Flavour) &&
              (itJ->Cluster.StringMatches(service) || itJ->InfoEndpoint.StringMatches(service))) {
            itJ = jobs.erase(itJ);
          }
          else {
            ++itJ;
          }
        }
      }
    }

    // Add jobs on selected services.
    if (!usercfg.GetSelectedServices(COMPUTING).empty() && !jobs.empty()) {
      for (std::list<std::string>::const_iterator itC = usercfg.GetSelectedServices(COMPUTING).begin();
           itC != usercfg.GetSelectedServices(COMPUTING).end(); ++itC) {
        std::size_t pos = itC->find(":");
        std::string cFlavour = itC->substr(0, pos), service = itC->substr(pos+1);
        logger.msg(DEBUG, "cFlavour = %s; service = %s", cFlavour, service);
        for (std::list<Job>::iterator itJ = jobs.begin(); itJ != jobs.end();) {
          if ((cFlavour == "*" || cFlavour == itJ->Flavour) &&
              (itJ->Cluster.StringMatches(service) || itJ->InfoEndpoint.StringMatches(service))) {
            if (AddJob(*itJ)) {
              // Job was added to JobController, remove it from list.
              itJ = jobs.erase(itJ);
              continue;
            }
            else {
              logger.msg(WARNING, "Unable to handle job (%s), no suitable JobController plugin found.", itJ->IDFromEndpoint.str());
            }
          }
          ++itJ;
        }
      }
    }

    // If neither jobs was specified explicitly or services was selected, all jobs should be added (except those rejected).
    if (jobIDsAndNames.empty() && usercfg.GetSelectedServices(COMPUTING).empty() && !jobs.empty()) {
      for (std::list<Job>::const_iterator itJ = jobs.begin();
           itJ != jobs.end(); ++itJ) {
        if (!AddJob(*itJ)) {
          logger.msg(WARNING, "Unable to handle job (%s), no suitable job management plugin found.", itJ->IDFromEndpoint.str());
        }
      }
    }
  }

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
      logger.msg(VERBOSE, "Ignoring job (%s), the Job::Flavour attribute must be specified", job.IDFromEndpoint.str());
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

  bool JobSupervisor::Get(const std::list<std::string>& status,
                          const std::string& downloaddir,
                          bool usejobname,
                          bool force,
                          std::list<URL>& retrievedJobs) {
    bool ok = true;

    std::list<JobController*> jobConts = loader.GetJobControllers();
    for (std::list<JobController*>::iterator itJobC = jobConts.begin();
         itJobC != jobConts.end(); itJobC++) {
      (*itJobC)->GetJobInformation();

      std::list<Job*> downloadable;
      for (std::list<Job>::iterator it = (*itJobC)->jobstore.begin();
           it != (*itJobC)->jobstore.end(); it++) {

        if (!it->State) {
          logger.msg(WARNING, "Unable to get job (%s), job information not found at execution service", it->JobID.str());
          continue;
        }

        if (!status.empty() &&
            std::find(status.begin(), status.end(), it->State()) == status.end() &&
            std::find(status.begin(), status.end(), it->State.GetGeneralState()) == status.end()) {
          continue;
        }

        if (it->State == JobState::DELETED) {
          logger.msg(WARNING, "Unable to get job (%s), job is deleted", it->JobID.str());
          continue;
        }
        else if (!it->State.IsFinished()) {
          logger.msg(WARNING, "Unable to get job (%s), it has not finished yet", it->JobID.str());
          continue;
        }

        downloadable.push_back(&(*it));
      }

      for (std::list<Job*>::iterator it = downloadable.begin();
           it != downloadable.end(); it++) {
        if (!(*itJobC)->GetJob(**it, downloaddir, usejobname, force)) {
          logger.msg(ERROR, "Failed getting job (%s)", (*it)->JobID.str());
          ok = false;
        }
        else {
          retrievedJobs.push_back((*it)->JobID);
        }
      }
    }

    return ok;
  }

  bool JobSupervisor::Kill(const std::list<std::string>& status,
                           std::list<URL>& killedJobs) {
    bool ok = true;

    std::list<JobController*> jobConts = loader.GetJobControllers();
    for (std::list<JobController*>::iterator itJobC = jobConts.begin();
         itJobC != jobConts.end(); itJobC++) {
      (*itJobC)->GetJobInformation();

      std::list<Job*> killable;
      for (std::list<Job>::iterator it = (*itJobC)->jobstore.begin();
           it != (*itJobC)->jobstore.end(); it++) {

        if (!it->State) {
          logger.msg(WARNING, "Unable to kill job (%s), job information not found at execution service", it->JobID.str());
          continue;
        }

        if (!status.empty() &&
            std::find(status.begin(), status.end(), it->State()) == status.end() &&
            std::find(status.begin(), status.end(), it->State.GetGeneralState()) == status.end()) {
          continue;
        }

        if (it->State == JobState::DELETED) {
          logger.msg(WARNING, "Unable to kill job (%s), job is deleted", it->JobID.str());
          continue;
        }
        else if (it->State.IsFinished()) {
          logger.msg(WARNING, "Unable to kill job (%s), job has already finished", it->JobID.str());
          continue;
        }

        killable.push_back(&(*it));
      }

      for (std::list<Job*>::iterator it = killable.begin();
           it != killable.end(); it++) {
        if (!(*itJobC)->CancelJob(**it)) {
          logger.msg(ERROR, "Failed killing job (%s)", (*it)->JobID.str());
          ok = false;
        }
        else {
          killedJobs.push_back((*it)->JobID);
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
          logger.msg(ERROR, "Unable to resubmit job (%s), job description could not be retrieved remotely", it->IDFromEndpoint.str());
          notresubmitted.push_back(it->IDFromEndpoint);
          ok = false;
          continue;
        }

        // Verify checksums of local input files
        if (!it->LocalInputFiles.empty()) {
          std::map<std::string, std::string>::iterator itF = it->LocalInputFiles.begin();
          for (; itF != it->LocalInputFiles.end(); ++itF) {
            if (itF->second != CheckSumAny::FileChecksum(itF->first, CheckSumAny::md5)) {
              break;
            }
          }

          if (itF != it->LocalInputFiles.end()) {
            logger.msg(ERROR, "Unable to resubmit job (%s), local input file (%s) has changed", it->IDFromEndpoint.str(), itF->first);
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
        logger.msg(ERROR, "Unable to resubmit job (%s), unable to parse obtained job description", (*it)->IDFromEndpoint.str());
        resubmittedJobs.pop_back();
        notresubmitted.push_back((*it)->IDFromEndpoint);
        ok = false;
        continue;
      }
      jobdescs.front().Identification.ActivityOldId = (*it)->ActivityOldID;
      jobdescs.front().Identification.ActivityOldId.push_back((*it)->JobID.str());

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
          logger.msg(ERROR, "Unable to resubmit job (%s), target information retrieval failed for target: %s", (*it)->IDFromEndpoint.str(), (*it)->Cluster.str());
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
        logger.msg(ERROR, "Unable to resubmit job (%s), no targets applicable for submission", (*it)->IDFromEndpoint.str());
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
          logger.msg(ERROR, "Unable to migrate job (%s), job description could not be retrieved remotely", it->IDFromEndpoint.str());
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
        logger.msg(ERROR, "Unable to migrate job (%s), unable to parse obtained job description", (*itJ)->IDFromEndpoint.str());
        continue;
      }
      jobdescs.front().Identification.ActivityOldId = (*itJ)->ActivityOldID;
      jobdescs.front().Identification.ActivityOldId.push_back((*itJ)->JobID.str());

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
        logger.msg(ERROR, "Job migration failed for job (%s), no applicable targets", (*itJ)->IDFromEndpoint.str());
        ok = false;
        migratedJobs.pop_back();
        notmigrated.push_back((*itJ)->IDFromEndpoint);
      }
    }

    return ok;
  }

  std::list<URL> JobSupervisor::Cancel(const std::list<URL>& jobids,
                                       std::list<URL>& notcancelled) {
    std::list<URL> cancelled;

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

    return cancelled;
  }

  std::list<URL> JobSupervisor::Clean(const std::list<URL>& jobids,
                                      std::list<URL>& notcleaned) {
    std::list<URL> cleaned;

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

    return cleaned;
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
