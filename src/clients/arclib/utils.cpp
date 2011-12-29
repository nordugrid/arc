// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/ArcConfig.h>
#include <arc/IString.h>
#include <arc/loader/FinderLoader.h>
#include <arc/loader/Plugin.h>

#include "utils.h"

void showplugins(const std::string& program, const std::list<std::string>& types, Arc::Logger& logger, const std::string& chosenBroker) {

  for (std::list<std::string>::const_iterator itType = types.begin();
       itType != types.end(); ++itType) {
    if (*itType == "HED:Submitter") {
      std::cout << Arc::IString("Types of execution services %s is able to submit jobs to:", program) << std::endl;
    }
    else if (*itType == "HED:TargetRetriever") {
      std::cout << Arc::IString("Types of index and information services which %s is able collect information from:", program) << std::endl;
    }
    else if (*itType == "HED:JobController") {
      std::cout << Arc::IString("Types of services %s is able to manage jobs at:", program) << std::endl;
    }
    else if (*itType == "HED:JobDescriptionParser") {
      std::cout << Arc::IString("Job description languages supported by %s:", program) << std::endl;
    }
    else if (*itType == "HED:Broker") {
      std::cout << Arc::IString("Brokers available to %s:", program) << std::endl;
    }
    
    std::list<Arc::ModuleDesc> modules;
    Arc::PluginsFactory pf(Arc::BaseConfig().MakeConfig(Arc::Config()).Parent());

    bool isDefaultBrokerLocated = false;
    pf.scan(Arc::FinderLoader::GetLibrariesList(), modules);
    Arc::PluginsFactory::FilterByKind(*itType, modules);
    for (std::list<Arc::ModuleDesc>::iterator itMod = modules.begin();
         itMod != modules.end(); itMod++) {
      for (std::list<Arc::PluginDesc>::iterator itPlug = itMod->plugins.begin();
           itPlug != itMod->plugins.end(); itPlug++) {
        std::cout << "  " << itPlug->name;
        if (*itType == "HED:Broker" && itPlug->name == chosenBroker) {
          std::cout << " (default)";
          isDefaultBrokerLocated = true;
        }
        std::cout << " - " << itPlug->description << std::endl;
      }
    }

    if (*itType == "HED:Broker" && !isDefaultBrokerLocated) {
      logger.msg(Arc::WARNING, "Default broker (%s) is not available. When using %s a broker should be specified explicitly (-b option).", chosenBroker, program);
    }
  }
}
