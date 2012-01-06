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

#ifdef TEST
#define RUNRENEW(X) test_arcrenew_##X
#else
#define RUNRENEW(X) X
#endif
int RUNRENEW(main)(int argc, char **argv) {

  setlocale(LC_ALL, "");

  Arc::Logger logger(Arc::Logger::getRootLogger(), "arcrenew");
  Arc::LogStream logcerr(std::cerr);
  logcerr.setFormat(Arc::ShortFormat);
  Arc::Logger::getRootLogger().addDestination(logcerr);
  Arc::Logger::getRootLogger().setThreshold(Arc::WARNING);

  Arc::ArcLocation::Init(argv[0]);

  ClientOptions opt(ClientOptions::CO_RENEW, istring("[job ...]"));

  std::list<std::string> jobidentifiers = opt.Parse(argc, argv);

  if (opt.showversion) {
    std::cout << Arc::IString("%s version %s", "arcrenew", VERSION)
              << std::endl;
    return 0;
  }

  // If debug is specified as argument, it should be set before loading the configuration.
  if (!opt.debug.empty())
    Arc::Logger::getRootLogger().setThreshold(Arc::string_to_level(opt.debug));

  if (opt.show_plugins) {
    std::list<std::string> types;
    types.push_back("HED:JobController");
    showplugins("arcrenew", types, logger);
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
  if (!jobmaster.JobsFound()) {
    std::cout << Arc::IString("No jobs") << std::endl;
    return 0;
  }

  std::list<Arc::URL> renewed, notrenewed;
  int retval = (int)!jobmaster.RenewByStatus(opt.status, renewed, notrenewed);

  std::cout << Arc::IString("Jobs processed: %d, renewed: %d", renewed.size()+notrenewed.size(), renewed.size()) << std::endl;

  return retval;
}
