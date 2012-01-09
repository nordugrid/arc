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
#define RUNCAT(X) test_arccat_##X
#else
#define RUNCAT(X) X
#endif
int RUNCAT(main)(int argc, char **argv) {

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
    types.push_back("HED:JobController");
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

  std::string whichfile;
  if (opt.show_joblog)      whichfile = "joblog";
  else if (opt.show_stderr) whichfile = "stderr";
  else                      whichfile = "stdout";

  // saving to a temp file is necessary because chunks from server
  // may arrive out of order
  std::string filename = Glib::build_filename(Glib::get_tmp_dir(), "arccat.XXXXXX");
  int tmp_h = Glib::mkstemp(filename);
  if (tmp_h == -1) {
    logger.msg(Arc::INFO, "Could not create temporary file \"%s\"", filename);
    logger.msg(Arc::ERROR, "Cannot create output of %s for any jobs", whichfile);
    return 1;
  }

  Arc::URL dst("stdio:///"+Arc::tostring(tmp_h));
  if (!dst) {
    logger.msg(Arc::ERROR, "Cannot create output of %s for any jobs", whichfile);
    logger.msg(Arc::INFO, "Invalid destination URL %s", dst.str());
    close(tmp_h);
    unlink(filename.c_str());
    return 1;
  }

  int retval = 0;
  jobmaster.Update();
  jobs = jobmaster.GetJobs();
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
      logger.msg(Arc::ERROR, "Cannot determine the %s location: %s", whichfile, it->JobID.fullstr());
      retval = 1;
      continue;
    }

    std::map<std::string, Arc::JobController*>::const_iterator itJC = jobmaster.GetJobControllerMap().find(it->Flavour);
    if (itJC == jobmaster.GetJobControllerMap().end()) {
      logger.msg(Arc::VERBOSE, "Unable to find JobController for job %s (plugin type: %s)", it->JobID.fullstr(), it->Flavour);
      retval = 1;
      continue;
    }

    Arc::URL src = itJC->second->GetFileUrlForJob((*it), whichfile);
    if (!src) {
      logger.msg(Arc::ERROR, "Cannot create output of %s for job (%s): Invalid source %s", whichfile, it->JobID.fullstr(), src.str());
      retval = 1;
      continue;
    }

    if (!itJC->second->CopyJobFile(src, dst)) {
      retval = 1;
      continue;
    }

    logger.msg(Arc::VERBOSE, "Catting %s for job %s", whichfile, it->JobID.fullstr());

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
