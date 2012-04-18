// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <list>
#include <string>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <arc/ArcConfig.h>
#include <arc/ArcLocation.h>
#include <arc/IString.h>
#include <arc/Logger.h>
#include <arc/StringConv.h>
#include <arc/URL.h>
#include <arc/UserConfig.h>
#include <arc/Utils.h>
#include <arc/client/Broker.h>
#include <arc/client/ComputingServiceRetriever.h>
#include <arc/client/Job.h>
#include <arc/client/JobDescription.h>
#include <arc/client/SubmitterPlugin.h>

#include "utils.h"

static Arc::Logger logger(Arc::Logger::getRootLogger(), "arcsub");

int submit(const Arc::UserConfig& usercfg, const std::list<Arc::JobDescription>& jobdescriptionlist, const std::list<Arc::Endpoint>& services, const std::list<std::string> rejectedURLs, const std::string& jobidfile);
int dumpjobdescription(const Arc::UserConfig& usercfg, const std::list<Arc::JobDescription>& jobdescriptionlist, const std::list<Arc::Endpoint>& services, const std::list<std::string> rejectedURLs);

int RUNMAIN(arcsub)(int argc, char **argv) {

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
    types.push_back("HED:SubmitterPlugin");
    types.push_back("HED:ServiceEndpointRetrieverPlugin");
    types.push_back("HED:TargetInformationRetrieverPlugin");
    types.push_back("HED:JobDescriptionParser");
    types.push_back("HED:BrokerPlugin");
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

  std::list<Arc::Endpoint> services = getServicesFromUserConfigAndCommandLine(usercfg, opt.indexurls, opt.clusters, opt.requestedSubmissionInterfaceName);

  std::list<std::string> rejectedURLs = getRejectedURLsFromUserConfigAndCommandLine(usercfg, opt.rejectedurls);

  if (opt.dumpdescription) {
    return dumpjobdescription(usercfg, jobdescriptionlist, services, rejectedURLs);
  }

  return submit(usercfg, jobdescriptionlist, services, rejectedURLs, opt.jobidoutfile);
}

void printjobid(const std::string& jobid, const std::string& jobidfile) {
  if (!jobidfile.empty())
    if (!Arc::Job::WriteJobIDToFile(jobid, jobidfile))
      logger.msg(Arc::WARNING, "Cannot write jobid (%s) to file (%s)", jobid, jobidfile);
  std::cout << Arc::IString("Job submitted with jobid: %s", jobid) << std::endl;
}

