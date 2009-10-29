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

int main(int argc, char **argv) {

  setlocale(LC_ALL, "");

  Arc::Logger logger(Arc::Logger::getRootLogger(), "arcsync");
  Arc::LogStream logcerr(std::cerr);
  logcerr.setFormat(Arc::ShortFormat);
  Arc::Logger::getRootLogger().addDestination(logcerr);
  Arc::Logger::getRootLogger().setThreshold(Arc::WARNING);

  Arc::ArcLocation::Init(argv[0]);

  Arc::OptionParser options(istring(" "),
                            istring("The command synchronized your local job"
                                    "list with the information at a given "
                                    "cluster or index server"),
                            istring("Argument to -i has the format "
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
                    istring("explicity select or reject a specific cluster"),
                    istring("[-]name"),
                    clusters);

  std::list<std::string> indexurls;
  options.AddOption('i', "index",
                    istring("explicity select or reject an index server"),
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
                    istring("truncate the joblist before sync"),
                    truncate);

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

  std::list<std::string> params = options.Parse(argc, argv);

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

  if (version) {
    std::cout << Arc::IString("%s version %s", "arcsync", VERSION)
              << std::endl;
    return 0;
  }

  //sanity check
  if (!force) {
    std::cout << Arc::IString("Synchronizing the local list of active jobs with the information in the MDS\n"
                              "can result in some inconsistencies. Very recently submitted jobs might not\n"
                              "yet be present in the MDS information, whereas jobs very recently scheduled\n"
                              "for deletion can still be present."
                              ) << std::endl;
    std::cout << Arc::IString("Are you sure you want to synchronize your local job list?") << " ["
              << Arc::IString("y") << "/" << Arc::IString("n") << "] ";
    std::string response;
    std::cin >> response;
    /*
       if (response != Arc::IString("y")) {
       std::cout << Arc::IString("Cancelling synchronization request") << std::endl;
       return;
       }
     */
  }

  if (!clusters.empty() || !indexurls.empty())
    usercfg.ClearSelectedServices();

  if (!clusters.empty())
    usercfg.AddServices(clusters, Arc::COMPUTING);

  if (!indexurls.empty())
    usercfg.AddServices(indexurls, Arc::INDEX);

  //Find all jobs
  Arc::TargetGenerator targen(usercfg);
  targen.GetTargets(1, 1);

  //Some dummy output
  std::cout << "Found number of jobs: " << targen.FoundJobs().size() << std::endl;

  //Write extracted job info to joblist (overwrite the file)
  { //start of file lock
    Arc::FileLock lock(usercfg.JobListFile());
    Arc::NS ns;
    Arc::Config jobs(ns);

    if (!truncate)
      jobs.ReadFromFile(usercfg.JobListFile());
    for (std::list<Arc::XMLNode*>::const_iterator itSyncedJob = targen.FoundJobs().begin();
         itSyncedJob != targen.FoundJobs().end(); itSyncedJob++) {
      if (!truncate)
        for (Arc::XMLNode j = jobs["Job"]; j; ++j)
          if ((std::string)j["JobID"] == (std::string)(**itSyncedJob)["JobID"]) {
            j.Destroy();
            break;
          }

      jobs.NewChild(**itSyncedJob);
    }
    jobs.SaveToFile(usercfg.JobListFile());
  } //end of file lock

  return 0;

}
