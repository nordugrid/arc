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
#include <arc/client/JobController.h>
#include <arc/client/JobSupervisor.h>
#include <arc/UserConfig.h>

#include "utils.h"

#ifdef TEST
#define RUNKILL(X) test_arckill_##X
#else
#define RUNKILL(X) X
#endif
int RUNKILL(main)(int argc, char **argv) {

  setlocale(LC_ALL, "");

  Arc::Logger logger(Arc::Logger::getRootLogger(), "arckill");
  Arc::LogStream logcerr(std::cerr);
  logcerr.setFormat(Arc::ShortFormat);
  Arc::Logger::getRootLogger().addDestination(logcerr);
  Arc::Logger::getRootLogger().setThreshold(Arc::WARNING);

  Arc::ArcLocation::Init(argv[0]);

  ClientOptions opt(ClientOptions::CO_KILL,
                    istring("[job ...]"),
                    istring("The arckill command is used to kill "
                            "running jobs."));

  std::list<std::string> jobs = opt.Parse(argc, argv);

  if (opt.showversion) {
    std::cout << Arc::IString("%s version %s", "arckill", VERSION)
              << std::endl;
    return 0;
  }

  // If debug is specified as argument, it should be set before loading the configuration.
  if (!opt.debug.empty())
    Arc::Logger::getRootLogger().setThreshold(Arc::string_to_level(opt.debug));

  if (opt.show_plugins) {
    std::list<std::string> types;
    types.push_back("HED:JobController");
    showplugins("arckill", types, logger);
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
    if (!Arc::Job::ReadJobIDsFromFile(*it, jobs)) {
      logger.msg(Arc::WARNING, "Cannot read specified jobid file: %s", *it);
    }
  }

  if (opt.timeout > 0)
    usercfg.Timeout(opt.timeout);

  if ((!opt.joblist.empty() || !opt.status.empty()) && jobs.empty() && opt.clusters.empty())
    opt.all = true;

  if (jobs.empty() && opt.clusters.empty() && !opt.all) {
    logger.msg(Arc::ERROR, "No jobs given");
    return 1;
  }

  if (!jobs.empty() || opt.all)
    usercfg.ClearSelectedServices();

  if (!opt.clusters.empty()) {
    usercfg.ClearSelectedServices();
    usercfg.AddServices(opt.clusters, Arc::COMPUTING);
  }

  // If the user specified a joblist on the command line joblist equals
  // usercfg.JobListFile(). If not use the default, ie. usercfg.JobListFile().
  Arc::JobSupervisor jobmaster(usercfg, jobs);
  if (!jobmaster.JobsFound()) {
    std::cout << Arc::IString("No jobs") << std::endl;
    return 0;
  }
  std::list<Arc::JobController*> jobcont = jobmaster.GetJobControllers();

  if (jobcont.empty()) {
    logger.msg(Arc::ERROR, "No job controller plugins loaded");
    return 1;
  }

  int retval = 0;
  for (std::list<Arc::JobController*>::iterator it = jobcont.begin();
       it != jobcont.end(); it++)
    if (!(*it)->Kill(opt.status, opt.keep))
      retval = 1;

  return retval;
}
