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
#include <arc/client/JobController.h>
#include <arc/client/JobSupervisor.h>
#include <arc/client/ClientInterface.h>
#include <arc/UserConfig.h>

namespace Arc {

  Logger JobSupervisor::logger(Logger::getRootLogger(), "JobSupervisor");

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
