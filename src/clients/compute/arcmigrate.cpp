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
#include <arc/URL.h>
#include <arc/UserConfig.h>
#include <arc/compute/Job.h>
#include <arc/compute/JobSupervisor.h>

#include "utils.h"

int RUNMAIN(arcmigrate)(int argc, char **argv) {

  setlocale(LC_ALL, "");

  Arc::Logger logger(Arc::Logger::getRootLogger(), "arcmigrate");
  Arc::LogStream logcerr(std::cerr);
  logcerr.setFormat(Arc::ShortFormat);
  Arc::Logger::getRootLogger().addDestination(logcerr);
  Arc::Logger::getRootLogger().setThreshold(Arc::WARNING);

  Arc::ArcLocation::Init(argv[0]);

  ClientOptions opt(ClientOptions::CO_MIGRATE,
                    istring("[job ...]"),
                    istring("The arcmigrate command is used for "
                            "migrating queued jobs to another resource.\n"
                            "Note that migration is only supported "
                            "between A-REX powered resources."));

  std::list<std::string> jobidentifiers = opt.Parse(argc, argv);

  if (opt.showversion) {
    std::cout << Arc::IString("%s version %s", "arcmigrate", VERSION)
              << std::endl;
    return 0;
  }

  // If debug is specified as argument, it should be set before loading the configuration.
  if (!opt.debug.empty())
    Arc::Logger::getRootLogger().setThreshold(Arc::string_to_level(opt.debug));

  Arc::UserConfig usercfg(opt.conffile, opt.joblist);
  if (!usercfg) {
    logger.msg(Arc::ERROR, "Failed configuration initialization");
    return 1;
  }

  if (opt.show_plugins) {
    std::list<std::string> types;
    types.push_back("HED:SubmitterPlugin");
    types.push_back("HED:ServiceEndpointRetrieverPlugin");
    types.push_back("HED:TargetInformationRetrieverPlugin");
    types.push_back("HED:BrokerPlugin");
    showplugins("arcmigrate", types, logger, usercfg.Broker().first);
    return 0;
  }

  if (!checkproxy(usercfg)) {
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

  if (!opt.broker.empty())
    usercfg.Broker(opt.broker);

  if (!opt.joblist.empty() && jobidentifiers.empty() && opt.clusters.empty())
    opt.all = true;

  if (jobidentifiers.empty() && opt.clusters.empty() && !opt.all) {
    logger.msg(Arc::ERROR, "No jobs given");
    return 1;
  }

  std::list<std::string> selectedURLs = getSelectedURLsFromUserConfigAndCommandLine(usercfg, opt.clusters);
  std::list<std::string> rejectManagementURLs = getRejectManagementURLsFromUserConfigAndCommandLine(usercfg, opt.rejectmanagement);

  std::list<Arc::Job> jobs;
  Arc::JobInformationStorageXML jobList(usercfg.JobListFile());
  if (( opt.all && !jobList.ReadAll(jobs, rejectManagementURLs)) ||
      (!opt.all && !jobList.Read(jobs, jobidentifiers, selectedURLs, rejectManagementURLs))) {
    logger.msg(Arc::ERROR, "Unable to read job information from file (%s)", usercfg.JobListFile());
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
  jobmaster.SelectByStatus(std::list<std::string>(1, "Queuing"));
  if (jobmaster.GetSelectedJobs().empty()) {
    std::cout << Arc::IString("No jobs") << std::endl;
    return 1;
  }

  std::list<Arc::Endpoint> services = getServicesFromUserConfigAndCommandLine(usercfg, opt.qlusters, opt.indexurls, opt.requestedSubmissionInterfaceName, opt.infointerface);

  std::list<std::string> rejectDiscoveryURLs = getRejectDiscoveryURLsFromUserConfigAndCommandLine(usercfg, opt.rejectdiscovery);

  std::list<Arc::Job> migratedJobs;
  int retval = (int)!jobmaster.Migrate(opt.forcemigration, services, migratedJobs, rejectDiscoveryURLs);

  std::list<std::string> notmigrated = jobmaster.GetIDsNotProcessed();

  for (std::list<Arc::Job>::const_iterator it = migratedJobs.begin();
       it != migratedJobs.end(); ++it) {
    std::cout << Arc::IString("Job submitted with jobid: %s", it->JobID) << std::endl;
  }

  if (!migratedJobs.empty() && !jobList.Write(migratedJobs)) {
    std::cout << Arc::IString("Warning: Failed to lock job list file %s", usercfg.JobListFile()) << std::endl;
    std::cout << Arc::IString("         To recover missing jobs, run arcsync") << std::endl;
    retval = 1;
  }

  if (!opt.jobidoutfile.empty() && !Arc::Job::WriteJobIDsToFile(migratedJobs, opt.jobidoutfile)) {
    logger.msg(Arc::WARNING, "Cannot write job IDs of submitted jobs to file (%s)", opt.jobidoutfile);
    retval = 1;
  }

  if (!jobmaster.Cancel()) {
    retval = 1;
  }
  for (std::list<std::string>::const_iterator it = jobmaster.GetIDsNotProcessed().begin();
       it != jobmaster.GetIDsNotProcessed().end(); ++it) {
    logger.msg(Arc::WARNING, "Migration of job (%s) succeeded, but killing the job failed - it will still appear in the job list", *it);
  }

  if (!opt.keep) {
    if (!jobmaster.Clean()) {
      retval = 1;
    }
    for (std::list<std::string>::const_iterator it = jobmaster.GetIDsNotProcessed().begin();
         it != jobmaster.GetIDsNotProcessed().end(); ++it) {
      logger.msg(Arc::WARNING, "Migration of job (%s) succeeded, but cleaning the job failed - it will still appear in the job list", *it);
    }

    if (!jobList.Remove(jobmaster.GetIDsProcessed())) {
      std::cout << Arc::IString("Warning: Failed to lock job list file %s", usercfg.JobListFile()) << std::endl;
      std::cout << Arc::IString("         Use arcclean to remove non-existing jobs") << std::endl;
      retval = 1;
    }
  }

  if ((migratedJobs.size() + notmigrated.size()) > 1) {
    std::cout << std::endl << Arc::IString("Job migration summary:") << std::endl;
    std::cout << "-----------------------" << std::endl;
    std::cout << Arc::IString("%d of %d jobs were migrated", migratedJobs.size(), migratedJobs.size() + notmigrated.size()) << std::endl;
    if (!notmigrated.empty()) {
      std::cout << Arc::IString("The following %d were not migrated", notmigrated.size()) << std::endl;
      for (std::list<std::string>::const_iterator it = notmigrated.begin();
           it != notmigrated.end(); ++it) {
        std::cout << *it << std::endl;
      }
    }
  }

  return retval;
}
