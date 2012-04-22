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

#include "JobControllerPlugin.h"

namespace Arc {

  Logger JobControllerPlugin::logger(Logger::getRootLogger(), "JobControllerPlugin");

  std::map<std::string, std::string> JobControllerPluginLoader::interfacePluginMap;

  void JobControllerPlugin::UpdateJobs(std::list<Job*>& jobs, bool isGrouped) const {
    std::list<URL> idsProcessed, idsNotProcessed;
    return UpdateJobs(jobs, idsProcessed, idsNotProcessed, isGrouped);
  };

  bool JobControllerPlugin::CleanJobs(const std::list<Job*>& jobs, bool isGrouped) const {
    std::list<URL> idsProcessed, idsNotProcessed;
    return CleanJobs(jobs, idsProcessed, idsNotProcessed, isGrouped);
  }
  
  bool JobControllerPlugin::CancelJobs(const std::list<Job*>& jobs, bool isGrouped) const {
    std::list<URL> idsProcessed, idsNotProcessed;
    return CancelJobs(jobs, idsProcessed, idsNotProcessed, isGrouped);
  }
  
  bool JobControllerPlugin::RenewJobs(const std::list<Job*>& jobs, bool isGrouped) const {
    std::list<URL> idsProcessed, idsNotProcessed;
    return RenewJobs(jobs, idsProcessed, idsNotProcessed, isGrouped);
  }
  
  bool JobControllerPlugin::ResumeJobs(const std::list<Job*>& jobs, bool isGrouped) const {
    std::list<URL> idsProcessed, idsNotProcessed;
    return ResumeJobs(jobs, idsProcessed, idsNotProcessed, isGrouped);
  }

  JobControllerPluginLoader::JobControllerPluginLoader()
    : Loader(BaseConfig().MakeConfig(Config()).Parent()) {}

  JobControllerPluginLoader::~JobControllerPluginLoader() {
    for (std::multimap<std::string, JobControllerPlugin*>::iterator it = jobcontrollers.begin(); it != jobcontrollers.end(); it++)
      delete it->second;
  }

  void JobControllerPluginLoader::initialiseInterfacePluginMap(const UserConfig& uc) {
    std::list<ModuleDesc> modules;
    PluginsFactory factory(BaseConfig().MakeConfig(Config()).Parent());
    factory.scan(FinderLoader::GetLibrariesList(), modules);
    PluginsFactory::FilterByKind("HED:JobControllerPlugin", modules);
    std::list<std::string> availablePlugins;
    for (std::list<ModuleDesc>::const_iterator it = modules.begin(); it != modules.end(); ++it) {
      for (std::list<PluginDesc>::const_iterator it2 = it->plugins.begin(); it2 != it->plugins.end(); ++it2) {
        availablePlugins.push_back(it2->name);
      }
    }

    if (interfacePluginMap.empty()) {
      // Map supported interfaces to available plugins.
      for (std::list<std::string>::iterator itT = availablePlugins.begin(); itT != availablePlugins.end(); ++itT) {
        JobControllerPlugin* p = load(*itT, uc);
  
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

  JobControllerPlugin* JobControllerPluginLoader::load(const std::string& name, const UserConfig& uc) {
    if (name.empty())
      return NULL;

    if(!factory_->load(FinderLoader::GetLibrariesList(), "HED:JobControllerPlugin", name)) {
      logger.msg(ERROR, "Unable to locate the \"%s\" plugin. Please refer to installation instructions and check if package providing support for \"%s\" plugin is installed", name, name);
      logger.msg(DEBUG, "JobControllerPlugin plugin \"%s\" not found.", name);
      return NULL;
    }

    JobControllerPluginPluginArgument arg(uc);
    JobControllerPlugin *jobcontroller = factory_->GetInstance<JobControllerPlugin>("HED:JobControllerPlugin", name, &arg, false);

    if (!jobcontroller) {
      logger.msg(ERROR, "Unable to locate the \"%s\" plugin. Please refer to installation instructions and check if package providing support for \"%s\" plugin is installed", name, name);
      logger.msg(DEBUG, "JobControllerPlugin %s could not be created", name);
      return NULL;
    }

    jobcontrollers.insert(std::pair<std::string, JobControllerPlugin*>(name, jobcontroller));
    logger.msg(DEBUG, "Loaded JobControllerPlugin %s", name);
    return jobcontroller;
  }

  JobControllerPlugin* JobControllerPluginLoader::loadByInterfaceName(const std::string& name, const UserConfig& uc) {
    if (interfacePluginMap.empty()) {
      initialiseInterfacePluginMap(uc);
    }
    
    std::map<std::string, std::string>::const_iterator itPN = interfacePluginMap.find(name);
    if (itPN != interfacePluginMap.end()) {
      std::map<std::string, JobControllerPlugin*>::iterator itJC = jobcontrollers.find(itPN->second);
      if (itJC != jobcontrollers.end()) {
        return itJC->second;
      }
      
      return load(itPN->second, uc);
    }

    return NULL;
  }
} // namespace Arc
