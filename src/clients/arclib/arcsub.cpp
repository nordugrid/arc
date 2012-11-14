// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <list>
#include <string>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <arc/ArcLocation.h>
#include <arc/IString.h>
#include <arc/Logger.h>
#include <arc/UserConfig.h>
#include <arc/Utils.h>
#include <arc/compute/Broker.h>
#include <arc/compute/ComputingServiceRetriever.h>
#include <arc/compute/ExecutionTarget.h>
#include <arc/compute/Job.h>
#include <arc/compute/JobDescription.h>
#include <arc/compute/Submitter.h>

#include "utils.h"

static Arc::Logger logger(Arc::Logger::getRootLogger(), "arcsub");

static int submit(const Arc::UserConfig& usercfg, const std::list<Arc::JobDescription>& jobdescriptionlist, const std::list<Arc::Endpoint>& services, const std::list<std::string> requestedSubmissionInterfaces, const std::string& jobidfile);
static int dumpjobdescription(const Arc::UserConfig& usercfg, const std::list<Arc::JobDescription>& jobdescriptionlist, const std::list<Arc::Endpoint>& services, const std::list<std::string> requestedSubmissionInterfaces);

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

  std::list<Arc::Endpoint> services = getServicesFromUserConfigAndCommandLine(usercfg, opt.indexurls, opt.clusters, opt.requestedSubmissionInterfaceName, opt.infointerface);

  usercfg.AddRejectDiscoveryURLs(opt.rejectdiscovery);

  std::list<std::string> rsi;
  if(!opt.requestedSubmissionInterfaceName.empty()) rsi.push_back(opt.requestedSubmissionInterfaceName);

  if (opt.dumpdescription) {
    return dumpjobdescription(usercfg, jobdescriptionlist, services, rsi);
  }

  return submit(usercfg, jobdescriptionlist, services, rsi, opt.jobidoutfile);
}


class HandleSubmittedJobs : public Arc::EntityConsumer<Arc::Job> {
public:
  HandleSubmittedJobs(const std::string& jobidfile, const std::string& joblist) : jobidfile(jobidfile), joblist(joblist), submittedJobs() {}

  void addEntity(const Arc::Job& j) {
    std::cout << Arc::IString("Job submitted with jobid: %s", j.JobID.fullstr()) << std::endl;
    submittedJobs.push_back(j);
  }
  
  void write() const {
    if (!jobidfile.empty() && !Arc::Job::WriteJobIDsToFile(submittedJobs, jobidfile)) {
      logger.msg(Arc::WARNING, "Cannot write jobids to file (%s)", jobidfile);
    }
    if (!Arc::Job::WriteJobsToFile(joblist, submittedJobs)) {
      std::cout << Arc::IString("Warning: Failed to lock job list file %s", joblist)
                << std::endl;
      std::cout << Arc::IString("To recover missing jobs, run arcsync") << std::endl;
    }
  }

  void printsummary(const std::list<Arc::JobDescription>& originalDescriptions, const std::list<const Arc::JobDescription*>& notsubmitted) const {
    if (originalDescriptions.size() > 1) {
      std::cout << std::endl << Arc::IString("Job submission summary:") << std::endl;
      std::cout << "-----------------------" << std::endl;
      std::cout << Arc::IString("%d of %d jobs were submitted", submittedJobs.size(), submittedJobs.size()+notsubmitted.size()) << std::endl;
      if (!notsubmitted.empty()) {
        std::cout << Arc::IString("The following %d were not submitted", notsubmitted.size()) << std::endl;
        for (std::list<const Arc::JobDescription*>::const_iterator it = notsubmitted.begin();
             it != notsubmitted.end(); ++it) {
          int jobnr = 1;
          for (std::list<Arc::JobDescription>::const_iterator itOrig = originalDescriptions.begin();
               itOrig != originalDescriptions.end(); ++itOrig, ++jobnr) {
            if (&(*itOrig) == *it) {
              std::cout << Arc::IString("Job nr.") << " " << jobnr;
              if (!(*it)->Identification.JobName.empty()) {
                std::cout << ": " << (*it)->Identification.JobName;
              }
              std::cout << std::endl;
              break;
            }
          }
        }
      }
    }
  }

  void clearsubmittedjobs() { submittedJobs.clear(); }

private:
  const std::string jobidfile, joblist;
  std::list<Arc::Job> submittedJobs;
};


static bool match_submission_interface(const Arc::ExecutionTarget& target, const std::list<std::string>& requestedSubmissionInterfaces) {
  if(requestedSubmissionInterfaces.empty()) return true;
  for(std::list<std::string>::const_iterator iname = requestedSubmissionInterfaces.begin();
                iname != requestedSubmissionInterfaces.end();++iname) {
    if(*iname == target.ComputingEndpoint->InterfaceName) return true;
  }
  return false;
}

