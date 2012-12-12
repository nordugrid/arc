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
#include <arc/compute/Broker.h>
#include <arc/compute/ComputingServiceRetriever.h>
#include <arc/compute/Submitter.h>
#include <arc/compute/SubmitterPlugin.h>

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
    if (job.JobID.empty()) {
      logger.msg(VERBOSE, "Ignoring job, the job ID is empty");
      return false;
    }

    if (job.JobManagementInterfaceName.empty()) {
      logger.msg(VERBOSE, "Ignoring job (%s), the management interface name is unknown", job.JobID);
      return false;
    }

    if (!job.JobManagementURL) {
      logger.msg(VERBOSE, "Ignoring job (%s), the job management URL is unknown", job.JobID);
      return false;
    }

    if (job.JobStatusInterfaceName.empty()) {
      logger.msg(VERBOSE, "Ignoring job (%s), the status interface name is unknown", job.JobID);
      return false;
    }

    if (!job.JobStatusURL) {
      logger.msg(VERBOSE, "Ignoring job (%s), the job status URL is unknown", job.JobID);
      return false;
    }

    std::map<std::string, JobControllerPlugin*>::iterator currentJC = loadedJCs.find(job.JobManagementInterfaceName);
    if (currentJC == loadedJCs.end()) {
      JobControllerPlugin *jc = Job::loader.loadByInterfaceName(job.JobManagementInterfaceName, usercfg);
      currentJC = loadedJCs.insert(std::pair<std::string, JobControllerPlugin*>(job.JobManagementInterfaceName, jc)).first;
      if (!jc) {
        logger.msg(VERBOSE, "Ignoring job (%s), unable to load JobControllerPlugin for %s", job.JobID, job.JobManagementInterfaceName);
        return false;
      }
      jcJobMap[jc] = std::pair< std::list<Job *>, std::list<Job*> >();
    }
    else if (!currentJC->second) {
      // Already tried to load JobControllerPlugin, and it failed.
      logger.msg(VERBOSE, "Ignoring job (%s), already tried and were unable to load JobControllerPlugin", job.JobID);
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

  void JobSupervisor::SelectByID(const std::list<std::string>& ids) {
    processed.clear();
    notprocessed.clear();

    if (ids.empty()) {
      return;
    }
    
    for (JobSelectionMap::iterator it = jcJobMap.begin();
         it != jcJobMap.end(); ++it) {
      for (std::list<Job*>::iterator itJ = it->second.first.begin();
           itJ != it->second.first.end();) {
        if (std::find(ids.begin(), ids.end(), (*itJ)->JobID) == ids.end()) {
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
          std::string path = URL((*itJ)->JobID).Path();
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
    if (!broker.isValid(false)) {
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

    Submitter s(resubmitUsercfg);
    for (std::list< std::list<Job*>::iterator >::iterator itJ = resubmittableJobs.begin();
         itJ != resubmittableJobs.end(); ++itJ) {
      resubmittedJobs.push_back(Job());

      std::list<JobDescription> jobdescs;
      if (!JobDescription::Parse((**itJ)->JobDescriptionDocument, jobdescs) || jobdescs.empty()) {
        std::cout << (**itJ)->JobDescriptionDocument << std::endl;
        logger.msg(ERROR, "Unable to resubmit job (%s), unable to parse obtained job description", (**itJ)->JobID);
        resubmittedJobs.pop_back();
        notprocessed.push_back((**itJ)->JobID);
        ok = false;
        jcJobMap[(**itJ)->jc].second.push_back(**itJ);
        jcJobMap[(**itJ)->jc].first.erase(*itJ);
        continue;
      }
      jobdescs.front().Identification.ActivityOldID = (**itJ)->ActivityOldID;
      jobdescs.front().Identification.ActivityOldID.push_back((**itJ)->JobID);

      // remove the queuename which was added during the original submission of the job
      jobdescs.front().Resources.QueueName = "";

      std::list<URL> rejectEndpoints;
      if (destination == 1) { // Jobs should be resubmitted to same target.
        std::list<Endpoint> sametarget(1, Endpoint((**itJ)->ServiceInformationURL.fullstr()));
        sametarget.front().Capability.insert(Endpoint::GetStringForCapability(Endpoint::COMPUTINGINFO));

        csr = new ComputingServiceRetriever(resubmitUsercfg, sametarget, rejectedURLs);
        csr->wait();
        if (csr->empty()) {
          logger.msg(ERROR, "Unable to resubmit job (%s), target information retrieval failed for target: %s", (**itJ)->JobID, (**itJ)->ServiceInformationURL.str());
          delete csr;
          resubmittedJobs.pop_back();
          notprocessed.push_back((**itJ)->JobID);
          jcJobMap[(**itJ)->jc].second.push_back(**itJ);
          jcJobMap[(**itJ)->jc].first.erase(*itJ);
          continue;
        }
      }
      else if (destination == 2) { // Jobs should NOT be resubmitted to same target.
        rejectEndpoints.push_back((**itJ)->ServiceInformationURL);
      }

      ExecutionTargetSorter ets(broker, jobdescs.front(), *csr, rejectEndpoints);
      for (ets.reset(); !ets.endOfList(); ets.next()) {
        if (s.Submit(*ets, jobdescs.front(), resubmittedJobs.back())) {
          ets.registerJobSubmission();
          processed.push_back((**itJ)->JobID);
          break;
        }
      }
      
      if (ets.endOfList()) {
        resubmittedJobs.pop_back();
        notprocessed.push_back((**itJ)->JobID);
        ok = false;
        logger.msg(ERROR, "Unable to resubmit job (%s), no targets applicable for submission", (**itJ)->JobID);
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
          logger.msg(ERROR, "Unable to migrate job (%s), job description could not be retrieved remotely", (*itJ)->JobID);
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
    if (!broker.isValid(false)) {
      logger.msg(ERROR, "Job migration aborted, unable to load broker (%s)", usercfg.Broker().first);
      for (std::list< std::list<Job*>::iterator >::const_iterator itJ = migratableJobs.begin();
           itJ != migratableJobs.end(); ++itJ) {
        notprocessed.push_back((**itJ)->JobID);
        jcJobMap[(**itJ)->jc].second.push_back(**itJ);
        jcJobMap[(**itJ)->jc].first.erase(*itJ);
      }
      return false;
    }

    SubmitterPluginLoader *spl = NULL;

    for (std::list< std::list<Job*>::iterator >::iterator itJ = migratableJobs.begin();
         itJ != migratableJobs.end(); ++itJ) {
      std::list<JobDescription> jobdescs;
      if (!JobDescription::Parse((**itJ)->JobDescriptionDocument, jobdescs) || jobdescs.empty()) {
        logger.msg(ERROR, "Unable to migrate job (%s), unable to parse obtained job description", (**itJ)->JobID);
        notprocessed.push_back((**itJ)->JobID);
        jcJobMap[(**itJ)->jc].second.push_back(**itJ);
        jcJobMap[(**itJ)->jc].first.erase(*itJ);
        continue;
      }
      jobdescs.front().Identification.ActivityOldID = (**itJ)->ActivityOldID;
      jobdescs.front().Identification.ActivityOldID.push_back((**itJ)->JobID);

      // remove the queuename which was added during the original submission of the job
      jobdescs.front().Resources.QueueName = "";

      migratedJobs.push_back(Job());

      ExecutionTargetSorter ets(broker, jobdescs.front(), csr);
      for (ets.reset(); !ets.endOfList(); ets.next()) {
        if (spl == NULL) {
          spl = new SubmitterPluginLoader();
        }
        SubmitterPlugin* sp = spl->loadByInterfaceName(ets->ComputingEndpoint->InterfaceName, usercfg);
        if (sp == NULL) {
          logger.msg(INFO, "Unable to load submission plugin for %s interface", ets->ComputingEndpoint->InterfaceName);
          continue;
        }
        if (sp->Migrate((**itJ)->JobID, jobdescs.front(), *ets, forcemigration, migratedJobs.back())) {
          ets.registerJobSubmission();
          break;
        }
      }

      if (ets.endOfList()) {
        logger.msg(ERROR, "Job migration failed for job (%s), no applicable targets", (**itJ)->JobID);
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

    if (spl) {
      delete spl;
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
