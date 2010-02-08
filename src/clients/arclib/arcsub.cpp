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

int main(int argc, char **argv) {

  setlocale(LC_ALL, "");

  Arc::Logger logger(Arc::Logger::getRootLogger(), "arcsub");
  Arc::LogStream logcerr(std::cerr);
  logcerr.setFormat(Arc::ShortFormat);
  Arc::Logger::getRootLogger().addDestination(logcerr);
  Arc::Logger::getRootLogger().setThreshold(Arc::WARNING);

  Arc::ArcLocation::Init(argv[0]);

  Arc::OptionParser options(istring("[filename ...]"),
                            istring("The arcsub command is used for "
                                    "submitting jobs to grid enabled "
                                    "computing\nresources."),
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
                    istring("file where the jobs will be stored"),
                    istring("filename"),
                    joblist);

  /*
     bool dryrun = false;
     options.AddOption('D', "dryrun", istring("add dryrun option"),
                    dryrun);

   */
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

/**
 * Sandboxing is always done atm. Maybe there should be a switch to turn it off? 'n' "nolocalsandbox"
  bool dolocalsandbox = true;
  options.AddOption('n', "dolocalsandbox",
                    istring("store job descriptions in local sandbox."),
                    dolocalsandbox);
*/

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

  if (timeout > 0)
    usercfg.Timeout(timeout);

  if (!broker.empty())
    usercfg.Broker(broker);

  if (version) {
    std::cout << Arc::IString("%s version %s", "arcsub", VERSION)
              << std::endl;
    return 0;
  }

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
    Arc::JobDescription jobdesc;
    jobdesc.Parse((std::string)buffer);

    if (jobdesc)
      jobdescriptionlist.push_back(jobdesc);
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

    Arc::JobDescription jobdesc;

    jobdesc.Parse(*it);

    if (jobdesc)
      jobdescriptionlist.push_back(jobdesc);
    else {
      logger.msg(Arc::ERROR, "Invalid JobDescription:");
      std::cout << *it << std::endl;
      return 1;
    }
  }

  Arc::TargetGenerator targen(usercfg);
  targen.GetTargets(0, 1);

  if (targen.FoundTargets().empty()) {
    std::cout << Arc::IString("Job submission aborted because no clusters returned any information") << std::endl;
    return 1;
  }

  std::map<int, std::string> notsubmitted;

  int jobnr = 1;
  std::list<std::string> jobids;

  Arc::BrokerLoader loader;
  Arc::Broker *ChosenBroker = loader.load(usercfg.Broker().first, usercfg);
  if (!ChosenBroker) {
    logger.msg(Arc::ERROR, "Unable to load broker %s", usercfg.Broker().first);
    return 1;
  }
  logger.msg(Arc::INFO, "Broker %s loaded", usercfg.Broker().first);

  for (std::list<Arc::JobDescription>::iterator it =
         jobdescriptionlist.begin(); it != jobdescriptionlist.end();
       it++, jobnr++) {

    ChosenBroker->PreFilterTargets(targen.ModifyFoundTargets(), *it);

    while (true) {
      const Arc::ExecutionTarget* target = ChosenBroker->GetBestTarget();

      if (!target) {
        if (dumpdescription) {
          std::cout << Arc::IString("Unable to print job description: No target found.") << std::endl;
          return 1;
        }
        std::cout << Arc::IString("Job submission failed, no more possible targets") << std::endl;
        break;
      }

      Arc::Submitter *submitter = target->GetSubmitter(usercfg);

      if (dumpdescription) {
        Arc::JobDescription jobdescdump(*it);
        if (!submitter->ModifyJobDescription(jobdescdump, *target)) {
          std::cout << "Unable to modify job description according to needs for target cluster." << std::endl;
          return 1;
        }

        std::string jobdesclang = "ARCJSDL";
        if (target->GridFlavour == "ARC0")
          jobdesclang = "XRSL";
        else if (target->GridFlavour == "CREAM")
          jobdesclang = "JDL";
        const std::string jobdesc = jobdescdump.UnParse(jobdesclang);
        if (jobdesc.empty()) {
          std::cout << Arc::IString("An error occurred during the generation of the job description output.") << std::endl;
          return 1;
        }

        std::cout << Arc::IString("Job description to be send to ") << target->Cluster.str() << ":" << std::endl;
        std::cout << jobdesc << std::endl;
        return 0;
      }


      //submit the job
      Arc::URL jobid = submitter->Submit(*it, *target);
      if (!jobid) {
        std::cout << Arc::IString("Submission to %s failed, trying next target", target->url.str()) << std::endl;
        continue;
      }

      ChosenBroker->RegisterJobsubmission();
      std::cout << Arc::IString("Job submitted with jobid: %s", jobid.str())
                << std::endl;

      break;
    } //end loop over all possible targets
  } //end loop over all job descriptions

  if (jobdescriptionlist.size() > 1) {
    std::cout << std::endl << Arc::IString("Job submission summary:")
              << std::endl;
    std::cout << "-----------------------" << std::endl;
    std::cout << Arc::IString("%d of %d jobs were submitted",
                              jobdescriptionlist.size() - notsubmitted.size(),
                              jobdescriptionlist.size()) << std::endl;
    if (notsubmitted.size())
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

  return 0;
}
