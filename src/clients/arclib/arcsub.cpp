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
#include <arc/URL.h>
#include <arc/UserConfig.h>
#include <arc/Utils.h>
#include <arc/XMLNode.h>
#include <arc/client/Broker.h>
#include <arc/client/JobDescription.h>
#include <arc/client/TargetGenerator.h>
#include <arc/client/Submitter.h>
#include <arc/loader/Plugin.h>
#include <arc/loader/FinderLoader.h>
#include <arc/credential/Credential.h>

static Arc::Logger logger(Arc::Logger::getRootLogger(), "arcsub");

int submit(const Arc::UserConfig& usercfg, const std::list<Arc::JobDescription>& jobdescriptionlist, const std::string& jobidfile);
int dumpjobdescription(const Arc::UserConfig& usercfg, const std::list<Arc::JobDescription>& jobdescriptionlist);

#ifdef TEST
#define RUNSUB(X) test_arcsub_##X
#else
#define RUNSUB(X) X
#endif
int RUNSUB(main)(int argc, char **argv) {

  setlocale(LC_ALL, "");

  Arc::LogStream logcerr(std::cerr);
  logcerr.setFormat(Arc::ShortFormat);
  Arc::Logger::getRootLogger().addDestination(logcerr);
  Arc::Logger::getRootLogger().setThreshold(Arc::WARNING);

  Arc::ArcLocation::Init(argv[0]);

  Arc::OptionParser options(istring("[filename ...]"),
                            istring("The arcsub command is used for "
                                    "submitting jobs to Grid enabled "
                                    "computing\nresources."));

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

  std::list<std::string> jobdescriptionstrings;
  options.AddOption('e', "jobdescrstring",
                    istring("jobdescription string describing the job to "
                            "be submitted"),
                    istring("string"),
                    jobdescriptionstrings);

  std::list<std::string> jobdescriptionfiles;
  options.AddOption('f', "jobdescrfile",
                    istring("jobdescription file describing the job to "
                            "be submitted"),
                    istring("string"),
                    jobdescriptionfiles);

  std::string joblist;
  options.AddOption('j', "joblist",
                    istring("the file storing information about active jobs (default ~/.arc/jobs.xml)"),
                    istring("filename"),
                    joblist);

  std::string jobidfile;
  options.AddOption('o', "jobids-to-file",
                    istring("the IDs of the submitted jobs will be appended to this file"),
                    istring("filename"),
                    jobidfile);

  bool dryrun = false;
  options.AddOption('D', "dryrun", istring("submit jobs as dry run (no submission to batch system)"),
                    dryrun);

  bool dumpdescription = false;
  options.AddOption('x', "dumpdescription",
                    istring("do not submit - dump job description "
                            "in the language accepted by the target"),
                    dumpdescription);

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
                    istring("select broker method (list available brokers with --listplugins flag)"),
                    istring("broker"), broker);

