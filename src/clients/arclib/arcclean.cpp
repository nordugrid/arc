// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include <list>
#include <string>

#include <arc/ArcLocation.h>
#include <arc/IString.h>
#include <arc/Logger.h>
#include <arc/UserConfig.h>
#include <arc/client/JobSupervisor.h>

#include "utils.h"

int RUNMAIN(arcclean)(int argc, char **argv) {

  setlocale(LC_ALL, "");

  Arc::Logger logger(Arc::Logger::getRootLogger(), "arcclean");
  Arc::LogStream logcerr(std::cerr);
  logcerr.setFormat(Arc::ShortFormat);
  Arc::Logger::getRootLogger().addDestination(logcerr);
  Arc::Logger::getRootLogger().setThreshold(Arc::WARNING);

  Arc::ArcLocation::Init(argv[0]);

  ClientOptions opt(ClientOptions::CO_CLEAN,
                    istring("[job ...]"),
                    istring("The arcclean command removes a job "
                            "from the computing resource."));

  std::list<std::string> jobidentifiers = opt.Parse(argc, argv);

  if (opt.showversion) {
    std::cout << Arc::IString("%s version %s", "arcclean", VERSION)
              << std::endl;
    return 0;
  }

  // If debug is specified as argument, it should be set before loading the configuration.
  if (!opt.debug.empty())
    Arc::Logger::getRootLogger().setThreshold(Arc::string_to_level(opt.debug));

  if (opt.show_plugins) {
    std::list<std::string> types;
    types.push_back("HED:JobController");
    showplugins("arcclean", types, logger);
    return 0;
  }

  Arc::UserConfig usercfg(opt.conffile, opt.joblist);
  if (!usercfg) {
    logger.msg(Arc::ERROR, "Failed configuration initialization");
    return 1;
  }

  if (opt.debug.empty() && !usercfg.Verbosity().empty())
    Arc::Logger::getRootLogger().setThreshold(Arc::string_to_level(usercfg.Verbosity()));

  for (std::list<std::string>::const_iterator it = opt.jobidinfiles.begin(); it != opt.jobidinfiles.end(); it++) {
    if (!Arc::Job::ReadJobIDsFromFile(*it, jobidentifiers)) {
      logger.msg(Arc::WARNING, "Cannot read specified jobid file: %s", *it);
    }
  }

  if (opt.timeout > 0)
    usercfg.Timeout(opt.timeout);

  if ((!opt.joblist.empty() || !opt.status.empty()) && jobidentifiers.empty() && opt.clusters.empty())
    opt.all = true;

  if (jobidentifiers.empty() && opt.clusters.empty() && !opt.all) {
    logger.msg(Arc::ERROR, "No jobs given");
    return 1;
  }

  std::list<std::string> rejectClusters;
  splitendpoints(opt.clusters, rejectClusters);
  if (!usercfg.ResolveAliases(opt.clusters, Arc::COMPUTING) || !usercfg.ResolveAliases(rejectClusters, Arc::COMPUTING)) {
    return 1;
  }

  std::list<Arc::Job> jobs;
  if (!Arc::Job::ReadJobsFromFile(usercfg.JobListFile(), jobs, jobidentifiers, opt.all, opt.clusters, rejectClusters)) {
    logger.msg(Arc::ERROR, "Unable to read job information from file (%s)", usercfg.JobListFile());
    return 1;
  }

  for (std::list<std::string>::const_iterator itJIdentifier = jobidentifiers.begin();
       itJIdentifier != jobidentifiers.end(); ++itJIdentifier) {
    std::cout << Arc::IString("Warning: Job not found in job list: %s", *itJIdentifier) << std::endl;
  }

  Arc::JobSupervisor jobmaster(usercfg, jobs);
  jobmaster.Update();
  jobmaster.SelectValid();
  if (!opt.status.empty()) {
    jobmaster.SelectByStatus(opt.status);
  }

  if (jobmaster.GetSelectedJobs().empty()) {
    std::cout << Arc::IString("No jobs") << std::endl;
    return 1;
  }

  int retval = (int)!jobmaster.Clean();

  std::list<Arc::URL> cleaned = jobmaster.GetIDsProcessed();
  const std::list<Arc::URL>& notcleaned = jobmaster.GetIDsNotProcessed();

  if ((!opt.status.empty() && std::find(opt.status.begin(), opt.status.end(), "Undefined") != opt.status.end()) || opt.forceclean) {
    std::string response = "";
    if (!opt.forceclean) {
      std::cout << Arc::IString("You are about to remove jobs from the job list for which no information could be\n"
                                "found. NOTE: Recently submitted jobs might not have appeared in the information\n"
                                "system, and this action will also remove such jobs.") << std::endl;
      std::cout << Arc::IString("Are you sure you want to clean jobs missing information?") << " ["
                << Arc::IString("y") << "/" << Arc::IString("n") << "] ";
      std::cin >> response;
    }

    if (!opt.forceclean && Arc::lower(response) != std::string(Arc::FindTrans("y"))) {
      std::cout << Arc::IString("Jobs missing information will not be cleaned!") << std::endl;
      if (cleaned.empty() && notcleaned.empty()) {
        return retval;
      }
    }
    else {
      for (std::list<Arc::Job>::const_iterator it = jobmaster.GetAllJobs().begin(); it != jobmaster.GetAllJobs().end(); ++it) {
        if (it->State == Arc::JobState::UNDEFINED) {
          cleaned.push_back(it->JobID);
        }
      }
    }
  }

  if (!Arc::Job::RemoveJobsFromFile(usercfg.JobListFile(), cleaned)) {
    std::cout << Arc::IString("Warning: Failed to lock job list file %s", usercfg.JobListFile()) << std::endl;
    std::cout << Arc::IString("         Run 'arcclean -s Undefined' to remove cleaned jobs from job list", usercfg.JobListFile()) << std::endl;
  }

  std::cout << Arc::IString("Jobs processed: %d, deleted: %d", cleaned.size()+notcleaned.size(), cleaned.size()) << std::endl;

  return retval;
}
