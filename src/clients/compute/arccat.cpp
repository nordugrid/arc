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
#include <arc/compute/JobControllerPlugin.h>
#include <arc/compute/JobInformationStorage.h>
#include <arc/compute/JobSupervisor.h>
#include <arc/UserConfig.h>

#include "utils.h"

int RUNMAIN(arccat)(int argc, char **argv) {

  setlocale(LC_ALL, "");

  Arc::Logger logger(Arc::Logger::getRootLogger(), "arccat");
  Arc::LogStream logcerr(std::cerr);
  logcerr.setFormat(Arc::ShortFormat);
  Arc::Logger::getRootLogger().addDestination(logcerr);
  Arc::Logger::getRootLogger().setThreshold(Arc::WARNING);

  Arc::ArcLocation::Init(argv[0]);

  ClientOptions opt(ClientOptions::CO_CAT,
                    istring("[job ...]"),
                    istring("The arccat command performs the cat "
                            "command on the stdout, stderr or grid\n"
                            "manager's error log of the job."));

  std::list<std::string> jobidentifiers = opt.Parse(argc, argv);

  if (opt.showversion) {
    std::cout << Arc::IString("%s version %s", "arccat", VERSION)
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
    showplugins("arccat", types, logger);
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
  jobmaster.SelectValid();
  if (!opt.status.empty()) {
    jobmaster.SelectByStatus(opt.status);
  }

  jobs = jobmaster.GetSelectedJobs();
  if (jobs.empty()) {
    std::cout << Arc::IString("No jobs") << std::endl;
    return 1;
  }

  std::string resourceName;
  if (opt.show_joblog) { resourceName = "joblog"; }
  else if (opt.show_stderr) { resourceName = "stderr"; }
  else if (!opt.show_file.empty()) { resourceName = "session file"; }
  else { resourceName = "stdout"; }

  // saving to a temp file is necessary because chunks from server
  // may arrive out of order
  std::string filename = Glib::build_filename(Glib::get_tmp_dir(), "arccat.XXXXXX");
  int tmp_h = Glib::mkstemp(filename);
  if (tmp_h == -1) {
    logger.msg(Arc::INFO, "Could not create temporary file \"%s\"", filename);
    logger.msg(Arc::ERROR, "Cannot create output of %s for any jobs", resourceName);
    return 1;
  }

  Arc::URL dst("stdio:///"+Arc::tostring(tmp_h));
  if (!dst) {
    logger.msg(Arc::ERROR, "Cannot create output of %s for any jobs", resourceName);
    logger.msg(Arc::INFO, "Invalid destination URL %s", dst.str());
    close(tmp_h);
    unlink(filename.c_str());
    return 1;
  }

  Arc::URL stdoutdst("stdio:///stdout");

  int retval = 0;
  for (std::list<Arc::Job>::const_iterator it = jobs.begin();
       it != jobs.end(); ++it) {
    if (!it->State || (!opt.status.empty() &&
        std::find(opt.status.begin(), opt.status.end(), it->State()) == opt.status.end() &&
        std::find(opt.status.begin(), opt.status.end(), it->State.GetGeneralState()) == opt.status.end())) {
      continue;
    }

    if (it->State == Arc::JobState::DELETED) {
      logger.msg(Arc::WARNING, "Job deleted: %s", it->JobID);
      retval = 1;
      continue;
    }

    // The job-log might be available before the job has started (middleware dependent).
    if (!opt.show_joblog &&
        !it->State.IsFinished() &&
        it->State != Arc::JobState::RUNNING &&
        it->State != Arc::JobState::FINISHING) {
      logger.msg(Arc::WARNING, "Job has not started yet: %s", it->JobID);
      retval = 1;
      continue;
    }

    //if ((opt.show_joblog && it->LogDir.empty()) ||
    //    (!opt.show_joblog && opt.show_stderr && it->StdErr.empty()) ||
    //    (!opt.show_joblog && !opt.show_stderr && it->StdOut.empty())) {
    //  logger.msg(Arc::ERROR, "Cannot determine the %s location: %s", resourceName, it->JobID);
    //  retval = 1;
    //  continue;
    //}

    Arc::Job::ResourceType resource;
    if (opt.show_joblog) { resource = Arc::Job::JOBLOG; }
    else if (opt.show_stderr) { resource = Arc::Job::STDERR; }
    else if (!opt.show_file.empty()) {
      switch((Arc::JobState::StateType)it->State) {
        case Arc::JobState::ACCEPTED:
        case Arc::JobState::PREPARING:
        case Arc::JobState::SUBMITTING:
        case Arc::JobState::HOLD:
           resource = Arc::Job::STAGEINDIR;
           break;
        case Arc::JobState::QUEUING:
        case Arc::JobState::RUNNING:
        case Arc::JobState::OTHER:
        default:
           resource = Arc::Job::SESSIONDIR;
           break;
        case Arc::JobState::FINISHING:
        case Arc::JobState::FINISHED:
        case Arc::JobState::FAILED:
        case Arc::JobState::KILLED:
           resource = Arc::Job::STAGEOUTDIR;
           break;
      }
    }
    else { resource = Arc::Job::STDOUT; }
    Arc::URL src;
    if(!it->GetURLToResource(resource, src)) {
      logger.msg(Arc::ERROR, "Cannot determine the %s location: %s", resourceName, it->JobID);
      retval = 1;
      continue;
    }
    if (!src) {
      logger.msg(Arc::ERROR, "Cannot create output of %s for job (%s): Invalid source %s", resourceName, it->JobID, src.str());
      retval = 1;
      continue;
    }
    if (!opt.show_file.empty()) {
      src.ChangePath(src.Path()+"/"+opt.show_file);
    }

    if (!it->CopyJobFile(usercfg, src, dst, true)) {
      retval = 1;
      continue;
    }

    logger.msg(Arc::VERBOSE, "Catting %s for job %s", resourceName, it->JobID);

    // Use File DMC in order to handle proper writing to stdout (e.g. supporting redirection and piping from shell).
    if (!it->CopyJobFile(usercfg, dst, stdoutdst, true)) {
      retval = 1;
      continue;
    }
  }

  close(tmp_h);
  unlink(filename.c_str());

  return retval;
}
