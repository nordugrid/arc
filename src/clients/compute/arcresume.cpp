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
#include <arc/compute/JobInformationStorage.h>
#include <arc/compute/JobSupervisor.h>

#include "utils.h"

int RUNMAIN(arcresume)(int argc, char **argv) {

  setlocale(LC_ALL, "");

  Arc::Logger logger(Arc::Logger::getRootLogger(), "arcresume");
  Arc::LogStream logcerr(std::cerr);
  logcerr.setFormat(Arc::ShortFormat);
  Arc::Logger::getRootLogger().addDestination(logcerr);
  Arc::Logger::getRootLogger().setThreshold(Arc::WARNING);

  Arc::ArcLocation::Init(argv[0]);

  ClientOptions opt(ClientOptions::CO_RESUME, istring("[job ...]"));

  std::list<std::string> jobidentifiers = opt.Parse(argc, argv);

  if (opt.showversion) {
    std::cout << Arc::IString("%s version %s", "arcresume", VERSION)
              << std::endl;
    return 0;
  }

  // If debug is specified as argument, it should be set before loading the configuration.
  if (!opt.debug.empty())
    Arc::Logger::getRootLogger().setThreshold(Arc::istring_to_level(opt.debug));

  logger.msg(Arc::VERBOSE, "Running command: %s", opt.GetCommandWithArguments());

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

  if (opt.show_plugins) {
    std::list<std::string> types;
    types.push_back("HED:JobControllerPlugin");
    showplugins("arcresume", types, logger);
    return 0;
  }

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
  std::list<std::string> rejectManagementURLs = getRejectDiscoveryURLsFromUserConfigAndCommandLine(usercfg, opt.rejectdiscovery);

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
  delete jobstore;

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

  if (jobmaster.GetSelectedJobs().empty()) {
    std::cout << Arc::IString("No jobs") << std::endl;
    return 1;
  }

  int retval = (int)!jobmaster.Resume();

  std::cout << Arc::IString("Jobs processed: %d, resumed: %d", jobmaster.GetIDsProcessed().size()+jobmaster.GetIDsNotProcessed().size(), jobmaster.GetIDsProcessed().size()) << std::endl;

  return retval;
}
