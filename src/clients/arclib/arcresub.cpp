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
#include <arc/FileLock.h>
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

int main(int argc, char **argv) {

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
                    istring("file where the jobs will be stored"),
                    istring("filename"),
                    joblist);

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
  options.AddOption('i', "index",
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
  targen.GetTargets(0, 1);

  if (targen.FoundTargets().empty()) {
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

  // Loop over jobs
  for (std::list<Arc::Job>::iterator it = toberesubmitted.begin();
       it != toberesubmitted.end(); it++) {

    Arc::JobDescription jobdesc;
    jobdesc.Parse(it->JobDescription);
    jobdesc.Identification.ActivityOldId = it->ActivityOldId;
    jobdesc.Identification.ActivityOldId.push_back(it->JobID.str());
    ChosenBroker->PreFilterTargets(targen.ModifyFoundTargets(), jobdesc);
    while (true) {
      const Arc::ExecutionTarget* target = ChosenBroker->GetBestTarget();
      if (!target) {
        std::cout << Arc::IString("Job submission failed, no more possible targets") << std::endl;
        break;
      }

      Arc::Submitter *submitter = target->GetSubmitter(usercfg2);

      //submit the job
      Arc::URL jobid = submitter->Submit(jobdesc, *target);
      if (!jobid) {
        std::cout << Arc::IString("Submission to %s failed, trying next target", target->url.str()) << std::endl;
        continue;
      }

      ChosenBroker->RegisterJobsubmission();
      std::cout << Arc::IString("Job resubmitted with new jobid: %s",
                                jobid.str()) << std::endl;

      jobs.push_back(it->JobID.str());
      break;
    } //end loop over all possible targets
  } //end loop over all job descriptions

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
       it != killcont.end(); it++)
    if (!(*it)->Kill(status, keep))
      if (!keep)
        if (!(*it)->Clean(status, true))
          logger.msg(Arc::WARNING, "Job could not be killed or cleaned");

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
