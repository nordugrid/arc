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
#include <arc/client/JobControllerPlugin.h>
#include <arc/client/JobSupervisor.h>
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
    Arc::Logger::getRootLogger().setThreshold(Arc::string_to_level(opt.debug));

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

  std::list<std::string> selectedURLs;
  if (!opt.clusters.empty()) {
    selectedURLs = getSelectedURLsFromUserConfigAndCommandLine(usercfg, opt.clusters);
  }
  std::list<std::string> rejectManagementURLs = getRejectManagementURLsFromUserConfigAndCommandLine(usercfg, opt.rejectmanagement);

  std::list<Arc::Job> jobs;
  if (!Arc::Job::ReadJobsFromFile(usercfg.JobListFile(), jobs, jobidentifiers, opt.all, selectedURLs, rejectManagementURLs)) {
    logger.msg(Arc::ERROR, "Unable to read job information from file (%s)", usercfg.JobListFile());
    return 1;
  }

  for (std::list<std::string>::const_iterator itJIdentifier = jobidentifiers.begin();
       itJIdentifier != jobidentifiers.end(); ++itJIdentifier) {
    std::cout << Arc::IString("Warning: Job not found in job list: %s", *itJIdentifier) << std::endl;
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
  Arc::Job::ResourceType resource;
  if (opt.show_joblog)      { resource = Arc::Job::JOBLOG; resourceName = "joblog"; }
  else if (opt.show_stderr) { resource = Arc::Job::STDERR; resourceName = "stderr"; }
  else                      { resource = Arc::Job::STDOUT; resourceName = "stdout"; }

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

  int retval = 0;
  for (std::list<Arc::Job>::const_iterator it = jobs.begin();
       it != jobs.end(); it++) {
    if (!it->State || (!opt.status.empty() &&
        std::find(opt.status.begin(), opt.status.end(), it->State()) == opt.status.end() &&
        std::find(opt.status.begin(), opt.status.end(), it->State.GetGeneralState()) == opt.status.end())) {
      continue;
    }

    if (it->State == Arc::JobState::DELETED) {
      logger.msg(Arc::WARNING, "Job deleted: %s", it->JobID.fullstr());
      retval = 1;
      continue;
    }

    // The job-log might be available before the job has started (middleware dependent).
    if (opt.show_joblog &&
        !it->State.IsFinished() &&
        it->State != Arc::JobState::RUNNING &&
        it->State != Arc::JobState::FINISHING) {
      logger.msg(Arc::WARNING, "Job has not started yet: %s", it->JobID.fullstr());
      retval = 1;
      continue;
    }

    if ((opt.show_joblog && it->LogDir.empty()) ||
        (!opt.show_joblog && opt.show_stderr && it->StdErr.empty()) ||
        (!opt.show_joblog && !opt.show_stderr && it->StdOut.empty())) {
      logger.msg(Arc::ERROR, "Cannot determine the %s location: %s", resourceName, it->JobID.fullstr());
      retval = 1;
      continue;
    }

    Arc::URL src;
    it->GetURLToResource(resource, src);
    if (!src) {
      logger.msg(Arc::ERROR, "Cannot create output of %s for job (%s): Invalid source %s", resourceName, it->JobID.fullstr(), src.str());
      retval = 1;
      continue;
    }

    if (!it->CopyJobFile(usercfg, src, dst)) {
      retval = 1;
      continue;
    }

    logger.msg(Arc::VERBOSE, "Catting %s for job %s", resourceName, it->JobID.fullstr());

    std::ifstream is(filename.c_str());
    char c;
    while (is.get(c)) {
      std::cout.put(c);
    }
    is.close();
  }

  close(tmp_h);
  unlink(filename.c_str());

  return retval;
}