static int submit(const Arc::UserConfig& usercfg, const std::list<Arc::JobDescription>& jobdescriptionlist, const std::list<Arc::Endpoint>& services, const std::list<std::string> requestedSubmissionInterfaces, const std::string& jobidfile) {
  int retval = 0;
  
  HandleSubmittedJobs hsj(jobidfile, usercfg.JobListFile());
  Arc::Submitter s(usercfg);
  s.addConsumer(hsj);

  Arc::SubmissionStatus status = s.BrokeredSubmit(services, jobdescriptionlist, requestedSubmissionInterfaces);
  hsj.write();

  if (status.isSet(Arc::SubmissionStatus::BROKER_PLUGIN_NOT_LOADED)) {
    std::cerr << Arc::IString("ERROR: Unable to load broker %s", usercfg.Broker().first) << std::endl;
    return 1;
  }
  if (status.isSet(Arc::SubmissionStatus::NO_SERVICES)) {
   std::cerr << Arc::IString("ERROR: Job submission aborted because no resource returned any information") << std::endl;
   return 1;
  }
  if (status.isSet(Arc::SubmissionStatus::SUBMITTER_PLUGIN_NOT_LOADED)) {
    bool gridFTPJobPluginFailed = false;
    for (std::map<Arc::Endpoint, Arc::EndpointSubmissionStatus>::const_iterator it = s.GetEndpointSubmissionStatuses().begin();
         it != s.GetEndpointSubmissionStatuses().end(); ++it) {
      if (it->first.InterfaceName == "org.nordugrid.gridftpjob" && it->second == Arc::EndpointSubmissionStatus::NOPLUGIN) {
        gridFTPJobPluginFailed = true;
      }
    }
    if (gridFTPJobPluginFailed) {
      std::cerr << Arc::IString("ERROR: A computing resource using the gridftp interface was requested, but") << std::endl;
      std::cerr << Arc::IString("       the corresponding plugin could not be loaded. Is the plugin installed?") << std::endl;
      std::cerr << Arc::IString("       If not, please install the package 'nordugrid-arc-plugins-globus'.") << std::endl;
      std::cerr << Arc::IString("       Depending on your type of installation the package name might differ. ") << std::endl;
    }
    // TODO: What to do when failing to load other plugins.
    retval = 1;
  }
  if (status.isSet(Arc::SubmissionStatus::DESCRIPTION_NOT_SUBMITTED)) {
    std::cerr << Arc::IString("ERROR: One or multiple job descriptions was not submitted.") << std::endl;
    retval = 1;
  }
    
  hsj.printsummary(jobdescriptionlist, s.GetDescriptionsNotSubmitted());

  return retval;
}

static int dumpjobdescription(const Arc::UserConfig& usercfg, const std::list<Arc::JobDescription>& jobdescriptionlist, const std::list<Arc::Endpoint>& services, std::list<std::string> requestedSubmissionInterfaces) {
  int retval = 0;

  std::set<std::string> preferredInterfaceNames;
  if (usercfg.InfoInterface().empty()) {
    preferredInterfaceNames.insert("org.nordugrid.ldapglue2");
  } else {
    preferredInterfaceNames.insert(usercfg.InfoInterface());
  }

  Arc::ComputingServiceUniq csu;
  Arc::ComputingServiceRetriever csr(usercfg, std::list<Arc::Endpoint>(), usercfg.RejectDiscoveryURLs(), preferredInterfaceNames);
  csr.addConsumer(csu);
  for (std::list<Arc::Endpoint>::const_iterator it = services.begin(); it != services.end(); it++) {
    csr.addEndpoint(*it);
  }
  csr.wait();
  std::list<Arc::ComputingServiceType> CEs = csu.getServices();


  if (CEs.empty()) {
    std::cout << Arc::IString("Dumping job description aborted because no resource returned any information") << std::endl;
    return 1;
  }

  Arc::Broker broker(usercfg, usercfg.Broker().first);
  if (!broker.isValid(false)) {
    logger.msg(Arc::ERROR, "Dumping job description aborted: Unable to load broker %s", usercfg.Broker().first);
    return 1;
  }

  Arc::ExecutionTargetSorter ets(broker, CEs);
  std::list<Arc::JobDescription>::const_iterator itJAlt; // Iterator to use for alternative job descriptions.
  for (std::list<Arc::JobDescription>::const_iterator itJ = jobdescriptionlist.begin();
       itJ != jobdescriptionlist.end(); ++itJ) {
    const Arc::JobDescription* currentJobDesc = &*itJ;
    bool descriptionDumped = false;
    do {
      Arc::JobDescription jobdescdump(*currentJobDesc);
      ets.set(jobdescdump);

      for (ets.reset(); !ets.endOfList(); ets.next()) {
        if(!match_submission_interface(*ets,requestedSubmissionInterfaces)) continue;
        if (!jobdescdump.Prepare(*ets)) {
          logger.msg(Arc::INFO, "Unable to prepare job description according to needs of the target resource (%s).", ets->ComputingEndpoint->URLString); 
          continue;
        }
  
        std::string jobdesclang = "nordugrid:jsdl";
        if (ets->ComputingEndpoint->InterfaceName == "org.nordugrid.gridftpjob") {
          jobdesclang = "nordugrid:xrsl";
        }
        else if (ets->ComputingEndpoint->InterfaceName == "org.glite.cream") {
          jobdesclang = "egee:jdl";
        }
        else if (ets->ComputingEndpoint->InterfaceName == "org.ogf.glue.emies.activitycreation") {
          jobdesclang = "emies:adl";
        }
        std::string jobdesc;
        if (!jobdescdump.UnParse(jobdesc, jobdesclang)) {
          logger.msg(Arc::INFO, "An error occurred during the generation of job description to be sent to %s", ets->ComputingEndpoint->URLString); 
          continue;
        }
  
        std::cout << Arc::IString("Job description to be sent to %s:", ets->AdminDomain->Name) << std::endl;
        std::cout << jobdesc << std::endl;
        descriptionDumped = true;
        break;
      }

      if (!descriptionDumped && itJ->HasAlternatives()) { // Alternative job descriptions.
        if (currentJobDesc == &*itJ) {
          itJAlt = itJ->GetAlternatives().begin();
        }
        else {
          ++itJAlt;
        }
        currentJobDesc = &*itJAlt;
      }
    } while (!descriptionDumped && itJ->HasAlternatives() && itJAlt != itJ->GetAlternatives().end());

    if (ets.endOfList()) {
      std::cout << Arc::IString("Unable to prepare job description according to needs of the target resource.") << std::endl;
      retval = 1;
    }
  } //end loop over all job descriptions

  return retval;
}
