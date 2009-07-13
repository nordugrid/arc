// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include <list>
#include <string>

#include <arc/ArcConfig.h>
#include <arc/ArcLocation.h>
#include <arc/IString.h>
#include <arc/Logger.h>
#include <arc/XMLNode.h>
#include <arc/OptionParser.h>
#include <arc/StringConv.h>
#include <arc/URL.h>
#include <arc/client/JobController.h>
#include <arc/client/JobSupervisor.h>
#include <arc/client/TargetGenerator.h>
#include <arc/client/UserConfig.h>
#include <arc/client/Job.h>
#include <arc/client/JobDescription.h>
#include <arc/DateTime.h>
#include <arc/FileLock.h>
#include <arc/Utils.h>
#include <arc/client/Submitter.h>
#include <arc/client/ACC.h>
#include <arc/client/Broker.h>
#include <arc/client/ACCLoader.h>

int main(int argc, char **argv) {

  setlocale(LC_ALL, "");

  Arc::Logger logger(Arc::Logger::getRootLogger(), "arcmigrate");
  Arc::LogStream logcerr(std::cerr);
  Arc::Logger::getRootLogger().addDestination(logcerr);
  Arc::Logger::getRootLogger().setThreshold(Arc::WARNING);

  Arc::ArcLocation::Init(argv[0]);

  Arc::OptionParser options(istring("[job ...]"),
                            istring("The arcmigrate command is used for "
                                    "migrating queud jobs to another cluster.\n"
                                    "Note that migration is only supported "
                                    "between ARC1 clusters."),
                            istring(" "));

  bool all = false;
  options.AddOption('a', "all",
                    istring("all jobs"),
                    all);

  bool forcemigration = false;
  options.AddOption('f', "force",
                    istring("force migration, ignore kill failure"),
                    forcemigration);

  std::string joblist;
  options.AddOption('j', "joblist",
                    istring("file containing a list of jobs"),
                    istring("filename"),
                    joblist);

  std::list<std::string> clusters;
  options.AddOption('c', "cluster",
                    istring("explicitly select or reject a cluster holding queued jobs"),
                    istring("[-]name"),
                    clusters);

  std::list<std::string> qlusters;
  options.AddOption('q', "qluster",
                    istring("explicitly select or reject a cluster to migrate to"),
                    istring("[-]name"),
                    qlusters);

  std::list<std::string> indexurls;
  options.AddOption('i', "index",
                    istring("explicitly select or reject an index server"),
                    istring("[-]name"),
                    indexurls);

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

  bool version = false;
  options.AddOption('v', "version", istring("print version information"),
                    version);

  std::string broker;
  options.AddOption('b', "broker",
                    istring("select broker method (RandomBroker (default), FastestQueueBroker, or custom)"),
                    istring("broker"), broker);

  std::list<std::string> jobs = options.Parse(argc, argv);

  Arc::UserConfig usercfg(conffile, joblist);
  if (!usercfg) {
    logger.msg(Arc::ERROR, "Failed configuration initialization");
    return 1;
  }

  if (timeout > 0) {
    usercfg.SetTimeOut(timeout);
  }

  if (!broker.empty())
    usercfg.SetBroker(broker);

  if (!debug.empty())
    Arc::Logger::getRootLogger().setThreshold(Arc::string_to_level(debug));
  else if (debug.empty() && usercfg.ConfTree()["Debug"]) {
    debug = (std::string)usercfg.ConfTree()["Debug"];
    Arc::Logger::getRootLogger().setThreshold(Arc::string_to_level(debug));
  }

  if (version) {
    std::cout << Arc::IString("%s version %s", "arcmigrate", VERSION)
              << std::endl;
    return 0;
  }

  // Proxy check
  if (!usercfg.CheckProxy())
    return 1;

  // If user specifies a joblist on the command line, he means to migrate jobs
  // stored in this file. So we should check if joblist is set or not, and not
  // if usercfg.JobListFile() is empty or not.
  if (jobs.empty() && joblist.empty() && clusters.empty() && !all) {
    logger.msg(Arc::ERROR, "No jobs given");
    return 1;
  }

  // If the user specified a joblist on the command line joblist equals
  // usercfg.JobListFile(). If not use the default, ie. usercfg.JobListFile().
  Arc::JobSupervisor jobmaster(usercfg, jobs, clusters, usercfg.JobListFile());
  std::list<Arc::JobController*> jobcont = jobmaster.GetJobControllers();

  if (jobcont.empty()) {
    logger.msg(Arc::ERROR, "No job controllers loaded");
    return 1;
  }

  Arc::TargetGenerator targetGen(usercfg, qlusters, indexurls);
  targetGen.GetTargets(0, 1);

  if (targetGen.FoundTargets().empty()) {
    std::cout << Arc::IString("Job migration aborted because no clusters returned any information") << std::endl;
    return 1;
  }

  Arc::Config cfg;
  usercfg.ApplyToConfig(cfg);
  Arc::ACCLoader loader;
  Arc::Broker *chosenBroker = dynamic_cast<Arc::Broker*>(loader.loadACC(usercfg.ConfTree()["Broker"]["Name"], &cfg));
  logger.msg(Arc::INFO, "Broker %s loaded", (std::string)usercfg.ConfTree()["Broker"]["Name"]);

  std::list<Arc::URL> migratedJobIDs;

  int retval = 0;
  // Loop over job controllers - arcmigrate should only support ARC-1 thus no loop...?
  for (std::list<Arc::JobController*>::iterator itJobCont = jobcont.begin(); itJobCont != jobcont.end(); itJobCont++) {
    if ((*itJobCont)->Flavour() != "ARC1") {
      std::cout << Arc::IString("Cannot migrate from %s clusters.", (*itJobCont)->Flavour()) << std::endl;
      std::cout << Arc::IString("Note: Migration is currently only supported between ARC1 clusters.") << std::endl;
      continue;
    }

    if (!(*itJobCont)->Migrate(targetGen, chosenBroker, usercfg, forcemigration, migratedJobIDs))
      retval = 1;
    for (std::list<Arc::URL>::iterator it = migratedJobIDs.begin();
         it != migratedJobIDs.end(); it++) {
      std::cout << Arc::IString("Job migrated with jobid: %s", it->str()) << std::endl;
    }
  } // Loop over job controllers

  return retval;
}
