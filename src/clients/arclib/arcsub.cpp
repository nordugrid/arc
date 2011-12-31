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
#include <arc/StringConv.h>
#include <arc/URL.h>
#include <arc/UserConfig.h>
#include <arc/Utils.h>
#include <arc/XMLNode.h>
#include <arc/client/Broker.h>
#include <arc/client/JobDescription.h>
#include <arc/client/TargetGenerator.h>
#include <arc/client/Submitter.h>

#include "utils.h"

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

  ClientOptions opt(ClientOptions::CO_SUB,
                    istring("[filename ...]"),
                    istring("The arcsub command is used for "
                            "submitting jobs to Grid enabled "
                            "computing\nresources."));

  std::list<std::string> params = opt.Parse(argc, argv);

  if (opt.showversion) {
    std::cout << Arc::IString("%s version %s", "arcsub", VERSION)
              << std::endl;
    return 0;
  }

  // If debug is specified as argument, it should be set before loading the configuration.
  if (!opt.debug.empty())
    Arc::Logger::getRootLogger().setThreshold(Arc::string_to_level(opt.debug));

  Arc::UserConfig usercfg(opt.conffile, opt.joblist);
  if (!usercfg) {
    logger.msg(Arc::ERROR, "Failed configuration initialization");
    return 1;
  }

  if (opt.show_plugins) {
    std::list<std::string> types;
    types.push_back("HED:Submitter");
    types.push_back("HED:TargetRetriever");
    types.push_back("HED:JobDescriptionParser");
    types.push_back("HED:Broker");
    showplugins("arcsub", types, logger, usercfg.Broker().first);
    return 0;
  }

  if (!checkproxy(usercfg)) {
    return 1;
  }

  if (opt.debug.empty() && !usercfg.Verbosity().empty())
    Arc::Logger::getRootLogger().setThreshold(Arc::string_to_level(usercfg.Verbosity()));

  if (opt.timeout > 0)
    usercfg.Timeout(opt.timeout);

  if (!opt.broker.empty())
    usercfg.Broker(opt.broker);

  if (!opt.clusters.empty() || !opt.indexurls.empty())
    usercfg.ClearSelectedServices();

  if (!opt.clusters.empty())
    usercfg.AddServices(opt.clusters, Arc::COMPUTING);

  if (!opt.indexurls.empty())
    usercfg.AddServices(opt.indexurls, Arc::INDEX);

  opt.jobdescriptionfiles.insert(opt.jobdescriptionfiles.end(),
                                 params.begin(), params.end());

  if (opt.jobdescriptionfiles.empty() && opt.jobdescriptionstrings.empty()) {
    logger.msg(Arc::ERROR, "No job description input specified");
    return 1;
  }

  std::list<Arc::JobDescription> jobdescriptionlist;

  // Loop over input job description files
  for (std::list<std::string>::iterator it = opt.jobdescriptionfiles.begin();
       it != opt.jobdescriptionfiles.end(); it++) {

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
        itJ->Application.DryRun = opt.dryrun;
        for (std::list<Arc::JobDescription>::iterator itJAlt = itJ->GetAlternatives().begin();
             itJAlt != itJ->GetAlternatives().end(); itJAlt++) {
          itJAlt->Application.DryRun = opt.dryrun;
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
  for (std::list<std::string>::iterator it = opt.jobdescriptionstrings.begin();
       it != opt.jobdescriptionstrings.end(); it++) {

    std::list<Arc::JobDescription> jobdescs;
    if (Arc::JobDescription::Parse(*it, jobdescs)) {
      for (std::list<Arc::JobDescription>::iterator itJ = jobdescs.begin();
           itJ != jobdescs.end(); itJ++) {
        itJ->Application.DryRun = opt.dryrun;
        for (std::list<Arc::JobDescription>::iterator itJAlt = itJ->GetAlternatives().begin();
             itJAlt != itJ->GetAlternatives().end(); itJAlt++) {
          itJAlt->Application.DryRun = opt.dryrun;
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

  if (opt.dumpdescription) {
    return dumpjobdescription(usercfg, jobdescriptionlist);
  }

  return submit(usercfg, jobdescriptionlist, opt.jobidoutfile);
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

      if (!jobdescdump.Prepare(*target)) {
        std::cout << Arc::IString("Unable to prepare job description according to needs of the target resource.") << std::endl;
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
      else if (target->GridFlavour == "EMIES") {
        jobdesclang = "emies:adl";
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
