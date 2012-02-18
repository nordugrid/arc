// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/ArcConfig.h>
#include <arc/Logger.h>
#include <arc/loader/FinderLoader.h>

#include "ServiceEndpointRetrieverPlugin.h"

namespace Arc {
  Logger ServiceEndpointRetrieverPluginLoader::logger(Logger::getRootLogger(), "ServiceEndpointRetrieverPluginLoader");

  ServiceEndpointRetrieverPluginLoader::ServiceEndpointRetrieverPluginLoader()
    : Loader(BaseConfig().MakeConfig(Config()).Parent()) {}

  ServiceEndpointRetrieverPluginLoader::~ServiceEndpointRetrieverPluginLoader() {
    for (std::map<std::string, ServiceEndpointRetrieverPlugin*>::iterator it = plugins.begin();
         it != plugins.end(); it++) {
      delete it->second;
    }
  }

  ServiceEndpointRetrieverPlugin* ServiceEndpointRetrieverPluginLoader::load(const std::string& name) {
    if (plugins.find(name) != plugins.end()) {
      logger.msg(DEBUG, "Found ServiceEndpointRetrieverPlugin %s (it was loaded already)", name);
      return plugins[name];
    }
        
    if (name.empty()) {
      return NULL;
    }

    if(!factory_->load(FinderLoader::GetLibrariesList(), "HED:ServiceEndpointRetrieverPlugin", name)) {
      logger.msg(ERROR, "Unable to locate the \"%s\" plugin. Please refer to installation instructions and check if package providing support for %s plugin is installed", name, name);
      logger.msg(DEBUG, "ServiceEndpointRetrieverPlugin plugin \"%s\" not found.", name, name);
      return NULL;
    }

    ServiceEndpointRetrieverPlugin *p = factory_->GetInstance<ServiceEndpointRetrieverPlugin>("HED:ServiceEndpointRetrieverPlugin", name, NULL, false);

    if (!p) {
      logger.msg(ERROR, "Unable to locate the \"%s\" plugin. Please refer to installation instructions and check if package providing support for \"%s\" plugin is installed", name, name);
      logger.msg(DEBUG, "ServiceEndpointRetrieverPlugin %s could not be created.", name, name);
      return NULL;
    }

    plugins[name] = p;
    logger.msg(DEBUG, "Loaded ServiceEndpointRetrieverPlugin %s", name);
    return p;
  }

  std::list<std::string> ServiceEndpointRetrieverPluginLoader::getListOfPlugins() {
    std::list<ModuleDesc> modules;
    PluginsFactory factory(BaseConfig().MakeConfig(Config()).Parent());
    factory.scan(FinderLoader::GetLibrariesList(), modules);
    PluginsFactory::FilterByKind("HED:ServiceEndpointRetrieverPlugin", modules);
    std::list<std::string> names;
    for (std::list<ModuleDesc>::iterator it = modules.begin(); it != modules.end(); it++) {
      for (std::list<PluginDesc>::iterator it2 = it->plugins.begin(); it2 != it->plugins.end(); it2++) {
        names.push_back(it2->name);
      }
    }
    return names;
  }

}
