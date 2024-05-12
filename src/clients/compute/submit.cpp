// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <list>
#include <string>
#include <set>
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
#include <arc/compute/JobInformationStorage.h>
#include <arc/compute/SubmissionStatus.h>
#include <arc/compute/Submitter.h>

#include "submit.h"

static Arc::Logger logger(Arc::Logger::getRootLogger(), "submit");

void HandleSubmittedJobs::addEntity(const Arc::Job& j) {
  std::cout << Arc::IString("Job submitted with jobid: %s", j.JobID) << std::endl;
  submittedJobs.push_back(j);
}

void HandleSubmittedJobs::write() const {
  if (!jobidfile.empty() && !Arc::Job::WriteJobIDsToFile(submittedJobs, jobidfile)) {
    logger.msg(Arc::WARNING, "Cannot write job IDs to file (%s)", jobidfile);
  }
  Arc::JobInformationStorage* jobStore = createJobInformationStorage(uc);
  if (jobStore == NULL || !jobStore->Write(submittedJobs)) {
    if (jobStore == NULL) {
      logger.msg(Arc::WARNING, "Unable to open job list file (%s), unknown format", uc.JobListFile());
    } else {
      logger.msg(Arc::WARNING, "Failed to write job information to database (%s)", uc.JobListFile());
    }
    logger.msg(Arc::WARNING, "To recover missing jobs, run arcsync");
  }
  logger.msg(Arc::DEBUG, "Record about new job successfully added to the database (%s)", uc.JobListFile());
  delete jobStore;
}

void HandleSubmittedJobs::printsummary(const std::list<Arc::JobDescription>& originalDescriptions, const std::list<const Arc::JobDescription*>& notsubmitted) const {
  if (originalDescriptions.size() > 1) {
    std::cout << std::endl << Arc::IString("Job submission summary:") << std::endl;
    std::cout << "-----------------------" << std::endl;
    std::cout << Arc::IString("%d of %d jobs were submitted", submittedJobs.size(), submittedJobs.size()+notsubmitted.size()) << std::endl;
    if (!notsubmitted.empty()) {
      std::cout << std::endl << Arc::IString("The following jobs were not submitted:") << std::endl;
      int jobnr = 1;
      for (std::list<const Arc::JobDescription*>::const_iterator it = notsubmitted.begin();
           it != notsubmitted.end(); ++it) {
        std::cout << " * " << Arc::IString("Job nr.") << " " << jobnr << ":" << std::endl;
        (*it)->SaveToStream(std::cout, "userlong");
        jobnr++;
      }
    }
  }
}

int process_submission_status(Arc::SubmissionStatus status, const Arc::UserConfig& usercfg) {
  if (status.isSet(Arc::SubmissionStatus::BROKER_PLUGIN_NOT_LOADED)) {
    std::cerr << Arc::IString("ERROR: Unable to load broker %s", usercfg.Broker().first) << std::endl;
    return 2;
  }
  if (status.isSet(Arc::SubmissionStatus::NO_SERVICES)) {
   std::cerr << Arc::IString("ERROR: Job submission aborted because no resource returned any information") << std::endl;
   return 2;
  }
  if (status.isSet(Arc::SubmissionStatus::DESCRIPTION_NOT_SUBMITTED)) {
    std::cerr << Arc::IString("ERROR: One or multiple job descriptions was not submitted.") << std::endl;
    return 1;
  }
  return 0;
}

void check_missing_plugins(Arc::Submitter s, int is_error) {
  bool gridFTPJobPluginFailed = false;
  for (std::map<Arc::Endpoint, Arc::EndpointSubmissionStatus>::const_iterator it = s.GetEndpointSubmissionStatuses().begin();
       it != s.GetEndpointSubmissionStatuses().end(); ++it) {
    if (it->first.InterfaceName == "org.nordugrid.gridftpjob" && it->second == Arc::EndpointSubmissionStatus::NOPLUGIN) {
      gridFTPJobPluginFailed = true;
    }
  }
  if (gridFTPJobPluginFailed) {
    Arc::LogLevel level  = (is_error ? Arc::ERROR : Arc::INFO);
    std::string indent   = (is_error ? "       " : "      ");
    logger.msg(level, "A computing resource using the GridFTP interface was requested, but\n"
                      "%sthe corresponding plugin could not be loaded. Is the plugin installed?\n"
                      "%sIf not, please install the package 'nordugrid-arc-plugins-globus'.\n"
                      "%sDepending on your type of installation the package name might differ.", indent, indent, indent);
  }
  // TODO: What to do when failing to load other plugins.
}

