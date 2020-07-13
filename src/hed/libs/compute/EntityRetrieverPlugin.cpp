// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/compute/Job.h>

#include "EntityRetrieverPlugin.h"

namespace Arc {

  template<> Logger EntityRetrieverPluginLoader<Endpoint>::logger(Logger::getRootLogger(), "ServiceEndpointRetrieverPluginLoader");
  template<> Logger EntityRetrieverPluginLoader<ComputingServiceType>::logger(Logger::getRootLogger(), "TargetInformationRetrieverPluginLoader");
  template<> Logger EntityRetrieverPluginLoader<Job>::logger(Logger::getRootLogger(), "JobListRetrieverPluginLoader");

  template<> const std::string EntityRetrieverPlugin<Endpoint>::kind("HED:ServiceEndpointRetrieverPlugin");
  template<> const std::string EntityRetrieverPlugin<ComputingServiceType>::kind("HED:TargetInformationRetrieverPlugin");
  template<> const std::string EntityRetrieverPlugin<Job>::kind("HED:JobListRetrieverPlugin");

  template<typename T>
  EntityRetrieverPluginLoader<T>::~EntityRetrieverPluginLoader() {
    for (typename std::map<std::string, EntityRetrieverPlugin<T> *>::iterator it = plugins.begin();
         it != plugins.end(); it++) {
      delete it->second;
    }
  }

  template<typename T>
  EntityRetrieverPlugin<T>* EntityRetrieverPluginLoader<T>::load(const std::string& name) {
    if (plugins.find(name) != plugins.end()) {
      logger.msg(DEBUG, "Found %s %s (it was loaded already)", EntityRetrieverPlugin<T>::kind, name);
      return plugins[name];
    }

    if (name.empty()) {
      return NULL;
    }

    if(!factory_->load(FinderLoader::GetLibrariesList(), EntityRetrieverPlugin<T>::kind, name)) {
      logger.msg(ERROR, "Unable to locate the \"%s\" plugin. Please refer to installation instructions and check if package providing support for \"%s\" plugin is installed", name, name);
      logger.msg(DEBUG, "%s plugin \"%s\" not found.", EntityRetrieverPlugin<T>::kind, name, name);
      return NULL;
    }

     EntityRetrieverPlugin<T> *p = factory_->GetInstance< EntityRetrieverPlugin<T> >(EntityRetrieverPlugin<T>::kind, name, NULL, false);

    if (!p) {
      logger.msg(ERROR, "Unable to locate the \"%s\" plugin. Please refer to installation instructions and check if package providing support for \"%s\" plugin is installed", name, name);
      logger.msg(DEBUG, "%s %s could not be created.", EntityRetrieverPlugin<T>::kind, name, name);
      return NULL;
    }

    plugins[name] = p;
    logger.msg(DEBUG, "Loaded %s %s", EntityRetrieverPlugin<T>::kind, name);
    return p;
  }

  template<typename T>
  std::list<std::string> EntityRetrieverPluginLoader<T>::getListOfPlugins() {
    std::list<ModuleDesc> modules;
    PluginsFactory factory(BaseConfig().MakeConfig(Config()).Parent());
    factory.scan(FinderLoader::GetLibrariesList(), modules);
    PluginsFactory::FilterByKind(EntityRetrieverPlugin<T>::kind, modules);
    std::list<std::string> names;
    for (std::list<ModuleDesc>::const_iterator it = modules.begin(); it != modules.end(); it++) {
      for (std::list<PluginDesc>::const_iterator it2 = it->plugins.begin(); it2 != it->plugins.end(); it2++) {
        names.push_back(it2->name);
      }
    }
    return names;
  }

  ServiceEndpointRetrieverPlugin::ServiceEndpointRetrieverPlugin(PluginArgument* parg): EntityRetrieverPlugin<Endpoint>(parg) {};
  TargetInformationRetrieverPlugin::TargetInformationRetrieverPlugin(PluginArgument* parg): EntityRetrieverPlugin<ComputingServiceType>(parg) {};
  JobListRetrieverPlugin::JobListRetrieverPlugin(PluginArgument* parg): EntityRetrieverPlugin<Job>(parg) {};

  template class EntityRetrieverPluginLoader<Endpoint>;
  template class EntityRetrieverPluginLoader<ComputingServiceType>;
  template class EntityRetrieverPluginLoader<Job>;

} // namespace Arc
