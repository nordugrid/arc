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
                               const std::list<std::string>& jobs,
                               const std::list<std::string>& clusters) {
    URLListMap jobids;
    URLListMap clusterselect;
    URLListMap clusterreject;

    if (!usercfg.ResolveAlias(clusters, clusterselect, clusterreject)) {
      logger.msg(ERROR, "Failed resolving aliases");
      return;
    }

    Config jobstorage;
    if (!usercfg.JobListFile().empty()) {
      FileLock lock(usercfg.JobListFile());
      jobstorage.ReadFromFile(usercfg.JobListFile());
    }

    std::list<std::string> controllers;

    if (!jobs.empty()) {

      logger.msg(DEBUG, "Identifying needed job controllers according to "
                 "specified jobs");

      for (std::list<std::string>::const_iterator it = jobs.begin();
           it != jobs.end(); it++) {

        XMLNodeList xmljobs =
          jobstorage.XPathLookup("//Job[JobID='" + *it + "' or "
                                 "Name='" + *it + "']", NS());

        if (xmljobs.empty()) {
          logger.msg(WARNING, "Job not found in job list: %s", *it);
          continue;
        }

        for (XMLNodeList::iterator xit = xmljobs.begin();
             xit != xmljobs.end(); xit++) {

          URL jobid = (std::string)(*xit)["JobID"];
          std::string flavour = (std::string)(*xit)["Flavour"];

          jobids[flavour].push_back(jobid);

          if (std::find(controllers.begin(), controllers.end(),
                        flavour) == controllers.end()) {
            logger.msg(DEBUG, "Need job controller for grid flavour %s",
                       flavour);
            controllers.push_back(flavour);
          }
        }
      }
    }

    if (!clusterselect.empty()) {

      logger.msg(DEBUG, "Identifying needed job controllers according to "
                 "specified clusters");

      for (URLListMap::iterator it = clusterselect.begin();
           it != clusterselect.end(); it++)
        if (std::find(controllers.begin(), controllers.end(),
                      it->first) == controllers.end()) {
          logger.msg(DEBUG, "Need job controller for grid flavour %s",
                     it->first);
          controllers.push_back(it->first);
        }
    }

    if (jobs.empty() && clusterselect.empty()) {

      logger.msg(DEBUG, "Identifying needed job controllers according to "
                 "all jobs present in job list");

      XMLNodeList xmljobs = jobstorage.XPathLookup("/ArcConfig/Job", NS());

      if (xmljobs.empty()) {
        logger.msg(INFO, "No jobs to handle");
        return;
      }

      for (XMLNodeList::iterator it = xmljobs.begin();
           it != xmljobs.end(); it++)
        if (std::find(controllers.begin(), controllers.end(),
                      (std::string)(*it)["Flavour"]) == controllers.end()) {
          std::string flavour = (*it)["Flavour"];
          logger.msg(DEBUG, "Need job controller for grid flavour %s",
                     flavour);
          controllers.push_back(flavour);
        }
    }

    for (std::list<std::string>::iterator it = controllers.begin();
         it != controllers.end(); it++) {
      JobController *JC = loader.load(*it, usercfg);
      if (JC)
        JC->FillJobStore(jobids[*it], clusterselect[*it], clusterreject[*it]);
    }
  }

  JobSupervisor::~JobSupervisor() {}

} // namespace Arc
