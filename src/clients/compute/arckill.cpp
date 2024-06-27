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

int RUNMAIN(arckill)(int argc, char **argv) {

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

  std::list<std::string> jobidentifiers = opt.Parse(argc, argv);

  if (opt.showversion) {
    std::cout << Arc::IString("%s version %s", "arckill", VERSION)
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
    showplugins("arckill", types, logger);
    return 0;
  }

  Arc::UserConfig usercfg(opt.conffile, opt.joblist);
  if (!usercfg) {
    logger.msg(Arc::ERROR, "Failed configuration initialization");
    return 1;
  }
  if (opt.force_system_ca) usercfg.CAUseSystem(true);
  if (opt.force_grid_ca) usercfg.CAUseSystem(false);

  if (opt.debug.empty() && !usercfg.Verbosity().empty())
    Arc::Logger::getRootLogger().setThreshold(Arc::istring_to_level(usercfg.Verbosity()));

  for (std::list<std::string>::const_iterator it = opt.jobidinfiles.begin(); it != opt.jobidinfiles.end(); ++it) {
    if (!Arc::Job::ReadJobIDsFromFile(*it, jobidentifiers)) {
      logger.msg(Arc::WARNING, "Cannot read specified jobid file: %s", *it);
    }
  }

  if (opt.timeout > 0)
    usercfg.Timeout(opt.timeout);

  AuthenticationType authentication_type = UndefinedAuthentication;
  if(!opt.getAuthenticationType(logger, usercfg, authentication_type))
    return 1;
  switch(authentication_type) {
    case NoAuthentication:
      usercfg.CommunicationAuthType(Arc::UserConfig::AuthTypeNone);
      break;
    case X509Authentication:
      usercfg.CommunicationAuthType(Arc::UserConfig::AuthTypeCert);
      break;
    case TokenAuthentication:
      usercfg.CommunicationAuthType(Arc::UserConfig::AuthTypeToken);
      break;
    case UndefinedAuthentication:
    default:
      usercfg.CommunicationAuthType(Arc::UserConfig::AuthTypeUndefined);
      break;
  }

  if ((!opt.joblist.empty() || !opt.status.empty()) && jobidentifiers.empty() && opt.computing_elements.empty())
    opt.all = true;

  if (jobidentifiers.empty() && opt.computing_elements.empty() && !opt.all) {
    logger.msg(Arc::ERROR, "No jobs given");
    return 1;
  }

  std::list<std::string> selectedURLs;
  if (!opt.computing_elements.empty()) {
    selectedURLs = getSelectedURLsFromUserConfigAndCommandLine(usercfg, opt.computing_elements);
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

  if (jobmaster.GetSelectedJobs().empty()) {
    std::cout << Arc::IString("No jobs") << std::endl;
    delete jobstore;
    return 1;
  }

  int retval = (int)!jobmaster.Cancel();

  unsigned int selected_num = jobmaster.GetSelectedJobs().size();
  unsigned int canceled_num = jobmaster.GetIDsProcessed().size();
  unsigned int cleaned_num = 0;

  if (!opt.keep) {
    std::list<std::string> canceled = jobmaster.GetIDsProcessed();
    // No need to clean selection because retrieved is subset of selected
    jobmaster.SelectByID(canceled);
    if(!jobmaster.Clean()) {
      std::cout << Arc::IString("Warning: Some jobs were not removed from server") << std::endl;
      std::cout << Arc::IString("         Use arcclean to remove retrieved jobs from job list", usercfg.JobListFile()) << std::endl;
      retval = 1;
    }
    cleaned_num = jobmaster.GetIDsProcessed().size();

    if (!jobstore->Remove(jobmaster.GetIDsProcessed())) {
      std::cout << Arc::IString("Warning: Failed removing jobs from file (%s)", usercfg.JobListFile()) << std::endl;
      std::cout << Arc::IString("         Run 'arcclean -s Undefined' to remove killed jobs from job list") << std::endl;
      retval = 1;
    }
    std::cout << Arc::IString("Jobs processed: %d, successfully killed: %d, successfully cleaned: %d", selected_num, canceled_num, cleaned_num) << std::endl;
  } else {
    std::cout << Arc::IString("Jobs processed: %d, successfully killed: %d", selected_num, canceled_num) << std::endl;
  }
  delete jobstore;

  return retval;
}
