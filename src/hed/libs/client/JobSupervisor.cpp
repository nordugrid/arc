// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <algorithm>
#include <iostream>

#include <unistd.h>

#include <arc/ArcConfig.h>
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

  bool JobSupervisor::AddJob(const Job& job) {
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

  JobSupervisor::JobSupervisor(const UserConfig& usercfg,
                               const std::list<std::string>& jobs)
    : usercfg(usercfg) {
    std::map<std::string, std::list<URL> > jobmap;
    XMLNodeList xmljobs;

    Config jobstorage;
    if (!usercfg.JobListFile().empty()) {
      // lock job list file
      FileLock lock(usercfg.JobListFile());
      bool acquired = false;
      for (int tries = 10; tries > 0; --tries) {
        acquired = lock.acquire();
        if (acquired) {
          jobstorage.ReadFromFile(usercfg.JobListFile());
          lock.release();
          break;
        }
        logger.msg(VERBOSE, "Waiting for lock on job list file %s", usercfg.JobListFile());
        usleep(500000);
      }
      if (!acquired) {
        logger.msg(ERROR, "Failed to lock job list file %s", usercfg.JobListFile());
        return;
      }
    }

    if (!jobs.empty()) {
      for (std::list<std::string>::const_iterator it = jobs.begin();
           it != jobs.end(); it++) {

        xmljobs = jobstorage.XPathLookup("//Job[IDFromEndpoint='" +
                                         *it + "']", NS());
        if (xmljobs.empty())
          // Included for backwards compatibility.
          xmljobs = jobstorage.XPathLookup("//Job[JobID='" + *it + "']", NS());

        // Sanity check. A Job ID should be unique, thus the following
        // warning should never be shown.
        if (xmljobs.size() > 1)
          logger.msg(WARNING, "Job list (%s) contains %d jobs with identical "
                     "IDs, however only one will be processed.",
                     usercfg.JobListFile(), xmljobs.size());

        // No jobs in job list matched the string as job id, try job name.
        // Note: Multiple jobs can have identical names.
        if (xmljobs.empty())
          xmljobs = jobstorage.XPathLookup("//Job[Name='" + *it + "']", NS());

        if (xmljobs.empty()) {
          logger.msg(WARNING, "Job not found in job list: %s", *it);
          continue;
        }

        for (XMLNodeList::iterator xit = xmljobs.begin();
             xit != xmljobs.end(); xit++) {
          std::string jobid = ((*xit)["IDFromEndpoint"] ?
                               (std::string)(*xit)["IDFromEndpoint"] :
                               (std::string)(*xit)["JobID"]);
          jobmap[(std::string)(*xit)["Flavour"]].push_back(jobid);
        }
      }
    }

    if (jobs.empty() || !usercfg.GetSelectedServices(COMPUTING).empty()) {
      xmljobs = jobstorage.Path("Job");
      for (XMLNodeList::iterator xit = xmljobs.begin();
           xit != xmljobs.end(); xit++)
        if (jobmap.find((std::string)(*xit)["Flavour"]) == jobmap.end())
          jobmap[(std::string)(*xit)["Flavour"]];
    }

    for (std::map<std::string, std::list<URL> >::iterator it = jobmap.begin();
         it != jobmap.end(); it++) {
      JobController *JC = loader.load(it->first, usercfg);
      if (JC) {
        JC->FillJobStore(it->second);
      }
    }
  }

  JobSupervisor::JobSupervisor(const UserConfig& usercfg, const std::list<Job>& jobs)
    : usercfg(usercfg) {
    std::map<std::string, JobController*> loadedJCs;
    std::map<std::string, JobController*>::iterator currentJC;

    for (std::list<Job>::const_iterator it = jobs.begin();
         it != jobs.end(); ++it) {
      if (it->Flavour.empty()) {
        logger.msg(VERBOSE, "Ignoring job (%s), the Job::Flavour attribute must be specified", it->IDFromEndpoint.str());
        continue;
      }

      currentJC = loadedJCs.find(it->Flavour);
      if (currentJC == loadedJCs.end()) {
        JobController *jc = loader.load(it->Flavour, usercfg);
        currentJC = loadedJCs.insert(std::pair<std::string, JobController*>(it->Flavour, jc)).first;
        if (!jc) {
          logger.msg(WARNING, "Unable to load JobController %s plugin. Is the %s module installed?", it->Flavour, it->Flavour);
          continue;
        }
      }
      else if (!currentJC->second) {
        // Already tried to load JobController, and it failed.
        continue;
      }

      currentJC->second->FillJobStore(*it);
    }
  }

  JobSupervisor::~JobSupervisor() {}

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
            if (itF->second != Submitter::GetCksum(itF->first, usercfg)) {
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
