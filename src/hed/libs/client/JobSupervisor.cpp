// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <algorithm>
#include <iostream>

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
    /* Order selected jobs by flavour and then ID. A map is used so a job is only added once.
     jobmap[Flavour][IDFromEndpoint] = Job;
     */
    std::map<std::string, std::map<std::string, Job> > jobmap;
    XMLNodeList xmljobs;

    Config jobstorage;
    if (!usercfg.JobListFile().empty()) {
      FileLock lock(usercfg.JobListFile());
      jobstorage.ReadFromFile(usercfg.JobListFile());
    }

    if (!jobs.empty()) {
      for (std::list<std::string>::const_iterator it = jobs.begin();
           it != jobs.end(); it++) {
        std::string strJobID = *it;
        // Remove trailing slashes '/' from input (jobs).
        const std::string::size_type pos = strJobID.find_last_not_of("/");
        if (pos != std::string::npos) {
          strJobID = strJobID.substr(0, pos + 1);
        }

        xmljobs = jobstorage.XPathLookup("//Job[IDFromEndpoint='" + strJobID + "']", NS());
        if (xmljobs.empty()) { // Included for backwards compatibility.
          xmljobs = jobstorage.XPathLookup("//Job[JobID='" + strJobID + "']", NS());
        }

        // Sanity check. A Job ID should be unique, thus the following warning should never be shown.
        if (xmljobs.size() > 1) {
          logger.msg(WARNING, "Job list (%s) contains %d jobs with identical IDs, however only one will be processed.", usercfg.JobListFile(), xmljobs.size());
        }

        // No jobs in job list matched the string as job id, try job name. Note: Multiple jobs can have identical names.
        if (xmljobs.empty()) {
          xmljobs = jobstorage.XPathLookup("//Job[Name='" + *it + "']", NS());
        }

        if (xmljobs.empty()) {
          logger.msg(WARNING, "Job not found in job list: %s", *it);
          continue;
        }

        for (XMLNodeList::iterator xit = xmljobs.begin();
             xit != xmljobs.end(); xit++) {
          std::string jobid = ((*xit)["IDFromEndpoint"] ? (std::string)(*xit)["IDFromEndpoint"] : (std::string)(*xit)["JobID"]);
          jobmap[(std::string)(*xit)["Flavour"]][jobid] = *xit;
        }
      }
    }

    xmljobs.clear();
    if (!usercfg.GetSelectedServices(COMPUTING).empty()) {
      for (URLListMap::const_iterator it = usercfg.GetSelectedServices(COMPUTING).begin();
           it != usercfg.GetSelectedServices(COMPUTING).end(); it++) {
        for (std::list<URL>::const_iterator itCluster = it->second.begin(); itCluster != it->second.end(); itCluster++) {
          xmljobs = jobstorage.XPathLookup("//Job[Cluster='" + itCluster->str() + "']", NS());
          for (XMLNodeList::iterator xit = xmljobs.begin();
               xit != xmljobs.end(); xit++) {
            std::string jobid = ((*xit)["IDFromEndpoint"] ? (std::string)(*xit)["IDFromEndpoint"] : (std::string)(*xit)["JobID"]);
            jobmap[it->first][jobid] = *xit;
          }
        }
      }
    }

    xmljobs.clear();
    if (jobs.empty() && usercfg.GetSelectedServices(COMPUTING).empty()) {
      xmljobs = jobstorage.Path("Job");
      for (XMLNodeList::iterator xit = xmljobs.begin();
           xit != xmljobs.end(); xit++) {
        std::string jobid = ((*xit)["IDFromEndpoint"] ? (std::string)(*xit)["IDFromEndpoint"] : (std::string)(*xit)["JobID"]);
        jobmap[(std::string)(*xit)["Flavour"]][jobid] = *xit;
      }
    }

    jobsFound = (jobmap.size() > 0);

    for (std::map<std::string, std::map<std::string, Job> >::iterator it = jobmap.begin();
         it != jobmap.end(); it++) {
      JobController *JC = loader.load(it->first, usercfg);
      if (JC) {
        for (std::map<std::string, Job>::iterator itJobs = it->second.begin();
             itJobs != it->second.end(); itJobs++) {
          JC->FillJobStore(itJobs->second);
        }
      }
    }
  }

  JobSupervisor::~JobSupervisor() {}

} // namespace Arc
