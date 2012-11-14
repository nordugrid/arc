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
    Arc::Logger::getRootLogger().setThreshold(Arc::string_to_level(opt.debug));

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
  
  if (opt.list_configured_services) {
    std::map<std::string, Arc::ConfigEndpoint> allServices = usercfg.GetAllConfiguredServices();
    std::cout << "Configured registries:" << std::endl;
    for (std::map<std::string, Arc::ConfigEndpoint>::const_iterator it = allServices.begin(); it != allServices.end(); it++) {
      if (it->second.type == Arc::ConfigEndpoint::REGISTRY) {
        std::cout << "  " << it->first << ": " << it->second.URLString;
        if (!it->second.InterfaceName.empty()) {
          std::cout << " (" << it->second.InterfaceName << ")";
        }
        std::cout << std::endl;
      }
    }
    std::cout << "Configured computing elements:" << std::endl;
    for (std::map<std::string, Arc::ConfigEndpoint>::const_iterator it = allServices.begin(); it != allServices.end(); it++) {
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
    Arc::Logger::getRootLogger().setThreshold(Arc::string_to_level(usercfg.Verbosity()));

  if (opt.timeout > 0)
    usercfg.Timeout(opt.timeout);

  std::list<Arc::Endpoint> endpoints = getServicesFromUserConfigAndCommandLine(usercfg, opt.indexurls, opt.clusters, opt.requestedSubmissionInterfaceName, opt.infointerface);

  std::set<std::string> preferredInterfaceNames;
  if (usercfg.InfoInterface().empty()) {
    preferredInterfaceNames.insert("org.nordugrid.ldapglue2");
  } else {
    preferredInterfaceNames.insert(usercfg.InfoInterface());
  }

  std::list<std::string> rejectDiscoveryURLs = getRejectDiscoveryURLsFromUserConfigAndCommandLine(usercfg, opt.rejectdiscovery);

  Arc::ComputingServiceUniq csu;
  Arc::ComputingServiceRetriever csr(usercfg, std::list<Arc::Endpoint>(), rejectDiscoveryURLs, preferredInterfaceNames);
  csr.addConsumer(csu);
  for (std::list<Arc::Endpoint>::const_iterator it = endpoints.begin(); it != endpoints.end(); it++) {
    csr.addEndpoint(*it);
  }
  csr.wait();

  std::list<Arc::ComputingServiceType> services = csu.getServices();
  for (std::list<Arc::ComputingServiceType>::const_iterator it = services.begin();
       it != services.end(); ++it) {
    if (opt.longlist) {
      std::cout << *it << std::endl;
    }
    else {
      std::cout << "Computing service: " << (**it).Name;
      if (!(**it).QualityLevel.empty()) {
        std::cout << " (" << (**it).QualityLevel << ")";
      }
      std::cout << std::endl;
      std::cout << "  Information endpoint: " << (**it).Cluster.str() << std::endl;
      for (std::map<int, Arc::ComputingEndpointType>::const_iterator itCE = it->ComputingEndpoint.begin();
           itCE != it->ComputingEndpoint.end(); ++itCE) {
         if (itCE->second->Capability.empty() ||
             itCE->second->Capability.count(Arc::Endpoint::GetStringForCapability(Arc::Endpoint::JOBSUBMIT)) ||
             itCE->second->Capability.count(Arc::Endpoint::GetStringForCapability(Arc::Endpoint::JOBCREATION)))
         {
           std::cout << "  Submission endpoint: " << itCE->second->URLString << " (status: " << itCE->second->HealthState << ", interface: " << itCE->second->InterfaceName << ")" << std::endl;           
         }
      }
    }
  }

  return 0;
}
