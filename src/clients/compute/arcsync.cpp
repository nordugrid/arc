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
#include <arc/compute/Endpoint.h>
#include <arc/DateTime.h>
#include <arc/IString.h>
#include <arc/Logger.h>
#include <arc/StringConv.h>
#include <arc/UserConfig.h>
#include <arc/compute/EntityRetriever.h>
#include <arc/compute/Job.h>
#include <arc/compute/JobInformationStorage.h>

#include "utils.h"

class JobSynchronizer : public Arc::EntityConsumer<Arc::Endpoint> {
public:
  JobSynchronizer(
    const Arc::UserConfig& uc,
    const std::list<Arc::Endpoint>& services,
    const std::list<std::string>& rejectedServices = std::list<std::string>(),  
    const std::set<std::string>& preferredInterfaceNames = std::set<std::string>(),
    const std::list<std::string>& capabilityFilter = std::list<std::string>(1, Arc::Endpoint::GetStringForCapability(Arc::Endpoint::COMPUTINGINFO))
  ) : uc(uc), ser(uc, Arc::EndpointQueryOptions<Arc::Endpoint>(true, capabilityFilter, rejectedServices)),
      jlr(uc, Arc::EndpointQueryOptions<Arc::Job>(preferredInterfaceNames))
  {
    jlr.needAllResults();
    ser.addConsumer(*this);
    jlr.addConsumer(jobs);
    
    for (std::list<Arc::Endpoint>::const_iterator it = services.begin(); it != services.end(); ++it) {
      if (it->HasCapability(Arc::Endpoint::REGISTRY)) {
        ser.addEndpoint(*it);
      } else {
        jlr.addEndpoint(*it);
      }
    }
  }

  void wait() {
    ser.wait();
    jlr.wait();
  }

  void addEntity(const Arc::Endpoint& service) {
    if (service.HasCapability(Arc::Endpoint::COMPUTINGINFO)) {
      jlr.addEndpoint(service);
    }
  }

  bool writeJobs(bool truncate) {
    bool jobsWritten = false;
    bool jobsReported = false;
    Arc::JobInformationStorage *jobstore = createJobInformationStorage(uc);
    if (jobstore == NULL) {
      std::cerr << Arc::IString("Warning: Unable to open job list file (%s), unknown format", uc.JobListFile()) << std::endl;
      return false;
    }
    // Write extracted job info to joblist
    if (truncate) {
      jobstore->Clean();
      if ( (jobsWritten = jobstore->Write(jobs)) ) {
        for (std::list<Arc::Job>::const_iterator it = jobs.begin();
             it != jobs.end(); ++it) {
          if (!jobsReported) {
            std::cout << Arc::IString("Found the following jobs:")<<std::endl;
            jobsReported = true;
          }
          if (!it->Name.empty()) {
            std::cout << it->Name << " (" << it->JobID << ")" << std::endl;
          }
          else {
            std::cout << it->JobID << std::endl;
          }
        }
        std::cout << Arc::IString("Total number of jobs found: ") << jobs.size() << std::endl;
      }
    }
    else {
      std::list<const Arc::Job*> newJobs;
      std::set<std::string> prunedServices;
      jlr.getServicesWithStatus(Arc::EndpointQueryingStatus::SUCCESSFUL,
                                prunedServices);
      if ( (jobsWritten = jobstore->Write(jobs, prunedServices, newJobs)) ) {
        for (std::list<const Arc::Job*>::const_iterator it = newJobs.begin();
             it != newJobs.end(); ++it) {
          if (!jobsReported) {
            std::cout << Arc::IString("Found the following new jobs:")<<std::endl;
            jobsReported = true;
          }
          if (!(*it)->Name.empty()) {
            std::cout << (*it)->Name << " (" << (*it)->JobID << ")" << std::endl;
          }
          else {
            std::cout << (*it)->JobID << std::endl;
          }
        }
        std::cout << Arc::IString("Total number of new jobs found: ") << newJobs.size() << std::endl;
      }
    }
    delete jobstore;
    if (!jobsWritten) {
      std::cout << Arc::IString("ERROR: Failed to write job information to file (%s)", uc.JobListFile()) << std::endl;
      return false;
    }

    return true;
  }

private:
  const Arc::UserConfig& uc;
  Arc::ServiceEndpointRetriever ser;
  Arc::JobListRetriever jlr;
  Arc::EntityContainer<Arc::Job> jobs;
};