int dumpjobdescription(const Arc::UserConfig& usercfg, const std::list<Arc::JobDescription>& jobdescriptionlist, const std::list<Arc::Endpoint>& services, const std::string& requestedSubmissionInterface) {
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
  for (std::list<Arc::Endpoint>::const_iterator it = services.begin(); it != services.end(); ++it) {
    csr.addEndpoint(*it);
  }
  csr.wait();
  std::list<Arc::ComputingServiceType> CEs = csu.getServices();


  if (CEs.empty()) {
    std::cout << Arc::IString("Unable to adapt job description to any resource, no resource information could be obtained.") << std::endl;
    std::cout << Arc::IString("Original job description is listed below:") << std::endl;
    for (std::list<Arc::JobDescription>::const_iterator it = jobdescriptionlist.begin();
         it != jobdescriptionlist.end(); ++it) {
      std::string descOutput;
      it->UnParse(descOutput, it->GetSourceLanguage());
      std::cout << descOutput << std::endl;
    }
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
        if(!requestedSubmissionInterface.empty() && ets->ComputingEndpoint->InterfaceName != requestedSubmissionInterface) continue;
        if (!jobdescdump.Prepare(*ets)) {
          logger.msg(Arc::INFO, "Unable to prepare job description according to needs of the target resource (%s).", ets->ComputingEndpoint->URLString);
          continue;
        }

        std::string jobdesclang = "emies:adl";
        if (ets->ComputingEndpoint->InterfaceName == "org.nordugrid.gridftpjob") {
          jobdesclang = "nordugrid:xrsl";
        }
        else if (ets->ComputingEndpoint->InterfaceName == "org.ogf.glue.emies.activitycreation") {
          jobdesclang = "emies:adl";
        }
        else if (ets->ComputingEndpoint->InterfaceName == "org.nordugrid.internal") {
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

bool prepare_submission_endpoint_batches(const Arc::UserConfig& usercfg, const ClientOptions& opt, std::list<std::list<Arc::Endpoint> >& endpoint_batches) {
  bool info_discovery = true;

  // Computing element direct targets
  for (std::list<std::string>::const_iterator it = opt.computing_elements.begin();
       it != opt.computing_elements.end(); ++it) {
    if (opt.info_types.empty()) {
      std::list<Arc::Endpoint> endpoints;
      // any interfaces can be used: start with discovery
      if (opt.submit_types.empty()) {
        Arc::Endpoint service(*it);
        service.Capability.insert(Arc::Endpoint::GetStringForCapability(Arc::Endpoint::COMPUTINGINFO));
        service.RequestedSubmissionInterfaceName = "";
        endpoints.push_back(service);
      } else {
      // discovery is disabled - submit directly in the defined interface order
        info_discovery = false;
        for (std::list<std::string>::const_iterator sit = opt.submit_types.begin();
             sit != opt.submit_types.end(); ++sit) {
           Arc::Endpoint service(*it);
           service.Capability.insert(Arc::Endpoint::GetStringForCapability(Arc::Endpoint::JOBSUBMIT));
           service.Capability.insert(Arc::Endpoint::GetStringForCapability(Arc::Endpoint::JOBCREATION));
           service.InterfaceName = *sit;
           endpoints.push_back(service);
        }
      }
      endpoint_batches.push_back(endpoints);
    // add infointerfaces of all defined types when discovery is used
    } else {
      for (std::list<std::string>::const_iterator sit = opt.submit_types.begin();
             sit != opt.submit_types.end(); ++sit) {
        std::list<Arc::Endpoint> endpoints;
        for (std::list<std::string>::const_iterator iit = opt.info_types.begin();
             iit != opt.info_types.end(); ++iit) {
          Arc::Endpoint service(*it);
          service.Capability.insert(Arc::Endpoint::GetStringForCapability(Arc::Endpoint::COMPUTINGINFO));
          service.InterfaceName = *iit;
          service.RequestedSubmissionInterfaceName = *sit;
          endpoints.push_back(service);
        }
        endpoint_batches.push_back(endpoints);
      }
    }
  }

  // Query the registries for available endpoints
  if (!opt.registries.empty()) {
    Arc::EntityContainer<Arc::Endpoint> registry_endpoints;

    // Get all service endpoints regardless of capabilities
    std::list<std::string> rejectDiscoveryURLs =
      getRejectDiscoveryURLsFromUserConfigAndCommandLine(usercfg, opt.rejectdiscovery);
    std::list<std::string> capabilityFilter;

    Arc::ServiceEndpointRetriever ser(usercfg, Arc::EndpointQueryOptions<Arc::Endpoint>(
                                      true, capabilityFilter, rejectDiscoveryURLs));
    ser.addConsumer(registry_endpoints);
    for (std::list<std::string>::const_iterator it = opt.registries.begin();
         it != opt.registries.end(); ++it) {
      Arc::Endpoint registry(*it);
      registry.Capability.insert(Arc::Endpoint::GetStringForCapability(Arc::Endpoint::REGISTRY));
      ser.addEndpoint(registry);
    }
    ser.wait();

    // Loop over endpoints returned by registry and match against interface types
    if ( !opt.info_types.empty() ) {
      for (std::list<std::string>::const_iterator sit = opt.submit_types.begin();
           sit != opt.submit_types.end(); ++sit) {
        std::list<Arc::Endpoint> endpoints;
        for (Arc::EntityContainer<Arc::Endpoint>::iterator eit = registry_endpoints.begin();
             eit != registry_endpoints.end(); ++eit) {
          for (std::list<std::string>::const_iterator iit = opt.info_types.begin();
               iit != opt.info_types.end(); ++iit) {
            if ( eit->InterfaceName == *iit ) {
              Arc::Endpoint service(*eit);
              logger.msg(Arc::INFO, "Service endpoint %s (type %s) added to the list for resource discovery",
                         eit->URLString, eit->InterfaceName);
              service.RequestedSubmissionInterfaceName = *sit;
              endpoints.push_back(service);
            }
          }
        }
        if (!endpoints.empty()) {
          endpoint_batches.push_back(endpoints);
        } else {
          logger.msg(Arc::WARNING, "There are no endpoints in registry that match requested info endpoint type");
        }
      }
    // endpoint types was not requested at all
    } else if ( opt.submit_types.empty() ) {
      // try all infodiscovery endpoints but prioritize the interfaces in the following order
      std::list<std::string> info_priority;
      info_priority.push_back("org.ogf.glue.emies.resourceinfo");
      info_priority.push_back("org.nordugrid.arcrest");
      info_priority.push_back("org.nordugrid.ldapglue2");
      info_priority.push_back("org.nordugrid.ldapng");
      for (std::list<std::string>::const_iterator iit = info_priority.begin();
           iit != info_priority.end(); ++iit) {
        std::list<Arc::Endpoint> endpoints;
        for (Arc::EntityContainer<Arc::Endpoint>::iterator eit = registry_endpoints.begin();
             eit != registry_endpoints.end(); ++eit) {
          if ( eit->InterfaceName == *iit ) {
            Arc::Endpoint service(*eit);
            service.RequestedSubmissionInterfaceName = "";
            endpoints.push_back(service);
            logger.msg(Arc::INFO, "Service endpoint %s (type %s) added to the list for resource discovery",
                       eit->URLString, eit->InterfaceName);
          }
        }
        if (!endpoints.empty()) endpoint_batches.push_back(endpoints);
      }
    // it was requested to disable infodiscovery for targets
    } else {
      info_discovery = false;
      std::list<Arc::Endpoint> endpoints;
      for (std::list<std::string>::const_iterator sit = opt.submit_types.begin();
           sit != opt.submit_types.end(); ++sit) {
        for (Arc::EntityContainer<Arc::Endpoint>::iterator eit = registry_endpoints.begin();
             eit != registry_endpoints.end(); ++eit) {
          if ( eit->InterfaceName == *sit ) {
            Arc::Endpoint service(*eit);
            service.Capability.clear();
            service.Capability.insert(Arc::Endpoint::GetStringForCapability(Arc::Endpoint::JOBSUBMIT));
            service.Capability.insert(Arc::Endpoint::GetStringForCapability(Arc::Endpoint::JOBCREATION));
            service.InterfaceName = *sit;
            endpoints.push_back(service);
            logger.msg(Arc::INFO, "Service endpoint %s (type %s) added to the list for direct submission",
                       eit->URLString, eit->InterfaceName);
          }
        }
      }
      if (!endpoints.empty()) {
        endpoint_batches.push_back(endpoints);
      } else {
        logger.msg(Arc::WARNING, "There are no endpoints in registry that match requested submission endpoint type");
      }
    }
  }
  return info_discovery;
}

int submit_jobs(const Arc::UserConfig& usercfg, const std::list<std::list<Arc::Endpoint> >& endpoint_batches, bool info_discovery, const std::string& jobidfile, const std::list<Arc::JobDescription>& jobdescriptionlist, DelegationType delegation_type) {

    HandleSubmittedJobs hsj(jobidfile, usercfg);
    Arc::Submitter submitter(usercfg);
    submitter.addConsumer(hsj);

    std::list<Arc::JobDescription> w_jobdescriptionlist(jobdescriptionlist);
    int error_check = 0;
    for(std::list<Arc::JobDescription>::iterator it = w_jobdescriptionlist.begin();
                    it != w_jobdescriptionlist.end(); ++it) {
      it->X509Delegation = (delegation_type == X509Delegation);
      it->TokenDelegation = (delegation_type == TokenDelegation);
      for(std::list<Arc::JobDescription>::iterator itAlt = it->GetAlternatives().begin();
                      itAlt != it->GetAlternatives().end(); ++itAlt) {
        itAlt->X509Delegation = (delegation_type == X509Delegation);
        itAlt->TokenDelegation = (delegation_type == TokenDelegation);
      }
    }

    for (std::list<std::list<Arc::Endpoint> >::const_iterator it = endpoint_batches.begin();
         it != endpoint_batches.end(); ++it) {
      Arc::SubmissionStatus status;
      if (info_discovery) {
        status = submitter.BrokeredSubmit(*it, w_jobdescriptionlist);
      } else {
        status = submitter.Submit(*it, w_jobdescriptionlist);
      }

      hsj.write();

      error_check = process_submission_status(status, usercfg);
      if (error_check == 2) return 1;

      if (submitter.GetDescriptionsNotSubmitted().empty()) break;

      if (status.isSet(Arc::SubmissionStatus::SUBMITTER_PLUGIN_NOT_LOADED))
        check_missing_plugins(submitter, error_check);

      // remove already submitted jobs from description list
      std::list<const Arc::JobDescription*> failedjd = submitter.GetDescriptionsNotSubmitted();
      std::list<Arc::JobDescription>::iterator itOrig = w_jobdescriptionlist.begin();
      while ( itOrig != w_jobdescriptionlist.end() ) {
        bool is_failedjd = false;
        for (std::list<const Arc::JobDescription*>::const_iterator itFailed = failedjd.begin();
             itFailed != failedjd.end(); ++itFailed) {
          if (&(*itOrig) == *itFailed) {
            is_failedjd = true;
            break;
          }
        }
        if (is_failedjd) {
            ++itOrig;
            continue;
        }
        w_jobdescriptionlist.erase(itOrig++);
      }
    }

    hsj.printsummary(jobdescriptionlist, submitter.GetDescriptionsNotSubmitted());
    return error_check;
}

