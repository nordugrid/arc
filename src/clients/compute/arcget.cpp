// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/stat.h>
#include <iostream>
#include <list>
#include <string>

#include <arc/ArcLocation.h>
#include <arc/IString.h>
#include <arc/Logger.h>
#include <arc/UserConfig.h>
#include <arc/FileUtils.h>
#include <arc/compute/JobInformationStorage.h>
#include <arc/compute/JobSupervisor.h>

#include "utils.h"

int RUNMAIN(arcget)(int argc, char **argv) {

  setlocale(LC_ALL, "");

  Arc::Logger logger(Arc::Logger::getRootLogger(), "arcget");
  Arc::LogStream logcerr(std::cerr);
  logcerr.setFormat(Arc::ShortFormat);
  Arc::Logger::getRootLogger().addDestination(logcerr);
  Arc::Logger::getRootLogger().setThreshold(Arc::WARNING);

  Arc::ArcLocation::Init(argv[0]);

  ClientOptions opt(ClientOptions::CO_GET,
                    istring("[job ...]"),
                    istring("The arcget command is used for "
                            "retrieving the results from a job."));

  std::list<std::string> jobidentifiers = opt.Parse(argc, argv);

  if (opt.showversion) {
    std::cout << Arc::IString("%s version %s", "arcget", VERSION)
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
    showplugins("arcget", types, logger);
    return 0;
  }

  Arc::UserConfig usercfg(opt.conffile, opt.joblist);
  if (!usercfg) {
    logger.msg(Arc::ERROR, "Failed configuration initialization");
    return 1;
  }
  if (opt.force_default_ca) usercfg.CAUseDefault(true);
  if (opt.force_grid_ca) usercfg.CAUseDefault(false);

  if (opt.debug.empty() && !usercfg.Verbosity().empty())
    Arc::Logger::getRootLogger().setThreshold(Arc::istring_to_level(usercfg.Verbosity()));

  if (opt.downloaddir.empty()) {
    if (!usercfg.JobDownloadDirectory().empty()) {
      opt.downloaddir = usercfg.JobDownloadDirectory();
      logger.msg(Arc::INFO, "Job download directory from user configuration file: %s", opt.downloaddir);
    }
    else {
      logger.msg(Arc::INFO, "Job download directory will be created in present working directory.");
    }
  }
  else {
    logger.msg(Arc::INFO, "Job download directory: %s", opt.downloaddir);
  }

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
    for (std::list<std::string>::const_iterator itJIdentifier = jobidentifiers.begin();
         itJIdentifier != jobidentifiers.end(); ++itJIdentifier) {
      std::cout << Arc::IString("Warning: Job not found in job list: %s", *itJIdentifier) << std::endl;
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

  if(!opt.downloaddir.empty()) {
    Arc::URL dirpath(opt.downloaddir);
    if(dirpath.Protocol() == "file") {
      if(!Arc::DirCreate(dirpath.Path(),S_IRWXU,true)) {
        std::string errstr = Arc::StrError();
        logger.msg(Arc::ERROR, "Unable to create directory for storing results (%s) - %s", dirpath.Path(), errstr);
        return 1;
      }
    }
  }
  std::list<std::string> downloaddirectories;
  int retval = (int)!jobmaster.Retrieve(opt.downloaddir, opt.usejobname, opt.forcedownload, downloaddirectories);

  for (std::list<std::string>::const_iterator it = downloaddirectories.begin();
       it != downloaddirectories.end(); ++it) {
    std::cout << Arc::IString("Results stored at: %s", *it) << std::endl;
  }

  unsigned int processed_num = jobmaster.GetIDsProcessed().size();
  unsigned int retrieved_num = downloaddirectories.size();
  unsigned int cleaned_num = 0;

  if (!opt.keep) {
    std::list<std::string> retrieved = jobmaster.GetIDsProcessed();
    // No need to clean selection because retrieved is subset of selected
    jobmaster.SelectByID(retrieved);
    if(!jobmaster.Clean()) {
      std::cout << Arc::IString("Warning: Some jobs were not removed from server") << std::endl;
      std::cout << Arc::IString("         Use arcclean to remove retrieved jobs from job list", usercfg.JobListFile()) << std::endl;
      retval = 1;
    }
    cleaned_num = jobmaster.GetIDsProcessed().size();

    if (!jobstore->Remove(jobmaster.GetIDsProcessed())) {
      std::cout << Arc::IString("Warning: Failed removing jobs from file (%s)", usercfg.JobListFile()) << std::endl;
      std::cout << Arc::IString("         Use arcclean to remove retrieved jobs from job list", usercfg.JobListFile()) << std::endl;
      retval = 1;
    }

    std::cout << Arc::IString("Jobs processed: %d, successfully retrieved: %d, successfully cleaned: %d", processed_num, retrieved_num, cleaned_num) << std::endl;

  } else {

    std::cout << Arc::IString("Jobs processed: %d, successfully retrieved: %d", processed_num, retrieved_num) << std::endl;

  }
  delete jobstore;

  return retval;
}
