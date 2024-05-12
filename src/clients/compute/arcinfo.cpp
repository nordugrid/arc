// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include <list>
#include <string>

#include <arc/ArcLocation.h>
#include <arc/compute/Endpoint.h>
#include <arc/IString.h>
#include <arc/Logger.h>
#include <arc/StringConv.h>
#include <arc/UserConfig.h>
#include <arc/compute/ComputingServiceRetriever.h>

#include "utils.h"

int RUNMAIN(arcinfo)(int argc, char **argv) {

  setlocale(LC_ALL, "");

  Arc::Logger logger(Arc::Logger::getRootLogger(), "arcinfo");
  Arc::LogStream logcerr(std::cerr);
  logcerr.setFormat(Arc::ShortFormat);
  Arc::Logger::getRootLogger().addDestination(logcerr);
  Arc::Logger::getRootLogger().setThreshold(Arc::WARNING);

  Arc::ArcLocation::Init(argv[0]);

  ClientOptions opt(ClientOptions::CO_INFO,
                    istring("[resource ...]"),
                    istring("The arcinfo command is used for "
                            "obtaining the status of computing "
                            "resources on the Grid."));

  {
    std::list<std::string> clusterstmp = opt.Parse(argc, argv);
    opt.clusters.insert(opt.clusters.end(), clusterstmp.begin(), clusterstmp.end());
  }

  if (opt.showversion) {
    std::cout << Arc::IString("%s version %s", "arcinfo", VERSION) << std::endl;
    return 0;
  }

  // If debug is specified as argument, it should be set before loading the configuration.
  if (!opt.debug.empty())
    Arc::Logger::getRootLogger().setThreshold(Arc::istring_to_level(opt.debug));
  
  logger.msg(Arc::VERBOSE, "Running command: %s", opt.GetCommandWithArguments());

  if (opt.show_plugins) {
    std::list<std::string> types;
    types.push_back("HED:ServiceEndpointRetrieverPlugin");
    types.push_back("HED:TargetInformationRetrieverPlugin");
    showplugins("arcinfo", types, logger);
    return 0;
  }
  
  Arc::UserConfig usercfg(opt.conffile);
  if (!usercfg) {
    logger.msg(Arc::ERROR, "Failed configuration initialization");
    return 1;
  }
  if (opt.force_default_ca) usercfg.CAUseDefault(true);
  if (opt.force_grid_ca) usercfg.CAUseDefault(false);
 
  if (opt.list_configured_services) {
    std::map<std::string, Arc::ConfigEndpoint> allServices = usercfg.GetAllConfiguredServices();
    std::cout << "Configured registries:" << std::endl;
    for (std::map<std::string, Arc::ConfigEndpoint>::const_iterator it = allServices.begin(); it != allServices.end(); ++it) {
      if (it->second.type == Arc::ConfigEndpoint::REGISTRY) {
        std::cout << "  " << it->first << ": " << it->second.URLString;
        if (!it->second.InterfaceName.empty()) {
          std::cout << " (" << it->second.InterfaceName << ")";
        }
        std::cout << std::endl;
      }
    }
    std::cout << "Configured computing elements:" << std::endl;
    for (std::map<std::string, Arc::ConfigEndpoint>::const_iterator it = allServices.begin(); it != allServices.end(); ++it) {
      if (it->second.type == Arc::ConfigEndpoint::COMPUTINGINFO) {
        std::cout << "  " << it->first << ": " << it->second.URLString;
        if (!it->second.InterfaceName.empty() || !it->second.RequestedSubmissionInterfaceName.empty()) {
          std::cout << " (" << it->second.InterfaceName;
          if (!it->second.InterfaceName.empty() && !it->second.RequestedSubmissionInterfaceName.empty()) {
            std::cout << " / ";
          }
          std::cout << it->second.RequestedSubmissionInterfaceName + ")";
        }
        std::cout << std::endl;
      }
    }
    return 0;
  }

  if (opt.debug.empty() && !usercfg.Verbosity().empty())
    Arc::Logger::getRootLogger().setThreshold(Arc::istring_to_level(usercfg.Verbosity()));

  if (opt.timeout > 0)
    usercfg.Timeout(opt.timeout);

  AuthenticationType authentication_type = UndefinedAuthentication;
  if(!opt.getAuthenticationType(logger, usercfg, authentication_type))
    return 1;
  switch(authentication_type) {
    case NoAuthentication:
      usercfg.CommunicationAuthType(Arc::UserConfig::AuthTypeNone);
      break;
    case X509Authentication:
      usercfg.CommunicationAuthType(Arc::UserConfig::AuthTypeCert);
      break;
    case TokenAuthentication:
      usercfg.CommunicationAuthType(Arc::UserConfig::AuthTypeToken);
      break;
    case UndefinedAuthentication:
    default:
      usercfg.CommunicationAuthType(Arc::UserConfig::AuthTypeUndefined);
      break;
  }

  if (!opt.canonicalizeARC6InterfaceTypes(logger)) return 1;
  std::list<Arc::Endpoint> endpoints = getServicesFromUserConfigAndCommandLine(usercfg, opt.indexurls, opt.clusters, opt.submit_types.front(), opt.info_types.front());

  std::set<std::string> preferredInterfaceNames;
  if (usercfg.InfoInterface().empty()) {
    preferredInterfaceNames.insert("org.nordugrid.arcrest");
  } else {
    preferredInterfaceNames.insert(usercfg.InfoInterface());
  }

  std::list<std::string> rejectDiscoveryURLs = getRejectDiscoveryURLsFromUserConfigAndCommandLine(usercfg, opt.rejectdiscovery);

  Arc::ComputingServiceUniq csu;
  Arc::ComputingServiceRetriever csr(usercfg, std::list<Arc::Endpoint>(), rejectDiscoveryURLs, preferredInterfaceNames);
  csr.addConsumer(csu);
  for (std::list<Arc::Endpoint>::const_iterator it = endpoints.begin(); it != endpoints.end(); ++it) {
    csr.addEndpoint(*it);
  }
  csr.wait();

  std::list<Arc::ComputingServiceType> services = csu.getServices();
  for (std::list<Arc::ComputingServiceType>::const_iterator it = services.begin();
       it != services.end(); ++it) {
    if (opt.longlist) {
      if (it != services.begin()) std::cout << std::endl;
      std::cout << *it;
      std::cout << std::flush;
    }
    else {
      std::cout << "Computing service: " << (**it).Name;
      if (!(**it).QualityLevel.empty()) {
        std::cout << " (" << (**it).QualityLevel << ")";
      }
      std::cout << std::endl;
      
      std::stringstream infostream, submissionstream;
      for (std::map<int, Arc::ComputingEndpointType>::const_iterator itCE = it->ComputingEndpoint.begin();
           itCE != it->ComputingEndpoint.end(); ++itCE) {
        if (itCE->second->Capability.count(Arc::Endpoint::GetStringForCapability(Arc::Endpoint::COMPUTINGINFO))) {
          infostream << "  " << Arc::IString("Information endpoint") << ": " << itCE->second->URLString;
          if ( !itCE->second->InterfaceName.empty() ) {
            infostream << " (" << itCE->second->InterfaceName << ")";
          }
          infostream << std::endl;
        }
        if (itCE->second->Capability.empty() ||
            itCE->second->Capability.count(Arc::Endpoint::GetStringForCapability(Arc::Endpoint::JOBSUBMIT)) ||
            itCE->second->Capability.count(Arc::Endpoint::GetStringForCapability(Arc::Endpoint::JOBCREATION)))
        {
          submissionstream << "  ";
          submissionstream << Arc::IString("Submission endpoint") << ": ";
          submissionstream << itCE->second->URLString;
          submissionstream << " (" << Arc::IString("status") << ": ";
          submissionstream << itCE->second->HealthState << ", ";
          submissionstream << Arc::IString("interface") << ": ";
          submissionstream << itCE->second->InterfaceName << ")" << std::endl;           
        }
      }
      
      std::cout << infostream.str() << submissionstream.str();
    }
  }

  bool anEndpointFailed = false;
  // Check if querying endpoint succeeded.
  Arc::EndpointStatusMap statusMap = csr.getAllStatuses();
  for (std::list<Arc::Endpoint>::const_iterator it = endpoints.begin(); it != endpoints.end(); ++it) {
    Arc::EndpointStatusMap::const_iterator itStatus = statusMap.find(*it);
    if (itStatus != statusMap.end() &&
        itStatus->second != Arc::EndpointQueryingStatus::SUCCESSFUL &&
        itStatus->second != Arc::EndpointQueryingStatus::SUSPENDED_NOTREQUIRED) {
      if (!anEndpointFailed) {
        anEndpointFailed = true;
        std::cerr << Arc::IString("ERROR: Failed to retrieve information from the following endpoints:") << std::endl;
      }
      
      std::cerr << "  " << it->URLString;
      if (!itStatus->second.getDescription().empty()) {
        std::cerr << " (" << itStatus->second.getDescription() << ")";
      }
      std::cerr << std::endl;
    }
  }
  if (anEndpointFailed) return 1;
  
  if (services.empty()) {
    std::cerr << Arc::IString("ERROR: Failed to retrieve information");
    if (!endpoints.empty()) {
      std::cerr << " " << Arc::IString("from the following endpoints:") << std::endl;
      for (std::list<Arc::Endpoint>::const_iterator it = endpoints.begin(); it != endpoints.end(); ++it) {
        std::cerr << "  " << it->URLString << std::endl;
      }
    } else {
      std::cerr << std::endl;
    }
    return 1;
  }

  return 0;
}
