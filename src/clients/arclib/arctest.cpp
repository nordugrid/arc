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
#include <arc/URL.h>
#include <arc/Utils.h>
#include <arc/XMLNode.h>
#include <arc/client/Submitter.h>
#include <arc/client/TargetGenerator.h>
#include <arc/client/JobDescription.h>
#include <arc/UserConfig.h>
#include <arc/client/Broker.h>

static Arc::Logger logger(Arc::Logger::getRootLogger(), "arcsub");

int test(const Arc::UserConfig& usercfg, const int& testid, const std::string& jobidfile);
int dumpjobdescription(const Arc::UserConfig& usercfg, const int& testid);

#ifdef TEST
#define RUNSUB(X) test_arctest_##X
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

  Arc::OptionParser options(istring(" "),
                            istring("The arctest command is used for "
                                    "testing clusters as resources."));

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

  int jobid;
  options.AddOption('J', "job",
                    istring("submit test job given by the number"),
                    istring("int"),
                    jobid);

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
  options.AddOption('D', "dryrun", istring("add dryrun option if available"),
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
                    istring("select broker method (Random (default), FastestQueue, or custom)"),
                    istring("broker"), broker);

  bool version = false;
  options.AddOption('v', "version", istring("print version information"),
                    version);

  std::list<std::string> params = options.Parse(argc, argv);

  if (version) {
    std::cout << Arc::IString("%s version %s", "arctest", VERSION)
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

  if (!clusters.empty() || !indexurls.empty())
    usercfg.ClearSelectedServices();

  if (!clusters.empty())
    usercfg.AddServices(clusters, Arc::COMPUTING);

  if (!indexurls.empty())
    usercfg.AddServices(indexurls, Arc::INDEX);

  int retval = 0;
  if (dumpdescription) {
     retval += dumpjobdescription(usercfg, jobid);
  }

  retval += test(usercfg, jobid, jobidfile);
  return retval;
}

void printjobid(const std::string& jobid, const std::string& jobidfile) {
  if (!jobidfile.empty())
    if (!Arc::Job::WriteJobIDToFile(jobid, jobidfile))
      logger.msg(Arc::WARNING, "Cannot write jobid (%s) to file (%s)", jobid, jobidfile);
  std::cout << Arc::IString("Test submitted with jobid: %s", jobid) << std::endl;
}

int test(const Arc::UserConfig& usercfg, const int& testid, const std::string& jobidfile) {
  int retval = 0;

  Arc::TargetGenerator targen(usercfg);
  targen.RetrieveExecutionTargets();

  if (targen.GetExecutionTargets().empty()) {
    std::cout << Arc::IString("Test aborted because no resource returned any information") << std::endl;
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

  bool descriptionSubmitted = false;
  submittedJobs.push_back(Arc::Job());
  if (ChosenBroker->Test(targen.GetExecutionTargets(), testid, submittedJobs.back())) {
    printjobid(submittedJobs.back().JobID.str(), jobidfile);
    descriptionSubmitted = true;
  }

  if (!descriptionSubmitted) {
    std::cout << Arc::IString("Test failed, no more possible targets") << std::endl;
    submittedJobs.pop_back();
  }

  if (!Arc::Job::WriteJobsToFile(usercfg.JobListFile(), submittedJobs)) {
    std::cout << Arc::IString("Warning: Failed to lock job list file %s", usercfg.JobListFile())
              << std::endl;
    std::cout << Arc::IString("To recover missing jobs, run arcsync") << std::endl;
  }

  return retval;
}

int dumpjobdescription(const Arc::UserConfig& usercfg, const int& testid) {
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

  ChosenBroker->UseAllTargets(targen.GetExecutionTargets());

  while (true) {
    const Arc::ExecutionTarget* target = ChosenBroker->GetBestTarget();

    if (!target) {
      std::cout << Arc::IString("Unable to print job description: No target found.") << std::endl;
      break;
    }

    Arc::Submitter *submitter = target->GetSubmitter(usercfg);

    Arc::JobDescription jobdescdump;

    // Return if test jobdescription is not defined
    if (!(submitter->GetTestJob(testid, jobdescdump))) {
      logger.msg(Arc::ERROR, "Illegal testjob-id given");
      return false;
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

  return retval;
}
