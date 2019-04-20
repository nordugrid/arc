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
#include <arc/UserConfig.h>
#include <arc/compute/JobInformationStorage.h>
#include <arc/compute/JobSupervisor.h>

#include "utils.h"

int RUNMAIN(arcstat)(int argc, char **argv) {

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
    Arc::Logger::getRootLogger().setThreshold(Arc::istring_to_level(opt.debug));

  logger.msg(Arc::VERBOSE, "Running command: %s", opt.GetCommandWithArguments());

  if (opt.show_plugins) {
    std::list<std::string> types;
    types.push_back("HED:JobControllerPlugin");
    showplugins("arcstat", types, logger);
    return 0;
  }

  Arc::UserConfig usercfg(opt.conffile, opt.joblist);
  if (!usercfg) {
    logger.msg(Arc::ERROR, "Failed configuration initialization");
    return 1;
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

  if (!opt.sort.empty() && !opt.rsort.empty()) {
    logger.msg(Arc::ERROR, "The 'sort' and 'rsort' flags cannot be specified at the same time.");
    return 1;
  }

  if (!opt.rsort.empty()) {
    opt.sort = opt.rsort;
  }

  typedef bool (*JobSorting)(const Arc::Job&, const Arc::Job&);
  std::map<std::string, JobSorting> orderings;
  orderings["jobid"] = &Arc::Job::CompareJobID;
  orderings["submissiontime"] = &Arc::Job::CompareSubmissionTime;
  orderings["jobname"] = &Arc::Job::CompareJobName;

  if (!opt.sort.empty() && orderings.find(opt.sort) == orderings.end()) {
    std::cerr << "Jobs cannot be sorted by \"" << opt.sort << "\", the following orderings are supported:" << std::endl;
    for (std::map<std::string, JobSorting>::const_iterator it = orderings.begin();
         it != orderings.end(); ++it)
      std::cerr << it->first << std::endl;
    return 1;
  }

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
  delete jobstore;

  if (!opt.all) {
    for (std::list<std::string>::const_iterator itJIDAndName = jobidentifiers.begin();
         itJIDAndName != jobidentifiers.end(); ++itJIDAndName) {
      std::cout << Arc::IString("Warning: Job not found in job list: %s", *itJIDAndName) << std::endl;
    }
  }

  Arc::JobSupervisor jobmaster(usercfg, jobs);
  jobmaster.Update();
  unsigned int queried_num = jobmaster.GetAllJobs().size();
  if (!opt.status.empty()) {
    jobmaster.SelectByStatus(opt.status);
  }
  if (!opt.show_unavailable) {
    jobmaster.SelectValid();
  }
  jobs = jobmaster.GetSelectedJobs();

  if (queried_num == 0) {
    std::cout << Arc::IString("No jobs found, try later") << std::endl;
    return 1;
  }

  std::vector<Arc::Job> jobsSortable(jobs.begin(), jobs.end());

  if (!opt.sort.empty()) {
    opt.rsort.empty() ? std::sort(jobsSortable.begin(),  jobsSortable.end(),  orderings[opt.sort]) :
                        std::sort(jobsSortable.rbegin(), jobsSortable.rend(), orderings[opt.sort]);
  }

  if (!opt.show_json) {
    for (std::vector<Arc::Job>::const_iterator it = jobsSortable.begin();
         it != jobsSortable.end(); ++it) {
      // Option 'long' (longlist) takes precedence over option 'print-jobids' (printids)
      if (opt.longlist || !opt.printids) {
        it->SaveToStream(std::cout, opt.longlist);
      }
      else {
        std::cout << it->JobID << std::endl;
      }
    }
  } else {
    std::cout << "\"jobs\": [";
    for (std::vector<Arc::Job>::const_iterator it = jobsSortable.begin();
         it != jobsSortable.end(); ++it) {
      std::cout << (it==jobsSortable.begin()?"":",") << std::endl;
      if (opt.longlist || !opt.printids) {
        it->SaveToStreamJSON(std::cout, opt.longlist);
      }
      else {
        std::cout << "\"" << it->JobID << "\"";
      }
    }
    std::cout << std::endl;
    std::cout << "]" << std::endl;
  }

  if (opt.show_unavailable) {
    jobmaster.SelectValid();
  }
  unsigned int returned_info_num = jobmaster.GetSelectedJobs().size();

  if (!opt.show_json) {
    std::cout << Arc::IString("Status of %d jobs was queried, %d jobs returned information", queried_num, returned_info_num) << std::endl;
  }

  return 0;
}
