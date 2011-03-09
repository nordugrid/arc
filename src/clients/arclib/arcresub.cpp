// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <fstream>
#include <iostream>
#include <list>
#include <string>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <arc/ArcConfig.h>
#include <arc/ArcLocation.h>
#include <arc/DateTime.h>
#include <arc/IString.h>
#include <arc/Logger.h>
#include <arc/OptionParser.h>
#include <arc/StringConv.h>
#include <arc/Utils.h>
#include <arc/URL.h>
#include <arc/XMLNode.h>
#include <arc/client/Submitter.h>
#include <arc/client/JobDescription.h>
#include <arc/client/JobController.h>
#include <arc/client/JobSupervisor.h>
#include <arc/client/TargetGenerator.h>
#include <arc/UserConfig.h>
#include <arc/client/Broker.h>
#include <arc/credential/Credential.h>

#ifdef TEST
#define RUNRESUB(X) test_arcresub_##X
#else
#define RUNRESUB(X) X
#endif
int RUNRESUB(main)(int argc, char **argv) {

  setlocale(LC_ALL, "");

  Arc::Logger logger(Arc::Logger::getRootLogger(), "arcresub");
  Arc::LogStream logcerr(std::cerr);
  logcerr.setFormat(Arc::ShortFormat);
  Arc::Logger::getRootLogger().addDestination(logcerr);
  Arc::Logger::getRootLogger().setThreshold(Arc::WARNING);

  Arc::ArcLocation::Init(argv[0]);

  Arc::OptionParser options(istring("[job ...]"));

  bool all = false;
  options.AddOption('a', "all",
                    istring("all jobs"),
                    all);

  std::string joblist;
  options.AddOption('j', "joblist",
                    istring("the file storing information about active jobs (default ~/.arc/jobs.xml)"),
                    istring("filename"),
                    joblist);
                    
  std::string jobidfileout;
  options.AddOption('o', "jobids-to-file",
                    istring("the IDs of the submitted jobs will be appended to this file"),
                    istring("filename"),
                    jobidfileout);

  std::list<std::string> jobidfilesin;
  options.AddOption('i', "jobids-from-file",
                    istring("a file containing a list of jobIDs"),
                    istring("filename"),
                    jobidfilesin);                    

  std::list<std::string> clusters;
  options.AddOption('c', "cluster",
                    istring("explicitly select or reject a specific resource"),
                    istring("[-]name"),
                    clusters);

  std::list<std::string> qlusters;
  options.AddOption('q', "qluster",
                    istring("explicitly select or reject a specific resource "
                            "for the new job"),
                    istring("[-]name"),
                    qlusters);

  std::list<std::string> indexurls;
  options.AddOption('g', "index",
                    istring("explicitly select or reject an index server"),
                    istring("[-]name"),
                    indexurls);

  bool keep = false;
  options.AddOption('k', "keep",
                    istring("keep the files on the server (do not clean)"),
                    keep);

  bool same = false;
  options.AddOption('m', "same",
                    istring("resubmit to the same resource"),
                    same);

  std::list<std::string> status;
  options.AddOption('s', "status",
                    istring("only select jobs whose status is statusstr"),
                    istring("statusstr"),
                    status);

  int timeout = -1;
  options.AddOption('t', "timeout", istring("timeout in seconds (default 20)"),
                    istring("seconds"), timeout);

  std::string conffile;
  options.AddOption('z', "conffile",
                    istring("configuration file (default ~/.arc/client.conf)"),
                    istring("filename"), conffile);

  std::string debug;
  options.AddOption('d', "debug",
                    istring("FATAL, ERROR, WARNING, INFO, VERBOSE or DEBUG"),
                    istring("debuglevel"), debug);

  std::string broker;
  options.AddOption('b', "broker",
                    istring("select broker method (Random (default), FastestQueue, or custom)"),
                    istring("broker"), broker);

  bool version = false;
  options.AddOption('v', "version", istring("print version information"),
                    version);


  std::list<std::string> jobs = options.Parse(argc, argv);

  for (std::list<std::string>::const_iterator it = jobidfilesin.begin(); it != jobidfilesin.end(); it++) {
    if (!Arc::Job::ReadJobIDsFromFile(*it, jobs)) {
      logger.msg(Arc::WARNING, "Cannot read specified jobid file: %s", *it);
    }
  }

  if (version) {
    std::cout << Arc::IString("%s version %s", "arcresub", VERSION)
              << std::endl;
    return 0;
  }

  // If debug is specified as argument, it should be set before loading the configuration.
  if (!debug.empty())
    Arc::Logger::getRootLogger().setThreshold(Arc::string_to_level(debug));

  Arc::UserConfig usercfg(conffile, joblist);
  if (!usercfg) {
    logger.msg(Arc::ERROR, "Failed configuration initialization");
    return 1;
  }

  if (!usercfg.ProxyPath().empty() ) {
    if (!usercfg.CACertificatesDirectory().empty() ){
      Arc::Credential holder(usercfg.ProxyPath(), "", usercfg.CACertificatesDirectory(), "");
      if(holder.IsValid() ){
        if (holder.GetEndTime() < Arc::Time()){
          std::cout << Arc::IString("Proxy expired. Job submission aborted. Please run 'arcproxy'!") << std::endl;
          return 1;
        }
        else if (holder.GetVerification()) {
          logger.msg(Arc::INFO, "Proxy successfully verified.");
        }
      }
      else {
          std::cout << Arc::IString("Proxy not valid. Job submission aborted. Please run 'arcproxy'!") << std::endl;
          return 1;
        }
      }
      else {
      std::cout << Arc::IString("Cannot find CA certificates directory. Please specify the location to the directory in the client configuration file.") << std::endl;
      return 1;
    }
  }
  else {
    std::cout << Arc::IString("Cannot find any proxy. arcresub currently cannot run without a proxy.\n"
                              "  If you have the proxy file in a non-default location,\n"
                              "  please make sure the path is specified in the client configuration file.\n"
                              "  If you don't have a proxy yet, please run 'arcproxy'!") << std::endl;
    return 1;
  }

  if (debug.empty() && !usercfg.Verbosity().empty())
    Arc::Logger::getRootLogger().setThreshold(Arc::string_to_level(usercfg.Verbosity()));

  if (timeout > 0)
    usercfg.Timeout(timeout);

  if (!broker.empty())
    usercfg.Broker(broker);

  if ((!joblist.empty() || !status.empty()) && jobs.empty() && clusters.empty())
    all = true;

  if (jobs.empty() && clusters.empty() && !all) {
    logger.msg(Arc::ERROR, "No jobs given");
    return 1;
  }

  // Different selected services are needed in two different context,
  // so the two copies of UserConfig objects will contain different
  // selected services.
  Arc::UserConfig usercfg2 = usercfg;

  if (!jobs.empty() || all)
    usercfg.ClearSelectedServices();

  if (!clusters.empty()) {
    usercfg.ClearSelectedServices();
    usercfg.AddServices(clusters, Arc::COMPUTING);
  }

  Arc::JobSupervisor jobmaster(usercfg, jobs);
  if (!jobmaster.JobsFound()) {
    std::cout << "No jobs" << std::endl;
    return 0;
  }
  std::list<Arc::JobController*> jobcont = jobmaster.GetJobControllers();

  // If the user specified a joblist on the command line joblist equals
  // usercfg.JobListFile(). If not use the default, ie. usercfg.JobListFile().
  if (jobcont.empty()) {
    logger.msg(Arc::ERROR, "No job controller plugins loaded");
    return 1;
  }

  // Clearing jobs.
  jobs.clear();

  std::list<Arc::Job> toberesubmitted;
  for (std::list<Arc::JobController*>::iterator it = jobcont.begin();
       it != jobcont.end(); it++) {
    std::list<Arc::Job> cont_jobs;
    cont_jobs = (*it)->GetJobDescriptions(status, true);
    toberesubmitted.insert(toberesubmitted.begin(), cont_jobs.begin(), cont_jobs.end());
  }
  if (toberesubmitted.empty()) {
    logger.msg(Arc::ERROR, "No jobs to resubmit");
    return 1;
  }

  if (same) {
    qlusters.clear();
    usercfg2.ClearSelectedServices();
  }
  else if (!qlusters.empty() || !indexurls.empty())
    usercfg2.ClearSelectedServices();

  // Preventing resubmitted jobs to be send to old clusters
  for (std::list<Arc::Job>::iterator it = toberesubmitted.begin();
       it != toberesubmitted.end(); it++)
    if (same) {
      qlusters.push_back(it->Flavour + ":" + it->Cluster.str());
      logger.msg(Arc::VERBOSE, "Trying to resubmit job to %s", qlusters.front());
    }
    else {
      qlusters.remove(it->Flavour + ":" + it->Cluster.str());
      qlusters.push_back("-" + it->Flavour + ":" + it->Cluster.str());
      logger.msg(Arc::VERBOSE, "Disregarding %s", it->Cluster.str());
    }
  qlusters.sort();
  qlusters.unique();

  usercfg2.AddServices(qlusters, Arc::COMPUTING);
  if (!same && !indexurls.empty())
    usercfg2.AddServices(indexurls, Arc::INDEX);

  // Resubmitting jobs
  Arc::TargetGenerator targen(usercfg2);
  targen.RetrieveExecutionTargets();

  if (targen.GetExecutionTargets().empty()) {
    std::cout << Arc::IString("Job submission aborted because no resource returned any information") << std::endl;
    return 1;
  }

  Arc::BrokerLoader loader;
  Arc::Broker *ChosenBroker = loader.load(usercfg.Broker().first, usercfg2);
  if (!ChosenBroker) {
    logger.msg(Arc::ERROR, "Unable to load broker %s", usercfg2.Broker().first);
    return 1;
  }

  logger.msg(Arc::INFO, "Broker %s loaded", usercfg2.Broker().first);

  std::list<Arc::Job> resubmittedJobs;

  // Loop over jobs
  for (std::list<Arc::Job>::iterator it = toberesubmitted.begin();
       it != toberesubmitted.end(); it++) {
    resubmittedJobs.push_back(Arc::Job());

    std::list<Arc::JobDescription> jobdescs;
    Arc::JobDescription::Parse(it->JobDescriptionDocument, jobdescs); // Do not check for validity. We are only interested in that the outgoing job description is valid.
    if (jobdescs.empty()) {
      std::cout << Arc::IString("Job resubmission failed, unable to parse obtained job description") << std::endl;
      resubmittedJobs.pop_back();
      continue;
    }
    jobdescs.front().Identification.ActivityOldId = it->ActivityOldID;
    jobdescs.front().Identification.ActivityOldId.push_back(it->JobID.str());

    // remove the queuename which was added during the original submission of the job
    jobdescs.front().Resources.QueueName = "";    

    if (ChosenBroker->Submit(targen.GetExecutionTargets(), jobdescs.front(), resubmittedJobs.back())) {
      std::string jobid = resubmittedJobs.back().JobID.str();
      if (!jobidfileout.empty())
        if (!Arc::Job::WriteJobIDToFile(jobid, jobidfileout))
          logger.msg(Arc::WARNING, "Cannot write jobid (%s) to file (%s)", jobid, jobidfileout);
      std::cout << Arc::IString("Job resubmitted with new jobid: %s", jobid) << std::endl;
      jobs.push_back(it->JobID.str());
    }
    else {
      std::cout << Arc::IString("Job resubmission failed, no more possible targets") << std::endl;
      resubmittedJobs.pop_back();
    }
  } //end loop over all job descriptions

  if (!Arc::Job::WriteJobsToFile(usercfg.JobListFile(), resubmittedJobs)) {
    std::cout << Arc::IString("Warning: Failed to lock job list file %s", usercfg.JobListFile())
              << std::endl;
    std::cout << Arc::IString("To recover missing jobs, run arcsync") << std::endl;
  }

  if (jobs.empty())
    return 0;

  usercfg.ClearSelectedServices();

  // Only kill and clean jobs that have been resubmitted
  Arc::JobSupervisor killmaster(usercfg, jobs);
  if (!killmaster.JobsFound()) {
    std::cout << "No jobs" << std::endl;
    return 0;
  }
  std::list<Arc::JobController*> killcont = killmaster.GetJobControllers();
  if (killcont.empty()) {
    logger.msg(Arc::ERROR, "No job controller plugins loaded");
    return 1;
  }

  for (std::list<Arc::JobController*>::iterator it = killcont.begin();
       it != killcont.end(); it++) {
    // Order matters.
    if (!(*it)->Kill(status, keep) && !keep && !(*it)->Clean(status, true)) {
      logger.msg(Arc::WARNING, "Job could not be killed or cleaned");
    }
  }

  /*
     if (toberesubmitted.size() > 1) {
     std::cout << std::endl << Arc::IString("Job submission summary:")
     << std::endl;
     std::cout << "-----------------------" << std::endl;
     std::cout << Arc::IString("%d of %d jobs were submitted",
     toberesubmitted.size() - notresubmitted.size(),
     toberesubmitted.size()) << std::endl;
     if (notresubmitted.size()) {
     std::cout << Arc::IString("The following %d were not submitted",
     notresubmitted.size()) << std::endl;
     }
     }*/
  return 0;
}