int RUNMAIN(arcsync)(int argc, char **argv) {

  setlocale(LC_ALL, "");

  Arc::Logger logger(Arc::Logger::getRootLogger(), "arcsync");
  Arc::LogStream logcerr(std::cerr);
  logcerr.setFormat(Arc::ShortFormat);
  Arc::Logger::getRootLogger().addDestination(logcerr);
  Arc::Logger::getRootLogger().setThreshold(Arc::WARNING);

  Arc::ArcLocation::Init(argv[0]);

  ClientOptions opt(ClientOptions::CO_SYNC, " ",
                    istring("The arcsync command synchronizes your "
                            "local job list with the information at\n"
                            "the given resources or index servers."));

  std::list<std::string> params = opt.Parse(argc, argv);

  if (opt.showversion) {
    std::cout << Arc::IString("%s version %s", "arcsync", VERSION)
              << std::endl;
    return 0;
  }

  // If debug is specified as argument, it should be set before loading the configuration.
  if (!opt.debug.empty())
    Arc::Logger::getRootLogger().setThreshold(Arc::istring_to_level(opt.debug));

  logger.msg(Arc::VERBOSE, "Running command: %s", opt.GetCommandWithArguments());

  if (opt.show_plugins) {
    std::list<std::string> types;
    types.push_back("HED:JobListRetrieverPlugin");
    showplugins("arcsync", types, logger);
    return 0;
  }

  Arc::UserConfig usercfg(opt.conffile, opt.joblist);
  if (!usercfg) {
    logger.msg(Arc::ERROR, "Failed configuration initialization");
    return 1;
  }

  if (opt.convert) {
    Arc::JobInformationStorage *jobstore = createJobInformationStorage(usercfg);
    if (jobstore == NULL) {
      std::cerr << Arc::IString("Warning: Unable to open job list file (%s), unknown format", usercfg.JobListFile()) << std::endl;
      return 1;
    }
    // Read current jobs
    std::list<Arc::Job> jobs;
    if (!jobstore->ReadAll(jobs)) {
      std::cerr << Arc::IString("Warning: Unable to read local list of jobs from file (%s)", usercfg.JobListFile()) << std::endl;
      return 1;
    }
    // Delete existing database so new on is created with specified format
    if (!jobstore->Clean()) {
      std::cerr << Arc::IString("Warning: Unable to truncate local list of jobs in file (%s)", usercfg.JobListFile()) << std::endl;
      return 1;
    }
    delete jobstore;
    jobstore = createJobInformationStorage(usercfg);
    if (jobstore == NULL) {
      std::cerr << Arc::IString("Warning: Unable to create job list file (%s), jobs list is destroyed", usercfg.JobListFile()) << std::endl;
      return 1;
    }
    if (!jobstore->Write(jobs)) {
      std::cerr << Arc::IString("Warning: Failed to write local list of jobs into file (%s), jobs list is destroyed", usercfg.JobListFile()) << std::endl;
      return 1;
    }
    return 0;
  }

  if (opt.timeout > 0)
    usercfg.Timeout(opt.timeout);

  if (!checkproxy(usercfg)) {
    return 1;
  }

  if (opt.debug.empty() && !usercfg.Verbosity().empty())
    Arc::Logger::getRootLogger().setThreshold(Arc::istring_to_level(usercfg.Verbosity()));

  //sanity check
  if (!opt.forcesync) {
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

  // legacy options => new options
  for (std::list<std::string>::const_iterator it = opt.clusters.begin(); it != opt.clusters.end(); ++it) {
    opt.computing_elements.push_back(*it); 
  }
  for (std::list<std::string>::const_iterator it = opt.indexurls.begin(); it != opt.indexurls.end(); ++it) {
    opt.registries.push_back(*it);
  }

  std::list<Arc::Endpoint> endpoints = getServicesFromUserConfigAndCommandLine(usercfg, opt.registries, opt.computing_elements);

  std::list<std::string> rejectDiscoveryURLs = getRejectDiscoveryURLsFromUserConfigAndCommandLine(usercfg, opt.rejectdiscovery);

  if (endpoints.empty()) {
    logger.msg(Arc::ERROR, "No services specified. Please configure default services in the client configuration, "
                           "or specify a cluster or index (-c or -g options, see arcsync -h).");
    return 1;
  }


  std::set<std::string> preferredInterfaceNames;
  if (usercfg.InfoInterface().empty()) {
    preferredInterfaceNames.insert("org.nordugrid.ldapglue2");
  } else {
    preferredInterfaceNames.insert(usercfg.InfoInterface());
  }

  JobSynchronizer js(usercfg, endpoints, rejectDiscoveryURLs, preferredInterfaceNames);
  js.wait();
  return js.writeJobs(opt.truncate)?0:1; // true -> 0, false -> 1.
}
