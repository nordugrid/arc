// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <algorithm>
#include <fstream>
#include <iostream>
#include <list>
#include <string>
#include <map>

#include <unistd.h>
#include <glibmm/fileutils.h>
#include <glibmm.h>

#include <arc/IString.h>
#include <arc/UserConfig.h>
#include <arc/data/URLMap.h>
#include <arc/loader/FinderLoader.h>

#include "JobController.h"

namespace Arc {

  Logger JobController::logger(Logger::getRootLogger(), "JobController");

  std::map<std::string, std::string> JobControllerLoader::interfacePluginMap;

  JobController::JobController(const UserConfig& usercfg, PluginArgument* parg)
    : Plugin(parg),
      usercfg(usercfg) {}

  JobControllerLoader::JobControllerLoader()
    : Loader(BaseConfig().MakeConfig(Config()).Parent()) {}

  JobControllerLoader::~JobControllerLoader() {
    for (std::multimap<std::string, JobController*>::iterator it = jobcontrollers.begin(); it != jobcontrollers.end(); it++)
      delete it->second;
  }

  void JobControllerLoader::initialiseInterfacePluginMap(const UserConfig& uc) {
    std::list<ModuleDesc> modules;
    PluginsFactory factory(BaseConfig().MakeConfig(Config()).Parent());
    factory.scan(FinderLoader::GetLibrariesList(), modules);
    PluginsFactory::FilterByKind("HED:JobController", modules);
    std::list<std::string> availablePlugins;
    for (std::list<ModuleDesc>::const_iterator it = modules.begin(); it != modules.end(); ++it) {
      for (std::list<PluginDesc>::const_iterator it2 = it->plugins.begin(); it2 != it->plugins.end(); ++it2) {
        availablePlugins.push_back(it2->name);
      }
    }

    if (interfacePluginMap.empty()) {
      // Map supported interfaces to available plugins.
      for (std::list<std::string>::iterator itT = availablePlugins.begin(); itT != availablePlugins.end(); ++itT) {
        JobController* p = load(*itT, uc);
  
        if (!p) {
          continue;
        }
  
        for (std::list<std::string>::const_iterator itI = p->SupportedInterfaces().begin(); itI != p->SupportedInterfaces().end(); ++itI) {
          if (!itT->empty()) { // Do not allow empty interface.
            // If two plugins supports two identical interfaces, then only the last will appear in the map.
            interfacePluginMap[*itI] = *itT;
          }
        }
      }
    }
  }

  JobController* JobControllerLoader::load(const std::string& name, const UserConfig& uc) {
    if (name.empty())
      return NULL;

    if(!factory_->load(FinderLoader::GetLibrariesList(), "HED:JobController", name)) {
      logger.msg(ERROR, "Unable to locate the \"%s\" plugin. Please refer to installation instructions and check if package providing support for \"%s\" plugin is installed", name, name);
      logger.msg(DEBUG, "JobController plugin \"%s\" not found.", name);
      return NULL;
    }

    JobControllerPluginArgument arg(uc);
    JobController *jobcontroller = factory_->GetInstance<JobController>("HED:JobController", name, &arg, false);

    if (!jobcontroller) {
      logger.msg(ERROR, "Unable to locate the \"%s\" plugin. Please refer to installation instructions and check if package providing support for \"%s\" plugin is installed", name, name);
      logger.msg(DEBUG, "JobController %s could not be created", name);
      return NULL;
    }

    jobcontrollers.insert(std::pair<std::string, JobController*>(name, jobcontroller));
    logger.msg(DEBUG, "Loaded JobController %s", name);
    return jobcontroller;
  }

  JobController* JobControllerLoader::loadByInterfaceName(const std::string& name, const UserConfig& uc) {
    if (interfacePluginMap.empty()) {
      initialiseInterfacePluginMap(uc);
    }
    
    std::map<std::string, std::string>::const_iterator itPN = interfacePluginMap.find(name);
    if (itPN != interfacePluginMap.end()) {
      std::map<std::string, JobController*>::iterator itJC = jobcontrollers.find(itPN->second);
      if (itJC != jobcontrollers.end()) {
        return itJC->second;
      }
      
      return load(itPN->second, uc);
    }

    return NULL;
  }
} // namespace Arc
