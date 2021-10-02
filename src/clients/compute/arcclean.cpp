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
#include <arc/StringConv.h>
#include <arc/UserConfig.h>
#include <arc/compute/JobInformationStorage.h>
#include <arc/compute/JobSupervisor.h>

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
    Arc::Logger::getRootLogger().setThreshold(Arc::istring_to_level(opt.debug));

  logger.msg(Arc::VERBOSE, "Running command: %s", opt.GetCommandWithArguments());

  if (opt.show_plugins) {
    std::list<std::string> types;
    types.push_back("HED:JobControllerPlugin");
    showplugins("arcclean", types, logger);
    return 0;
  }

  Arc::UserConfig usercfg(opt.conffile, opt.joblist);
  if (!usercfg) {
    logger.msg(Arc::ERROR, "Failed configuration initialization");
    return 1;
  }

  if(usercfg.OToken().empty()) {
    if (!checkproxy(usercfg)) {
      return 1;
    }
  }

  if (opt.debug.empty() && !usercfg.Verbosity().empty())
    Arc::Logger::getRootLogger().setThreshold(Arc::istring_to_level(usercfg.Verbosity()));

  for (std::list<std::string>::const_iterator it = opt.jobidinfiles.begin(); it != opt.jobidinfiles.end(); ++it) {
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

  std::list<std::string> selectedURLs;
  if (!opt.clusters.empty()) {
    selectedURLs = getSelectedURLsFromUserConfigAndCommandLine(usercfg, opt.clusters);
  }
  std::list<std::string> rejectManagementURLs = getRejectManagementURLsFromUserConfigAndCommandLine(usercfg, opt.rejectmanagement);

  std::list<Arc::Job> jobs;
  Arc::JobInformationStorage *jobstore = createJobInformationStorage(usercfg);
  if (jobstore != NULL && !jobstore->IsStorageExisting()) {
    logger.msg(Arc::ERROR, "Job list file (%s) doesn't exist", usercfg.JobListFile());
    delete jobstore;
    return 1;
  }
  if (jobstore == NULL ||
      ( opt.all && !jobstore->ReadAll(jobs, rejectManagementURLs)) ||
      (!opt.all && !jobstore->Read(jobs, jobidentifiers, selectedURLs, rejectManagementURLs))) {
    logger.msg(Arc::ERROR, "Unable to read job information from file (%s)", usercfg.JobListFile());
    delete jobstore;
    return 1;
  }

  if (!opt.all) {
    for (std::list<std::string>::const_iterator itJIDAndName = jobidentifiers.begin();
         itJIDAndName != jobidentifiers.end(); ++itJIDAndName) {
      std::cout << Arc::IString("Warning: Job not found in job list: %s", *itJIDAndName) << std::endl;
    }
  }

  Arc::JobSupervisor jobmaster(usercfg, jobs);
  jobmaster.Update();
  jobmaster.SelectValid();
  if (!opt.status.empty()) {
    jobmaster.SelectByStatus(opt.status);
  }

  //if (jobmaster.GetSelectedJobs().empty()) {
  //  std::cout << Arc::IString("No jobs") << std::endl;
  //  return 1;
  //}

  int retval = (int)!jobmaster.Clean();

  std::list<std::string> cleaned = jobmaster.GetIDsProcessed();
  const std::list<std::string>& notcleaned = jobmaster.GetIDsNotProcessed();

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


  if (!jobstore->Remove(cleaned)) {
    std::cout << Arc::IString("Warning: Failed to write job information to file (%s)", usercfg.JobListFile()) << std::endl;
    std::cout << Arc::IString("         Run 'arcclean -s Undefined' to remove cleaned jobs from job list", usercfg.JobListFile()) << std::endl;
  }
  delete jobstore;

  if (cleaned.empty() && notcleaned.empty()) {
    std::cout << Arc::IString("No jobs") << std::endl;
    return 1;
  }

  std::cout << Arc::IString("Jobs processed: %d, deleted: %d", cleaned.size()+notcleaned.size(), cleaned.size()) << std::endl;

  return retval;
}
