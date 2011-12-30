// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include <list>
#include <string>
#include <algorithm>

#include <arc/ArcLocation.h>
#include <arc/IString.h>
#include <arc/Logger.h>
#include <arc/StringConv.h>
#include <arc/client/JobController.h>
#include <arc/client/JobSupervisor.h>
#include <arc/UserConfig.h>

#include "utils.h"

#ifdef TEST
#define RUNSTAT(X) test_arcstat_##X
#else
#define RUNSTAT(X) X
#endif
int RUNSTAT(main)(int argc, char **argv) {

  setlocale(LC_ALL, "");

  Arc::Logger logger(Arc::Logger::getRootLogger(), "arcstat");
  Arc::LogStream logcerr(std::cerr);
  logcerr.setFormat(Arc::ShortFormat);
  Arc::Logger::getRootLogger().addDestination(logcerr);
  Arc::Logger::getRootLogger().setThreshold(Arc::WARNING);

  Arc::ArcLocation::Init(argv[0]);

  ClientOptions opt(ClientOptions::CO_STAT,
                    istring("[job ...]"),
                    istring("The arcstat command is used for "
                            "obtaining the status of jobs that have\n"
                            "been submitted to Grid enabled resources."));

  std::list<std::string> jobidentifiers = opt.Parse(argc, argv);

  if (opt.showversion) {
    std::cout << Arc::IString("%s version %s", "arcstat", VERSION)
              << std::endl;
    return 0;
  }

  // If debug is specified as argument, it should be set before loading the configuration.
  if (!opt.debug.empty())
    Arc::Logger::getRootLogger().setThreshold(Arc::string_to_level(opt.debug));

  if (opt.show_plugins) {
    std::list<std::string> types;
    types.push_back("HED:JobController");
    showplugins("arcstat", types, logger);
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

  if (!opt.sort.empty() && !opt.rsort.empty()) {
    logger.msg(Arc::ERROR, "The 'sort' and 'rsort' flags cannot be specified at the same time.");
    return 1;
  }

  if (!opt.rsort.empty()) {
    opt.sort = opt.rsort;
  }


  typedef bool (*JobSorting)(const Arc::Job*, const Arc::Job*);
  std::map<std::string, JobSorting> orderings;
  orderings["jobid"] = &Arc::Job::CompareJobID;
  orderings["submissiontime"] = &Arc::Job::CompareSubmissionTime;
  orderings["jobname"] = &Arc::Job::CompareJobName;

  if (!opt.sort.empty() && orderings.find(opt.sort) == orderings.end()) {
    std::cerr << "Jobs cannot be sorted by \"" << opt.sort << "\", the following orderings are supported:" << std::endl;
    for (std::map<std::string, JobSorting>::const_iterator it = orderings.begin();
         it != orderings.end(); it++)
      std::cerr << it->first << std::endl;
    return 1;
  }

  if ((!opt.joblist.empty() || !opt.status.empty()) && jobidentifiers.empty() && opt.clusters.empty())
    opt.all = true;

  if (jobidentifiers.empty() && opt.clusters.empty() && !opt.all) {
    logger.msg(Arc::ERROR, "No jobs given");
    return 1;
  }

  if (!jobidentifiers.empty() || opt.all)
    usercfg.ClearSelectedServices();

  if (!opt.clusters.empty()) {
    usercfg.ClearSelectedServices();
    usercfg.AddServices(opt.clusters, Arc::COMPUTING);
  }

  Arc::JobSupervisor jobmaster(usercfg, jobidentifiers);
  if (!jobmaster.JobsFound()) {
    std::cout << Arc::IString("No jobs") << std::endl;
    return 0;
  }
  std::list<Arc::JobController*> jobcont = jobmaster.GetJobControllers();

  if (jobcont.empty()) {
    logger.msg(Arc::ERROR, "No job controller plugins loaded");
    return 1;
  }

  std::vector<const Arc::Job*> jobs;
  for (std::list<Arc::JobController*>::iterator it = jobcont.begin();
       it != jobcont.end(); it++) {
    (*it)->FetchJobs(opt.status, jobs);
  }

  if (!opt.sort.empty()) {
    opt.rsort.empty() ? std::sort(jobs.begin(),  jobs.end(),  orderings[opt.sort]) :
                        std::sort(jobs.rbegin(), jobs.rend(), orderings[opt.sort]);
  }

  for (std::vector<const Arc::Job*>::const_iterator it = jobs.begin();
       it != jobs.end(); it++) {
    // Option 'long' (longlist) takes precedence over option 'print-jobids' (printids)
    if (opt.longlist || !opt.printids) {
      (*it)->SaveToStream(std::cout, opt.longlist);
    }
    else {
      std::cout << (*it)->JobID.fullstr() << std::endl;
    }
  }

  return 0;
}