/**
 * Sandboxing is always done atm. Maybe there should be a switch to turn it off? 'n' "nolocalsandbox"
  bool dolocalsandbox = true;
  options.AddOption('n', "dolocalsandbox",
                    istring("store job descriptions in local sandbox."),
                    dolocalsandbox);
*/

  bool show_plugins = false;
  options.AddOption('P', "listplugins", istring("list the available plugins"),
                    show_plugins);

  bool version = false;
  options.AddOption('v', "version", istring("print version information"),
                    version);

  std::list<std::string> params = options.Parse(argc, argv);
  
  if (version) {
    std::cout << Arc::IString("%s version %s", "arcsub", VERSION)
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
    std::cout << Arc::IString("Types of execution services arcsub is able to submit jobs to:") << std::endl;
    for (std::list<Arc::ModuleDesc>::iterator itMod = modules.begin();
         itMod != modules.end(); itMod++) {
      for (std::list<Arc::PluginDesc>::iterator itPlug = itMod->plugins.begin();
           itPlug != itMod->plugins.end(); itPlug++) {
        std::cout << "  " << itPlug->name << " - " << itPlug->description << std::endl;
      }
    }
    
    pf.scan(Arc::FinderLoader::GetLibrariesList(), modules);
    Arc::PluginsFactory::FilterByKind("HED:TargetRetriever", modules);
    std::cout << Arc::IString("Types of index and information services which arcsub is able collect information from:") << std::endl;
    for (std::list<Arc::ModuleDesc>::iterator itMod = modules.begin();
         itMod != modules.end(); itMod++) {
      for (std::list<Arc::PluginDesc>::iterator itPlug = itMod->plugins.begin();
           itPlug != itMod->plugins.end(); itPlug++) {
        std::cout << "  " << itPlug->name << " - " << itPlug->description << std::endl;
      }
    }
    
    pf.scan(Arc::FinderLoader::GetLibrariesList(), modules);
    Arc::PluginsFactory::FilterByKind("HED:JobDescriptionParser", modules);
    std::cout << Arc::IString("Job description languages supported by arcsub:") << std::endl;
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
    std::cout << Arc::IString("Brokers available to arcsub:") << std::endl;
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
    std::cout << Arc::IString("Cannot find any proxy. arcsub currently cannot run without a proxy.\n"
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

  if (!clusters.empty() || !indexurls.empty())
    usercfg.ClearSelectedServices();

  if (!clusters.empty())
    usercfg.AddServices(clusters, Arc::COMPUTING);

  if (!indexurls.empty())
    usercfg.AddServices(indexurls, Arc::INDEX);

  jobdescriptionfiles.insert(jobdescriptionfiles.end(),
                             params.begin(), params.end());

  if (jobdescriptionfiles.empty() && jobdescriptionstrings.empty()) {
    logger.msg(Arc::ERROR, "No job description input specified");
    return 1;
  }

  std::list<Arc::JobDescription> jobdescriptionlist;

  //Loop over input job description files
  for (std::list<std::string>::iterator it = jobdescriptionfiles.begin();
       it != jobdescriptionfiles.end(); it++) {

    std::ifstream descriptionfile(it->c_str());

    if (!descriptionfile) {
      logger.msg(Arc::ERROR, "Can not open job description file: %s", *it);
      return 1;
    }

    descriptionfile.seekg(0, std::ios::end);
    std::streamsize length = descriptionfile.tellg();
    descriptionfile.seekg(0, std::ios::beg);

    char *buffer = new char[length + 1];
    descriptionfile.read(buffer, length);
    descriptionfile.close();

    buffer[length] = '\0';
    std::list<Arc::JobDescription> jobdescs;
    if (Arc::JobDescription::Parse((std::string)buffer, jobdescs)) {
      for (std::list<Arc::JobDescription>::iterator itJ = jobdescs.begin();
           itJ != jobdescs.end(); itJ++) {
        itJ->Application.DryRun = dryrun;
        for (std::list<Arc::JobDescription>::iterator itJAlt = itJ->GetAlternatives().begin();
             itJAlt != itJ->GetAlternatives().end(); itJAlt++) {
          itJAlt->Application.DryRun = dryrun;
        }
      }

      jobdescriptionlist.insert(jobdescriptionlist.end(), jobdescs.begin(), jobdescs.end());
    }
    else {
      logger.msg(Arc::ERROR, "Invalid JobDescription:");
      std::cout << buffer << std::endl;
      delete[] buffer;
      return 1;
    }
    delete[] buffer;
  }

  //Loop over job description input strings
  for (std::list<std::string>::iterator it = jobdescriptionstrings.begin();
       it != jobdescriptionstrings.end(); it++) {

    std::list<Arc::JobDescription> jobdescs;
    if (Arc::JobDescription::Parse(*it, jobdescs)) {
      for (std::list<Arc::JobDescription>::iterator itJ = jobdescs.begin();
           itJ != jobdescs.end(); itJ++) {
        itJ->Application.DryRun = dryrun;
        for (std::list<Arc::JobDescription>::iterator itJAlt = itJ->GetAlternatives().begin();
             itJAlt != itJ->GetAlternatives().end(); itJAlt++) {
          itJAlt->Application.DryRun = dryrun;
        }
      }

      jobdescriptionlist.insert(jobdescriptionlist.end(), jobdescs.begin(), jobdescs.end());
    }
    else {
      logger.msg(Arc::ERROR, "Invalid JobDescription:");
      std::cout << *it << std::endl;
      return 1;
    }
  }

  if (dumpdescription) {
    return dumpjobdescription(usercfg, jobdescriptionlist);
  }

  return submit(usercfg, jobdescriptionlist, jobidfile);
}

void printjobid(const std::string& jobid, const std::string& jobidfile) {
  if (!jobidfile.empty())
    if (!Arc::Job::WriteJobIDToFile(jobid, jobidfile))
      logger.msg(Arc::WARNING, "Cannot write jobid (%s) to file (%s)", jobid, jobidfile);
  std::cout << Arc::IString("Job submitted with jobid: %s", jobid) << std::endl;
}

int submit(const Arc::UserConfig& usercfg, const std::list<Arc::JobDescription>& jobdescriptionlist, const std::string& jobidfile) {
  int retval = 0;
  
  Arc::TargetGenerator targen(usercfg);
  targen.RetrieveExecutionTargets();

  if (targen.GetExecutionTargets().empty()) {
    std::cout << Arc::IString("Job submission aborted because no resource returned any information") << std::endl;
    return 1;
  }

  Arc::BrokerLoader loader;
  Arc::Broker *ChosenBroker = loader.load(usercfg.Broker().first, usercfg);
  if (!ChosenBroker) {
    logger.msg(Arc::ERROR, "Unable to load broker %s", usercfg.Broker().first);
    return 1;
  }
  logger.msg(Arc::INFO, "Broker %s loaded", usercfg.Broker().first);

  int jobnr = 1;
  std::list<std::string> jobids;
  std::list<Arc::Job> submittedJobs;
  std::map<int, std::string> notsubmitted;

  for (std::list<Arc::JobDescription>::const_iterator it =
         jobdescriptionlist.begin(); it != jobdescriptionlist.end();
       it++, jobnr++) {
    bool descriptionSubmitted = false;
    submittedJobs.push_back(Arc::Job());
    if (ChosenBroker->Submit(targen.GetExecutionTargets(), *it, submittedJobs.back())) {
      printjobid(submittedJobs.back().JobID.fullstr(), jobidfile);
      descriptionSubmitted = true;
    }
    else if (it->HasAlternatives()) {
      // TODO: Deal with alternative job descriptions more effective.
      for (std::list<Arc::JobDescription>::const_iterator itAlt = it->GetAlternatives().begin();
           itAlt != it->GetAlternatives().end(); itAlt++) {
        if (ChosenBroker->Submit(targen.GetExecutionTargets(), *itAlt, submittedJobs.back())) {
          printjobid(submittedJobs.back().JobID.fullstr(), jobidfile);
          descriptionSubmitted = true;
          break;
        }
      }
    }

    if (!descriptionSubmitted) {
      std::cout << Arc::IString("Job submission failed, no more possible targets") << std::endl;
      submittedJobs.pop_back();
    }
  } //end loop over all job descriptions

  if (!Arc::Job::WriteJobsToFile(usercfg.JobListFile(), submittedJobs)) {
    std::cout << Arc::IString("Warning: Failed to lock job list file %s", usercfg.JobListFile())
              << std::endl;
    std::cout << Arc::IString("To recover missing jobs, run arcsync") << std::endl;
  }

  if (jobdescriptionlist.size() > 1) {
    std::cout << std::endl << Arc::IString("Job submission summary:")
              << std::endl;
    std::cout << "-----------------------" << std::endl;
    std::cout << Arc::IString("%d of %d jobs were submitted",
                              submittedJobs.size(),
                              jobdescriptionlist.size()) << std::endl;
    if (!notsubmitted.empty())
      std::cout << Arc::IString("The following %d were not submitted",
                                notsubmitted.size()) << std::endl;
    /*
       std::map<int, std::string>::iterator it;
       for (it = notsubmitted.begin(); it != notsubmitted.end(); it++) {
       std::cout << _("Job nr.") << " " << it->first;
       if (it->second.size()>0) std::cout << ": " << it->second;
       std::cout << std::endl;
       }
     */
  }
  return retval;
}

int dumpjobdescription(const Arc::UserConfig& usercfg, const std::list<Arc::JobDescription>& jobdescriptionlist) {
  int retval = 0;

  Arc::TargetGenerator targen(usercfg);
  targen.RetrieveExecutionTargets();

  if (targen.GetExecutionTargets().empty()) {
    std::cout << Arc::IString("Dumping job description aborted because no resource returned any information") << std::endl;
    return 1;
  }

  Arc::BrokerLoader loader;
  Arc::Broker *ChosenBroker = loader.load(usercfg.Broker().first, usercfg);
  if (!ChosenBroker) {
    logger.msg(Arc::ERROR, "Dumping job description aborted: Unable to load broker %s", usercfg.Broker().first);
    return 1;
  }
  logger.msg(Arc::INFO, "Broker %s loaded", usercfg.Broker().first);

  for (std::list<Arc::JobDescription>::const_iterator it = jobdescriptionlist.begin();
       it != jobdescriptionlist.end(); it++) {
    Arc::JobDescription jobdescdump(*it);
    ChosenBroker->PreFilterTargets(targen.GetExecutionTargets(), *it);
    ChosenBroker->Sort();

    if (ChosenBroker->EndOfList() && it->HasAlternatives()) {
      for (std::list<Arc::JobDescription>::const_iterator itAlt = it->GetAlternatives().begin();
           itAlt != it->GetAlternatives().end(); itAlt++) {
        ChosenBroker->PreFilterTargets(targen.GetExecutionTargets(), *itAlt);
        ChosenBroker->Sort();
        if (!ChosenBroker->EndOfList()) {
          jobdescdump = *itAlt;
          break;
        }
      }
    }

    for (const Arc::ExecutionTarget*& target = ChosenBroker->GetReference();
         !ChosenBroker->EndOfList(); ChosenBroker->Advance()) {
      Arc::Submitter *submitter = target->GetSubmitter(usercfg);

      if (!submitter->ModifyJobDescription(jobdescdump, *target)) {
        std::cout << Arc::IString("Unable to modify job description according to needs of the target resource.") << std::endl;
        retval = 1;
        break;
      }

      std::string jobdesclang = "nordugrid:jsdl";
      if (target->GridFlavour == "ARC0") {
        jobdesclang = "nordugrid:xrsl";
      }
      else if (target->GridFlavour == "CREAM") {
        jobdesclang = "egee:jdl";
      }
      std::string jobdesc;
      if (!jobdescdump.UnParse(jobdesc, jobdesclang)) {
        std::cout << Arc::IString("An error occurred during the generation of the job description output.") << std::endl;
        retval = 1;
        break;
      }

      std::cout << Arc::IString("Job description to be sent to %s:", target->Cluster.str()) << std::endl;
      std::cout << jobdesc << std::endl;
      break;
    } //end loop over all possible targets

    if (ChosenBroker->EndOfList()) {
      std::cout << Arc::IString("Unable to print job description: No matching target found.") << std::endl;
      break;
    }
  } //end loop over all job descriptions

  return retval;
}
