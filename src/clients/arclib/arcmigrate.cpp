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
#include <arc/OptionParser.h>
#include <arc/StringConv.h>
#include <arc/URL.h>
#include <arc/UserConfig.h>
#include <arc/client/Job.h>
#include <arc/client/JobSupervisor.h>
#include <arc/credential/Credential.h>
#include <arc/loader/FinderLoader.h>
#include <arc/loader/Plugin.h>

#ifdef TEST
#define RUNMIGRATE(X) test_arcmigrate_##X
#else
#define RUNMIGRATE(X) X
#endif
int RUNMIGRATE(main)(int argc, char **argv) {

  setlocale(LC_ALL, "");

  Arc::Logger logger(Arc::Logger::getRootLogger(), "arcmigrate");
  Arc::LogStream logcerr(std::cerr);
  logcerr.setFormat(Arc::ShortFormat);
  Arc::Logger::getRootLogger().addDestination(logcerr);
  Arc::Logger::getRootLogger().setThreshold(Arc::WARNING);

  Arc::ArcLocation::Init(argv[0]);

  Arc::OptionParser options(istring("[job ...]"),
                            istring("The arcmigrate command is used for "
                                    "migrating queued jobs to another resource.\n"
                                    "Note that migration is only supported "
                                    "between A-REX powered resources."));

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
                    istring("the file storing information about active jobs (default ~/.arc/jobs.xml)"),
                    istring("filename"),
                    joblist);

  std::string jobidfileout;
  options.AddOption('o', "jobids-to-file",
                    istring("the IDs of the submitted jobs will be appended to this file"),
                    istring("filename"),
                    jobidfileout);

  std::list<std::string> jobidfiles;
  options.AddOption('i', "jobids-from-file",
                    istring("a file containing a list of jobIDs"),
                    istring("filename"),
                    jobidfiles);

  std::list<std::string> clusters;
  options.AddOption('c', "cluster",
                    istring("explicitly select or reject a resource holding queued jobs"),
                    istring("[-]name"),
                    clusters);

  std::list<std::string> qlusters;
  options.AddOption('q', "qluster",
                    istring("explicitly select or reject a resource to migrate to"),
                    istring("[-]name"),
                    qlusters);

  std::list<std::string> indexurls;
  options.AddOption('g', "index",
                    istring("explicitly select or reject an index server"),
                    istring("[-]name"),
                    indexurls);

  bool show_plugins = false;
  options.AddOption('P', "listplugins",
                    istring("list the available plugins"),
                    show_plugins);

  bool keep = false;
  options.AddOption('k', "keep",
                    istring("keep the files on the server (do not clean)"),
                    keep);

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

  std::string broker;
  options.AddOption('b', "broker",
                    istring("select broker method (Random (default), FastestQueue, or custom)"),
                    istring("broker"), broker);

  std::list<std::string> jobIDsAndNames = options.Parse(argc, argv);

  if (version) {
    std::cout << Arc::IString("%s version %s", "arcmigrate", VERSION)
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

  if (show_plugins) {
    std::list<Arc::ModuleDesc> modules;
    Arc::PluginsFactory pf(Arc::BaseConfig().MakeConfig(Arc::Config()).Parent());

    pf.scan(Arc::FinderLoader::GetLibrariesList(), modules);
    Arc::PluginsFactory::FilterByKind("HED:Submitter", modules);
    std::cout << Arc::IString("Types of execution services arcmigrate is able to submit jobs to:") << std::endl;
    for (std::list<Arc::ModuleDesc>::iterator itMod = modules.begin();
         itMod != modules.end(); itMod++) {
      for (std::list<Arc::PluginDesc>::iterator itPlug = itMod->plugins.begin();
           itPlug != itMod->plugins.end(); itPlug++) {
        std::cout << "  " << itPlug->name << " - " << itPlug->description << std::endl;
      }
    }

    pf.scan(Arc::FinderLoader::GetLibrariesList(), modules);
    Arc::PluginsFactory::FilterByKind("HED:TargetRetriever", modules);
    std::cout << Arc::IString("Types of index and information services which arcmigrate is able collect information from:") << std::endl;
    for (std::list<Arc::ModuleDesc>::iterator itMod = modules.begin();
         itMod != modules.end(); itMod++) {
      for (std::list<Arc::PluginDesc>::iterator itPlug = itMod->plugins.begin();
           itPlug != itMod->plugins.end(); itPlug++) {
        std::cout << "  " << itPlug->name << " - " << itPlug->description << std::endl;
      }
    }

    modules.clear();
    pf.scan(Arc::FinderLoader::GetLibrariesList(), modules);
    Arc::PluginsFactory::FilterByKind("HED:Broker", modules);
    bool isDefaultBrokerLocated = false;
    std::cout << Arc::IString("Brokers available to arcmigrate:") << std::endl;
    for (std::list<Arc::ModuleDesc>::iterator itMod = modules.begin();
         itMod != modules.end(); itMod++) {
      for (std::list<Arc::PluginDesc>::iterator itPlug = itMod->plugins.begin();
           itPlug != itMod->plugins.end(); itPlug++) {
        std::cout << "  " << itPlug->name;
        if (itPlug->name == usercfg.Broker().first) {
          std::cout << " (default)";
          isDefaultBrokerLocated = true;
        }
        std::cout << " - " << itPlug->description << std::endl;
      }
    }

    if (!isDefaultBrokerLocated) {
      logger.msg(Arc::WARNING, "Default broker (%s) is not available. When using arcsub a broker should be specified explicitly (-b option).", usercfg.Broker().first);
    }
    return 0;
  }

  if (!usercfg.ProxyPath().empty() ) {
    Arc::Credential holder(usercfg.ProxyPath(), "", "", "");
    if (holder.GetEndTime() < Arc::Time()){
      std::cout << Arc::IString("Proxy expired. Job submission aborted. Please run 'arcproxy'!") << std::endl;
      return 1;
    }
  }
  else {
    std::cout << Arc::IString("Cannot find any proxy. arcmigrate currently cannot run without a proxy.\n"
                              "  If you have the proxy file in a non-default location,\n"
                              "  please make sure the path is specified in the client configuration file.\n"
                              "  If you don't have a proxy yet, please run 'arcproxy'!") << std::endl;
    return 1;
  }

  if (debug.empty() && !usercfg.Verbosity().empty())
    Arc::Logger::getRootLogger().setThreshold(Arc::string_to_level(usercfg.Verbosity()));

  for (std::list<std::string>::const_iterator it = jobidfiles.begin(); it != jobidfiles.end(); it++) {
    if (!Arc::Job::ReadJobIDsFromFile(*it, jobIDsAndNames)) {
      logger.msg(Arc::WARNING, "Cannot read specified jobid file: %s", *it);
    }
  }

  if (timeout > 0)
    usercfg.Timeout(timeout);

  if (!broker.empty())
    usercfg.Broker(broker);

  if (!joblist.empty() && jobIDsAndNames.empty() && clusters.empty())
    all = true;

  if (jobIDsAndNames.empty() && clusters.empty() && !all) {
    logger.msg(Arc::ERROR, "No jobs given");
    return 1;
  }

  // Removes slashes from end of cluster names, and put cluster to reject into separate list.
  std::list<std::string> rejectClusters;
  for (std::list<std::string>::iterator itC = clusters.begin();
       itC != clusters.end();) {
    if ((*itC)[itC->length()-1] == '/') {
      itC->erase(itC->length()-1);
    }
    if ((*itC)[0] == '-') {
      rejectClusters.push_back(itC->substr(1));
      itC = clusters.erase(itC);
    }
    else {
      ++itC;
    }
  }

  std::list<Arc::Job> jobs;
  Arc::Job::ReadAllJobsFromFile(usercfg.JobListFile(), jobs);
  if (!all) {
    for (std::list<Arc::Job>::iterator itJ = jobs.begin();
         itJ != jobs.end();) {
      if (jobIDsAndNames.empty() && clusters.empty()) {
        // Remove remaing jobs.
        jobs.erase(itJ, jobs.end());
        break;
      }
      std::list<std::string>::iterator itJID = jobIDsAndNames.begin();
      for (;itJID != jobIDsAndNames.end(); ++itJID) {
        if (itJ->IDFromEndpoint.str() == *itJID) {
          break;
        }
      }
      if (itJID != jobIDsAndNames.end()) {
        // Job explicitly specified. Remove id from list so we dont iterate it again.
        jobIDsAndNames.erase(itJID);
        ++itJ;
        continue;
      }

      std::list<std::string>::const_iterator itC = clusters.begin();
      for (; itC != clusters.end(); ++itC) {
        if (itJ->Cluster.str() == *itC ||
            itJ->Cluster.Host() == *itC ||
            itJ->Cluster.Host() + "/" + itJ->Cluster.Path() == *itC ||
            itJ->Cluster.Host() + ":" + Arc::tostring(itJ->Cluster.Port()) == *itC ||
            itJ->Cluster.Host() + ":" + Arc::tostring(itJ->Cluster.Port()) + "/" + itJ->Cluster.Path() == *itC) {
          break;
        }
      }
      if (itC != clusters.end()) {
        // Cluster on which job reside is explicitly specified.
        ++itJ;
        continue;
      }

      // Job is not selected - remove it.
      itJ = jobs.erase(itJ);
    }
  }

  // Filter jobs on rejected clusters.
  for (std::list<std::string>::const_iterator itC = rejectClusters.begin();
       itC != rejectClusters.end(); ++itC) {
    std::list<Arc::Job>::iterator itJ = jobs.begin();
    for (; itJ != jobs.end(); ++itJ) {
      if (itJ->Cluster.str() == *itC ||
          itJ->Cluster.Host() == *itC ||
          itJ->Cluster.Host() + "/" + itJ->Cluster.Path() == *itC ||
          itJ->Cluster.Host() + ":" + Arc::tostring(itJ->Cluster.Port()) == *itC ||
          itJ->Cluster.Host() + ":" + Arc::tostring(itJ->Cluster.Port()) + "/" + itJ->Cluster.Path() == *itC) {
        break;
      }
    }
    if (itJ != jobs.end()) {
      jobs.erase(itJ);
    }
  }

  if (!qlusters.empty() || !indexurls.empty()) {
    usercfg.ClearSelectedServices();
    if (!qlusters.empty()) {
      usercfg.AddServices(qlusters, Arc::COMPUTING);
    }
    if (!indexurls.empty()) {
      usercfg.AddServices(indexurls, Arc::INDEX);
    }
  }

  Arc::JobSupervisor jobmaster(usercfg, jobs);
  if (!jobmaster.JobsFound()) {
    std::cout << Arc::IString("No jobs selected for migration") << std::endl;
    return 0;
  }

  std::list<Arc::Job> migratedJobs;
  std::list<Arc::URL> notmigrated;
  if (jobmaster.Migrate(forcemigration, migratedJobs, notmigrated) && migratedJobs.empty()) {
    std::cout << Arc::IString("No queuing jobs to migrate") << std::endl;
    return 0;
  }

  for (std::list<Arc::Job>::const_iterator it = migratedJobs.begin();
       it != migratedJobs.end(); ++it) {
    std::cout << Arc::IString("Job submitted with jobid: %s", it->IDFromEndpoint.str()) << std::endl;
  }

  if (!migratedJobs.empty() && !Arc::Job::WriteJobsToFile(usercfg.JobListFile(), migratedJobs)) {
    std::cout << Arc::IString("Warning: Failed to lock job list file %s", usercfg.JobListFile()) << std::endl;
    std::cout << Arc::IString("         To recover missing jobs, run arcsync") << std::endl;
  }

  if (!jobidfileout.empty() && !Arc::Job::WriteJobIDsToFile(migratedJobs, jobidfileout)) {
    logger.msg(Arc::WARNING, "Cannot write job IDs of submitted jobs to file (%s)", jobidfileout);
  }

  // Get job IDs of jobs to kill.
  std::list<Arc::URL> jobstobekilled;
  for (std::list<Arc::Job>::const_iterator it = migratedJobs.begin();
       it != migratedJobs.end(); ++it) {
    if (!it->ActivityOldID.empty()) {
      jobstobekilled.push_back(it->ActivityOldID.back());
    }
  }

  std::list<Arc::URL> notkilled;
  std::list<Arc::URL> killedJobs = jobmaster.Cancel(jobstobekilled, notkilled);
  for (std::list<Arc::URL>::const_iterator it = notkilled.begin();
       it != notkilled.end(); ++it) {
    logger.msg(Arc::WARNING, "Migration of job (%s) succeeded, but killing the job failed - it will still appear in the job list", it->str());
  }

  if (!keep) {
    std::list<Arc::URL> notcleaned;
    std::list<Arc::URL> cleanedJobs = jobmaster.Clean(killedJobs, notcleaned);
    for (std::list<Arc::URL>::const_iterator it = notcleaned.begin();
         it != notcleaned.end(); ++it) {
      logger.msg(Arc::WARNING, "Migration of job (%s) succeeded, but cleaning the job failed - it will still appear in the job list", it->str());
    }

    if (!Arc::Job::RemoveJobsFromFile(usercfg.JobListFile(), cleanedJobs)) {
      std::cout << Arc::IString("Warning: Failed to lock job list file %s", usercfg.JobListFile()) << std::endl;
      std::cout << Arc::IString("         Use arcclean to remove non-existing jobs") << std::endl;
    }
  }

  if ((migratedJobs.size() + notmigrated.size()) > 1) {
    std::cout << std::endl << Arc::IString("Job migration summary:") << std::endl;
    std::cout << "-----------------------" << std::endl;
    std::cout << Arc::IString("%d of %d jobs were migrated", migratedJobs.size(), migratedJobs.size() + notmigrated.size()) << std::endl;
    if (!notmigrated.empty()) {
      std::cout << Arc::IString("The following %d were not migrated", notmigrated.size()) << std::endl;
      for (std::list<Arc::URL>::const_iterator it = notmigrated.begin();
           it != notmigrated.end(); ++it) {
        std::cout << it->str() << std::endl;
      }
    }
  }

  return notmigrated.empty();
}