int submit(const Arc::UserConfig& usercfg, const std::list<Arc::JobDescription>& jobdescriptionlist, const std::list<Arc::Endpoint>& services, const std::list<std::string> rejectedURLs, const std::string& jobidfile) {
  int retval = 0;

  std::list<std::string> preferredInterfaceNames;
  if (usercfg.InfoInterface().empty()) {
    preferredInterfaceNames.push_back("org.nordugrid.ldapglue2");
    preferredInterfaceNames.push_back("org.ogf.emies");
  } else {
    preferredInterfaceNames.push_back(usercfg.InfoInterface());
  }

  
  Arc::ComputingServiceRetriever csr(usercfg, services, rejectedURLs, preferredInterfaceNames);
  csr.wait();

  if (csr.empty()) {
    std::cout << Arc::IString("Job submission aborted because no resource returned any information") << std::endl;
    return 1;
  }

  Arc::Broker broker(usercfg, usercfg.Broker().first);
  if (!broker.isValid()) {
    logger.msg(Arc::ERROR, "Unable to load broker %s", usercfg.Broker().first);
    return 1;
  }
  logger.msg(Arc::INFO, "Broker %s loaded", usercfg.Broker().first);

  int jobnr = 1;
  std::list<std::string> jobids;
  std::list<Arc::Job> submittedJobs;
  std::map<int, std::string> notsubmitted;

  for (std::list<Arc::JobDescription>::const_iterator itJ =
         jobdescriptionlist.begin(); itJ != jobdescriptionlist.end();
       ++itJ, ++jobnr) {
    bool descriptionSubmitted = false;
    submittedJobs.push_back(Arc::Job());
    broker.set(*itJ);
    Arc::ExecutionTargetSet etSet(broker, csr);
    for (Arc::ExecutionTargetSet::iterator itET = etSet.begin(); itET != etSet.end(); ++itET) {
      if (itET->Submit(usercfg, *itJ, submittedJobs.back())) {
        printjobid(submittedJobs.back().JobID.fullstr(), jobidfile);
        descriptionSubmitted = true;
        itET->RegisterJobSubmission(*itJ);
        break;
      }
    }
    if (!descriptionSubmitted && itJ->HasAlternatives()) {
      // TODO: Deal with alternative job descriptions more effective.
      for (std::list<Arc::JobDescription>::const_iterator itJAlt = itJ->GetAlternatives().begin();
           itJAlt != itJ->GetAlternatives().end(); ++itJAlt) {
        broker.set(*itJAlt);
        Arc::ExecutionTargetSet etSetAlt(broker, csr);
        for (Arc::ExecutionTargetSet::iterator itET = etSetAlt.begin(); itET != etSetAlt.end(); ++itET) {
          if (itET->Submit(usercfg, *itJAlt, submittedJobs.back())) {
            printjobid(submittedJobs.back().JobID.fullstr(), jobidfile);
            descriptionSubmitted = true;
            itET->RegisterJobSubmission(*itJAlt);
            break;
          }
        }
        if (descriptionSubmitted) {
          break;
        }
      }
    }

    if (!descriptionSubmitted) {
      std::cout << Arc::IString("Job submission failed, no more possible targets") << std::endl;
      submittedJobs.pop_back();
      notsubmitted[jobnr] = itJ->Identification.JobName;
      retval = 1;
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
    if (!notsubmitted.empty()) {
      std::cout << Arc::IString("The following %d were not submitted",
                                notsubmitted.size()) << std::endl;
      std::map<int, std::string>::const_iterator it = notsubmitted.begin();
      for (; it != notsubmitted.end(); ++it) {
        std::cout << Arc::IString("Job nr.") << " " << it->first;
        if (!it->second.empty()) {
          std::cout << ": " << it->second;
        }
        std::cout << std::endl;
      }
    }
  }
  return retval;
}

int dumpjobdescription(const Arc::UserConfig& usercfg, const std::list<Arc::JobDescription>& jobdescriptionlist, const std::list<Arc::Endpoint>& services, const std::list<std::string> rejectedURLs) {
  int retval = 0;

  std::list<std::string> preferredInterfaceNames;
  if (usercfg.InfoInterface().empty()) {
    preferredInterfaceNames.push_back("org.nordugrid.ldapglue2");
    preferredInterfaceNames.push_back("org.ogf.emies");
  } else {
    preferredInterfaceNames.push_back(usercfg.InfoInterface());
  }

  Arc::ComputingServiceRetriever csr(usercfg, services, rejectedURLs, preferredInterfaceNames);
  csr.wait();

  if (csr.empty()) {
    std::cout << Arc::IString("Dumping job description aborted because no resource returned any information") << std::endl;
    return 1;
  }

  Arc::Broker broker(usercfg, usercfg.Broker().first);
  if (!broker.isValid()) {
    logger.msg(Arc::ERROR, "Dumping job description aborted: Unable to load broker %s", usercfg.Broker().first);
    return 1;
  }
  logger.msg(Arc::INFO, "Broker %s loaded", usercfg.Broker().first);

  for (std::list<Arc::JobDescription>::const_iterator itJ = jobdescriptionlist.begin();
       itJ != jobdescriptionlist.end(); ++itJ) {
    Arc::JobDescription jobdescdump(*itJ);
  
    broker.set(*itJ);
    Arc::ExecutionTargetSet ets(broker, csr);
    if (ets.empty() && itJ->HasAlternatives()) {
      for (std::list<Arc::JobDescription>::const_iterator itAlt = itJ->GetAlternatives().begin();
           itAlt != itJ->GetAlternatives().end(); ++itAlt) {
        ets.set(*itAlt);
        ets.addEntities(csr);
        if (ets.empty()) {
          jobdescdump = *itAlt;
          break;
        }
      }
    }
    
    Arc::ExecutionTargetSet::iterator itET = ets.begin();
    for (; itET != ets.end(); ++itET) {
      if (!jobdescdump.Prepare(*itET)) {
        logger.msg(Arc::INFO, "Unable to prepare job description according to needs of the target resource (%s).", itET->ComputingEndpoint->URLString); 
        continue;
      }

      std::string jobdesclang = "nordugrid:jsdl";
      if (itET->ComputingEndpoint->InterfaceName == "org.nordugrid.gridftpjob") {
        jobdesclang = "nordugrid:xrsl";
      }
      else if (itET->ComputingEndpoint->InterfaceName == "org.glite.cream") {
        jobdesclang = "egee:jdl";
      }
      else if (itET->ComputingEndpoint->InterfaceName == "org.ogf.emies") {
        jobdesclang = "emies:adl";
      }
      std::string jobdesc;
      if (!jobdescdump.UnParse(jobdesc, jobdesclang)) {
        logger.msg(Arc::INFO, "An error occurred during the generation of job description to be sent to %s", itET->ComputingEndpoint->URLString); 
        continue;
      }

      std::cout << Arc::IString("Job description to be sent to %s:", itET->ComputingService->Cluster.str()) << std::endl;
      std::cout << jobdesc << std::endl;
      break;
    }

    if (ets.empty()) {
      std::cout << Arc::IString("Unable to print job description: No matching target found.") << std::endl;
      retval = 1;
    } else if (itET == ets.end()) {
      std::cout << Arc::IString("Unable to prepare job description according to needs of the target resource.") << std::endl;
      retval = 1;
    }
  } //end loop over all job descriptions

  return retval;
}
