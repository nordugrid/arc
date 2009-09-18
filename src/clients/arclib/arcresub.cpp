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
  Arc::Logger::getRootLogger().addDestination(logcerr);
  Arc::Logger::getRootLogger().setThreshold(Arc::WARNING);

  Arc::ArcLocation::Init(argv[0]);

  Arc::OptionParser options(istring("[job ...]\n"));

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
                    istring("explicity select or reject a specific cluster"),
                    istring("[-]name"),
                    clusters);

  std::list<std::string> qlusters;
  options.AddOption('q', "qluster",
                    istring("explicity select or reject a specific cluster "
                            "for the new job"),
                    istring("[-]name"),
                    qlusters);

  std::list<std::string> indexurls;
  options.AddOption('i', "index",
                    istring("explicity select or reject an index server"),
                    istring("[-]name"),
                    indexurls);

  bool keep = false;
  options.AddOption('k', "keep",
                    istring("keep the files on the server (do not clean)"),
                    keep);

  bool same = false;
  options.AddOption('m', "same",
                    istring("resubmit to the same cluster"),
                    same);

  std::list<std::string> status;
  options.AddOption('s', "status",
                    istring("only select jobs whose status is statusstr"),
                    istring("statusstr"),
                    status);

  int timeout = -1;
  options.AddOption('t', "timeout", istring("timeout in seconds (default " + Arc::tostring(Arc::UserConfig::DEFAULT_TIMEOUT) + ")"),
                    istring("seconds"), timeout);

  std::string conffile;
  options.AddOption('z', "conffile",
                    istring("configuration file (default ~/.arc/client.xml)"),
                    istring("filename"), conffile);

  std::string debug;
  options.AddOption('d', "debug",
                    istring("FATAL, ERROR, WARNING, INFO, DEBUG or VERBOSE"),
                    istring("debuglevel"), debug);

  std::string broker;
  options.AddOption('b', "broker",
                    istring("select broker method (Random (default), QueueBalance, or custom)"),
                    istring("broker"), broker);

  bool version = false;
  options.AddOption('v', "version", istring("print version information"),
                    version);


  std::list<std::string> jobs = options.Parse(argc, argv);

  // If debug is specified as argument, it should be set before loading the configuration.
  if (!debug.empty())
    Arc::Logger::getRootLogger().setThreshold(Arc::string_to_level(debug));

  Arc::UserConfig usercfg(conffile, joblist);
  if (!usercfg) {
    logger.msg(Arc::ERROR, "Failed configuration initialization");
    return 1;
  }

  if (debug.empty() && usercfg.ConfTree()["Debug"]) {
    debug = (std::string)usercfg.ConfTree()["Debug"];
    Arc::Logger::getRootLogger().setThreshold(Arc::string_to_level(debug));
  }

  if (timeout > 0) {
    usercfg.SetTimeOut(timeout);
  }

  if (!broker.empty())
    usercfg.SetBroker(broker);

  if (version) {
    std::cout << Arc::IString("%s version %s", "arcresub", VERSION)
              << std::endl;
    return 0;
  }

  // If user specifies a joblist on the command line, he means to resub jobs
  // stored in this file. So we should check if joblist is set or not, and not
  // if usercfg.JobListFile() is empty or not.
  if (jobs.empty() && joblist.empty() && clusters.empty() && !all) {
    logger.msg(Arc::ERROR, "No jobs given");
    return 1;
  }
  
  Arc::JobSupervisor jobmaster(usercfg, jobs, clusters, usercfg.JobListFile());
  std::list<Arc::JobController*> jobcont = jobmaster.GetJobControllers();

  // If the user specified a joblist on the command line joblist equals
  // usercfg.JobListFile(). If not use the default, ie. usercfg.JobListFile().
  if (jobcont.empty()) {
    logger.msg(Arc::ERROR, "No job controllers loaded");
    return 1;
  }
  
  // Clearing jobs and cluster
  jobs.clear();
  clusters.clear();

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

  // Preventing resubmitted jobs to be send to old clusters
  if (same)
    qlusters.clear();
  for (std::list<Arc::Job>::iterator it = toberesubmitted.begin();
       it != toberesubmitted.end(); it++)
    if (same) {
      qlusters.push_back(it->Flavour + ":" + it->Cluster.str());
      logger.msg(Arc::DEBUG, "Trying to resubmit job to %s", qlusters.front());
    }
    else {
      qlusters.remove(it->Flavour + ":" + it->Cluster.str());
      qlusters.push_back("-" + it->Flavour + ":" + it->Cluster.str());
      logger.msg(Arc::DEBUG, "Disregarding %s", it->Cluster.str());
    }
  qlusters.sort();
  qlusters.unique();

  // Resubmitting jobs
  Arc::TargetGenerator targen(usercfg, qlusters, indexurls);
  targen.GetTargets(0, 1);

  if (targen.FoundTargets().empty()) {
    std::cout << Arc::IString("Job submission aborted because no clusters returned any information") << std::endl;
    return 1;
  }

  Arc::Config cfg;
  cfg.NewChild("Arguments") =
    (std::string)usercfg.ConfTree()["Broker"]["Arguments"];
  Arc::BrokerLoader loader;
  Arc::Broker *ChosenBroker = loader.load(usercfg.ConfTree()["Broker"]["Name"],
                                          cfg, usercfg);
  logger.msg(Arc::INFO, "Broker %s loaded",
             (std::string)usercfg.ConfTree()["Broker"]["Name"]);

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

      Arc::Submitter *submitter = target->GetSubmitter(usercfg);

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

  // Only kill and clean jobs that have been resubmitted
  Arc::JobSupervisor killmaster(usercfg, jobs, clusters, usercfg.JobListFile());
  std::list<Arc::JobController*> killcont = killmaster.GetJobControllers();
  if (killcont.empty()) {
    logger.msg(Arc::ERROR, "No job controllers loaded");
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
