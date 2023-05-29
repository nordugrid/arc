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
      JobControllerPlugin *jc = Job::getLoader().loadByInterfaceName(job.JobManagementInterfaceName, usercfg);
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

  void JobSupervisor::Select(const JobSelector& selector) {
    processed.clear();
    notprocessed.clear();

    for (JobSelectionMap::iterator it = jcJobMap.begin();
         it != jcJobMap.end(); ++it) {
      for (std::list<Job*>::iterator itJ = it->second.first.begin();
           itJ != it->second.first.end();) {
        if (!selector.Select(**itJ)) {
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
            if (Glib::file_test(downloaddir.Path(), Glib::FILE_TEST_IS_DIR)) {
              std::string cwd = URL(".").Path();
              cwd.resize(cwd.size()-1);
              if (downloaddir.Path().substr(0, cwd.size()) == cwd) {
                downloaddirectories.push_back(downloaddir.Path().substr(cwd.size()));
              } else {
                downloaddirectories.push_back(downloaddir.Path());
              }
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
