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

#include <arc/ArcLocation.h>
#include <arc/DateTime.h>
#include <arc/FileLock.h>
#include <arc/IString.h>
#include <arc/Logger.h>
#include <arc/OptionParser.h>
#include <arc/StringConv.h>
#include <arc/Utils.h>
#include <arc/XMLNode.h>
#include <arc/client/TargetGenerator.h>
#include <arc/UserConfig.h>

#ifdef TEST
#define RUNSYNC(X) test_arcsync_##X
#else
#define RUNSYNC(X) X
#endif
int RUNSYNC(main)(int argc, char **argv) {

  setlocale(LC_ALL, "");

  Arc::Logger logger(Arc::Logger::getRootLogger(), "arcsync");
  Arc::LogStream logcerr(std::cerr);
  logcerr.setFormat(Arc::ShortFormat);
  Arc::Logger::getRootLogger().addDestination(logcerr);
  Arc::Logger::getRootLogger().setThreshold(Arc::WARNING);

  Arc::ArcLocation::Init(argv[0]);

  Arc::OptionParser options(" ",
                            istring("The arcsync command synchronizes your "
                                    "local job list with the information at\n"
                                    "the given resources or index servers."),
                            istring("Argument to -g has the format "
                                    "Flavour:URL e.g.\n"
                                    "ARC0:ldap://grid.tsl.uu.se:2135/"
                                    "mds-vo-name=sweden,O=grid\n"
                                    "CREAM:ldap://cream.grid.upjs.sk:2170/"
                                    "o=grid\n"
                                    "\n"
                                    "Argument to -c has the format "
                                    "Flavour:URL e.g.\n"
                                    "ARC0:ldap://grid.tsl.uu.se:2135/"
                                    "nordugrid-cluster-name=grid.tsl.uu.se,"
                                    "Mds-Vo-name=local,o=grid"));

  std::list<std::string> clusters;
  options.AddOption('c', "cluster",
                    istring("explicitly select or reject a specific resource"),
                    istring("[-]name"),
                    clusters);

  std::list<std::string> indexurls;
  options.AddOption('g', "index",
                    istring("explicitly select or reject an index server"),
                    istring("[-]name"),
                    indexurls);

  std::string joblist;
  options.AddOption('j', "joblist",
                    istring("file where the jobs will be stored"),
                    istring("filename"),
                    joblist);

  bool force = false;
  options.AddOption('f', "force",
                    istring("do not ask for verification"),
                    force);

  bool truncate = false;
  options.AddOption('T', "truncate",
                    istring("truncate the joblist before synchronizing"),
                    truncate);

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

  bool version = false;
  options.AddOption('v', "version", istring("print version information"),
                    version);

  std::list<std::string> params = options.Parse(argc, argv);

  if (version) {
    std::cout << Arc::IString("%s version %s", "arcsync", VERSION)
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

  //sanity check
  if (!force) {
    std::cout << Arc::IString("Synchronizing the local list of active jobs with the information in the\n"
                              "information system can result in some inconsistencies. Very recently submitted\n"
                              "jobs might not yet be present, whereas jobs very recently scheduled for\n"
                              "deletion can still be present."
                              ) << std::endl;
    std::cout << Arc::IString("Are you sure you want to synchronize your local job list?") << " ["
              << Arc::IString("y") << "/" << Arc::IString("n") << "] ";
    std::string response;
    std::cin >> response;
    if (Arc::lower(response) != std::string(Arc::FindTrans("y"))) {
      std::cout << Arc::IString("Cancelling synchronization request") << std::endl;
      return 0;
    }
  }

  if (!clusters.empty() || !indexurls.empty())
    usercfg.ClearSelectedServices();

  if (!clusters.empty())
    usercfg.AddServices(clusters, Arc::COMPUTING);

  if (!indexurls.empty())
    usercfg.AddServices(indexurls, Arc::INDEX);

  //Find all jobs
  Arc::TargetGenerator targen(usercfg);
  targen.RetrieveJobs();  

  bool jobsWritten = false;
  std::cout << "Found the following new jobs:"<<std::endl; 
  //Write extracted job info to joblist
  if (truncate) {
    if (jobsWritten = Arc::Job::WriteJobsToTruncatedFile(usercfg.JobListFile(), targen.GetJobs())) {
      for (std::list<Arc::Job>::const_iterator it = targen.GetJobs().begin();
           it != targen.GetJobs().end(); it++) {
        if (!it->Name.empty()) {
          std::cout << it->Name << " (" << it->JobID.fullstr() << ")" << std::endl;
        }
        else {
          std::cout << it->JobID.fullstr() << std::endl;
        }
      }
      std::cout << "Total number of new jobs found: " << targen.GetJobs().size() << std::endl;
    }
  }
  else {
    std::list<const Arc::Job*> newJobs;
    if (jobsWritten = Arc::Job::WriteJobsToFile(usercfg.JobListFile(), targen.GetJobs(), newJobs)) {
      for (std::list<const Arc::Job*>::const_iterator it = newJobs.begin();
           it != newJobs.end(); it++) {
        if (!(*it)->Name.empty()) {
          std::cout << (*it)->Name << " (" << (*it)->JobID.fullstr() << ")" << std::endl;
        }
        else {
          std::cout << (*it)->JobID.fullstr() << std::endl;
        }
      }
      std::cout << "Total number of new jobs found: " << newJobs.size() << std::endl;
    }
  }
  
  if (!jobsWritten) {
    std::cout << Arc::IString("ERROR: Failed to lock job list file %s", usercfg.JobListFile()) << std::endl;
    std::cout << Arc::IString("Please try again later, or manually clean up lock file") << std::endl;
    return 1;
  }

  return 0;
}
