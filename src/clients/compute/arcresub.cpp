// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <list>
#include <string>
#include <sys/types.h>
#include <sys/stat.h>

#include <arc/ArcLocation.h>
#include <arc/IString.h>
#include <arc/Logger.h>
#include <arc/URL.h>
#include <arc/UserConfig.h>
#include <arc/compute/Job.h>
#include <arc/compute/JobInformationStorage.h>
#include <arc/compute/JobSupervisor.h>

#include "utils.h"
#include "submit.h"

int RUNMAIN(arcresub)(int argc, char **argv) {

  setlocale(LC_ALL, "");

  Arc::Logger logger(Arc::Logger::getRootLogger(), "arcresub");
  Arc::LogStream logcerr(std::cerr);
  logcerr.setFormat(Arc::ShortFormat);
  Arc::Logger::getRootLogger().addDestination(logcerr);
  Arc::Logger::getRootLogger().setThreshold(Arc::WARNING);

  Arc::ArcLocation::Init(argv[0]);

  ClientOptions opt(ClientOptions::CO_RESUB, istring("[job ...]"));

  std::list<std::string> jobidentifiers = opt.Parse(argc, argv);

  if (opt.showversion) {
    std::cout << Arc::IString("%s version %s", "arcresub", VERSION)
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

  if (opt.show_plugins) {
    std::list<std::string> types;
    types.push_back("HED:SubmitterPlugin");
    types.push_back("HED:ServiceEndpointRetrieverPlugin");
    types.push_back("HED:TargetInformationRetrieverPlugin");
    types.push_back("HED:BrokerPlugin");
    showplugins("arcresub", types, logger, usercfg.Broker().first);
    return 0;
  }

  if (!checkproxy(usercfg)) {
    return 1;
  }

  if (opt.debug.empty() && !usercfg.Verbosity().empty())
    Arc::Logger::getRootLogger().setThreshold(Arc::istring_to_level(usercfg.Verbosity()));

  if (opt.same && opt.notsame) {
    logger.msg(Arc::ERROR, "--same and --not-same cannot be specified together.");
    return 1;
  }

  for (std::list<std::string>::const_iterator it = opt.jobidinfiles.begin(); it != opt.jobidinfiles.end(); ++it) {
    if (!Arc::Job::ReadJobIDsFromFile(*it, jobidentifiers)) {
      logger.msg(Arc::WARNING, "Cannot read specified jobid file: %s", *it);
    }
  }

  if (opt.timeout > 0)
    usercfg.Timeout(opt.timeout);

  if (!opt.broker.empty())
    usercfg.Broker(opt.broker);

  if ((!opt.joblist.empty() || !opt.status.empty()) && jobidentifiers.empty() && opt.clusters.empty())
    opt.all = true;

  if (jobidentifiers.empty() && opt.clusters.empty() && !opt.all) {
    logger.msg(Arc::ERROR, "No jobs given");
    return 1;
  }

  std::list<std::string> selectedURLs = getSelectedURLsFromUserConfigAndCommandLine(usercfg, opt.clusters);
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

  if (jobmaster.GetSelectedJobs().empty()) {
    std::cout << Arc::IString("No jobs") << std::endl;
    delete jobstore;
    return 1;
  }

  std::list<std::string> rejectDiscoveryURLs = getRejectDiscoveryURLsFromUserConfigAndCommandLine(usercfg, opt.rejectdiscovery);

  // select endpoints to resubmit to
  std::list<Arc::Endpoint> services;
  if ( opt.isARC6TargetSelectionOptions(logger, true) ) {
    // canonicalize endpoint types (allow -c that is used for filtering here)
    if (!opt.canonicalizeARC6InterfaceTypes(logger)) return 1;
    // get endpoint batches according to ARC6 target selection logic
    std::list<std::list<Arc::Endpoint> > endpoint_batches;
    bool info_discovery = prepare_submission_endpoint_batches(usercfg, opt, endpoint_batches);
    // JobSupervisor::Resubmit code only works with brokering
    if (!info_discovery) {
        logger.msg(Arc::ERROR,"It is not possible to resubmit jobs without new target information discovery");
        return 1;
    }
    // resubmit only to priority submission interface, no fallbacks
    services = endpoint_batches.front();
  } else {
    services = getServicesFromUserConfigAndCommandLine(usercfg, opt.indexurls, opt.qlusters, opt.requestedSubmissionInterfaceName, opt.infointerface);
  }

  std::list<Arc::Job> resubmittedJobs;
  // same + 2*notsame in {0,1,2}. same and notsame cannot both be true, see above.
  int retval = (int)!jobmaster.Resubmit((int)opt.same + 2*(int)opt.notsame, services, resubmittedJobs, rejectDiscoveryURLs);
  if (retval == 0 && resubmittedJobs.empty()) {
    std::cout << Arc::IString("No jobs to resubmit with the specified status") << std::endl;
    delete jobstore;
    return 0;
  }

  for (std::list<Arc::Job>::const_iterator it = resubmittedJobs.begin();
       it != resubmittedJobs.end(); ++it) {
    std::cout << Arc::IString("Job submitted with jobid: %s", it->JobID) << std::endl;
  }

  if (!resubmittedJobs.empty() && !jobstore->Write(resubmittedJobs)) {
    std::cout << Arc::IString("Warning: Failed to write job information to file (%s)", usercfg.JobListFile()) << std::endl;
    std::cout << Arc::IString("         To recover missing jobs, run arcsync") << std::endl;
    retval = 1;
  }

  if (!opt.jobidoutfile.empty() && !Arc::Job::WriteJobIDsToFile(resubmittedJobs, opt.jobidoutfile)) {
    logger.msg(Arc::WARNING, "Cannot write jobids to file (%s)", opt.jobidoutfile);
    retval = 1;
  }

  std::list<std::string> notresubmitted = jobmaster.GetIDsNotProcessed();

  if (!jobmaster.Cancel()) {
    retval = 1;
  }
  for (std::list<std::string>::const_iterator it = jobmaster.GetIDsNotProcessed().begin();
       it != jobmaster.GetIDsNotProcessed().end(); ++it) {
    logger.msg(Arc::WARNING, "Resubmission of job (%s) succeeded, but killing the job failed - it will still appear in the job list", *it);
  }

  if (!opt.keep) {
    if (!jobmaster.Clean()) {
      retval = 1;
    }
    for (std::list<std::string>::const_iterator it = jobmaster.GetIDsNotProcessed().begin();
         it != jobmaster.GetIDsNotProcessed().end(); ++it) {
      logger.msg(Arc::WARNING, "Resubmission of job (%s) succeeded, but cleaning the job failed - it will still appear in the job list", *it);
    }

    if (!jobstore->Remove(jobmaster.GetIDsProcessed())) {
      std::cout << Arc::IString("Warning: Failed removing jobs from file (%s)", usercfg.JobListFile()) << std::endl;
      std::cout << Arc::IString("         Use arcclean to remove non-existing jobs") << std::endl;
      retval = 1;
    }
  }
  delete jobstore;

  if ((resubmittedJobs.size() + notresubmitted.size()) > 1) {
    std::cout << std::endl << Arc::IString("Job resubmission summary:") << std::endl;
    std::cout << "-----------------------" << std::endl;
    std::cout << Arc::IString("%d of %d jobs were resubmitted", resubmittedJobs.size(), resubmittedJobs.size() + notresubmitted.size()) << std::endl;
    if (!notresubmitted.empty()) {
      std::cout << Arc::IString("The following %d were not resubmitted", notresubmitted.size()) << std::endl;
      for (std::list<std::string>::const_iterator it = notresubmitted.begin();
           it != notresubmitted.end(); ++it) {
        std::cout << *it << std::endl;
      }
    }
  }

  return retval;
}
