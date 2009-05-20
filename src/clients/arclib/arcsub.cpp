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
#include <arc/URL.h>
#include <arc/Utils.h>
#include <arc/XMLNode.h>
#include <arc/client/Submitter.h>
#include <arc/client/Sandbox.h>
#include <arc/client/TargetGenerator.h>
#include <arc/client/JobDescription.h>
#include <arc/client/UserConfig.h>
#include <arc/client/ACC.h>
#include <arc/client/Broker.h>
#include <arc/client/ACCLoader.h>

int main(int argc, char **argv) {

  setlocale(LC_ALL, "");

  Arc::Logger logger(Arc::Logger::getRootLogger(), "arcsub");
  Arc::LogStream logcerr(std::cerr);
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
                            "in the format matching the selected cluster."),
                    dumpdescription);

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
                    istring("select broker method (RandomBroker (default), FastestQueueBroker, or custom)"),
                    istring("broker"), broker);

  bool dolocalsandbox = true;
  options.AddOption('n', "dolocalsandbox",
                    istring("store job descriptions in local sandbox."),
                    dolocalsandbox);

  bool version = false;
  options.AddOption('v', "version", istring("print version information"),
                    version);

  std::list<std::string> params = options.Parse(argc, argv);

  Arc::UserConfig usercfg(conffile, joblist);
  if (!usercfg) {
    logger.msg(Arc::ERROR, "Failed configuration initialization");
    return 1;
  }

  if (timeout > 0) {
    usercfg.SetTimeout(timeout);
  }
  
  if (!debug.empty())
    Arc::Logger::getRootLogger().setThreshold(Arc::string_to_level(debug));
  else if (debug.empty() && usercfg.ConfTree()["Debug"]) {
    debug = (std::string)usercfg.ConfTree()["Debug"];
    Arc::Logger::getRootLogger().setThreshold(Arc::string_to_level(debug));
  }

  if (version) {
    std::cout << Arc::IString("%s version %s", "arcsub", VERSION)
              << std::endl;
    return 0;
  }

  // Proxy check
  if (!usercfg.CheckProxy())
    return 1;

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

  Arc::TargetGenerator targen(usercfg, clusters, indexurls);
  targen.GetTargets(0, 1);

  if (targen.FoundTargets().empty()) {
    std::cout << Arc::IString("Job submission aborted because no clusters returned any information") << std::endl;
    return 1;
  }

  std::map<int, std::string> notsubmitted;

  int jobnr = 1;
  std::list<std::string> jobids;

  //prepare loader
  if (broker.empty())
    broker = "RandomBroker";

  Arc::ACCConfig acccfg;
  Arc::NS ns;
  Arc::Config cfg(ns);
  acccfg.MakeConfig(cfg);

  Arc::XMLNode Broker = cfg.NewChild("ArcClientComponent");
  std::string::size_type pos = broker.find(':');
  if (pos != std::string::npos) {
    Broker.NewAttribute("name") = broker.substr(0, pos);
    Broker.NewChild("Arguments") = broker.substr(pos + 1);
  }
  else
    Broker.NewAttribute("name") = broker;
  Broker.NewAttribute("id") = "broker";

  usercfg.ApplySecurity(Broker);
  usercfg.ApplyTimeout(Broker);

  Arc::ACCLoader loader(cfg);
  Arc::Broker *ChosenBroker = dynamic_cast<Arc::Broker*>(loader.getACC("broker"));
  logger.msg(Arc::INFO, "Broker %s loaded", broker);

  for (std::list<Arc::JobDescription>::iterator it =
         jobdescriptionlist.begin(); it != jobdescriptionlist.end();
       it++, jobnr++) {

    ChosenBroker->PreFilterTargets(targen.ModifyFoundTargets(), *it);

    while (true) {
      const Arc::ExecutionTarget* target = ChosenBroker->GetBestTarget();
      if (dumpdescription) {
        std::string flavour;
        std::string jobdesc;
        if (!target)
          flavour = target->GridFlavour;
        else if (!clusters.empty()) {
          Arc::URLListMap clusterselect;
          Arc::URLListMap clusterreject;
          if (!usercfg.ResolveAlias(clusters, clusterselect, clusterreject)) {
            logger.msg(Arc::ERROR, "Failed resolving aliases");
            return 0;
          }
          Arc::URLListMap::iterator it = clusterselect.begin();
          flavour = it->first;
        }
        if (flavour == "ARC1" || flavour == "UNICORE")
          jobdesc = it->UnParse("POSIXJSDL");
        else if (flavour == "ARC0")
          jobdesc = it->UnParse("XRSL");
        else if (flavour == "CREAM")
          jobdesc = it->UnParse("JDL");
        else {
          std::cout << "Cluster requires unknown format:"
                    << flavour << std::endl;
          return 1;
        }
        if (!target)
          std::cout << "Job description to be send to " << target->Cluster.str() << ":" << std::endl;
        else if (!clusters.empty())
          std::cout << "Job description to be send to " << clusters.front() << ":" << std::endl;
        else
          std::cout << "Job description could not be send to any cluster:" << std::endl;
        std::cout << jobdesc << std::endl;
        return 0;
      }

      if (!target) {
        std::cout << Arc::IString("Job submission failed, no more possible targets") << std::endl;
        break;
      }

      Arc::Submitter *submitter = target->GetSubmitter(usercfg);

      //submit the job
      Arc::URL jobid = submitter->Submit(*it, usercfg.JobListFile());
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
