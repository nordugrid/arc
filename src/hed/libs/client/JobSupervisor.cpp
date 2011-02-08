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
    : jobsFound(false) {
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
        if (!JC->GetJobs().empty())
          jobsFound = true;
      }
    }
  }

  JobSupervisor::~JobSupervisor() {}

} // namespace Arc
