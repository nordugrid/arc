// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/ArcConfig.h>
#include <arc/Logger.h>
#include <arc/loader/FinderLoader.h>

#include "TargetInformationRetrieverPlugin.h"

namespace Arc {
  Logger TargetInformationRetrieverPluginLoader::logger(Logger::getRootLogger(), "TargetInformationRetrieverPluginLoader");

  TargetInformationRetrieverPluginLoader::TargetInformationRetrieverPluginLoader()
    : Loader(BaseConfig().MakeConfig(Config()).Parent()) {}

  TargetInformationRetrieverPluginLoader::~TargetInformationRetrieverPluginLoader() {
    for (std::map<std::string, TargetInformationRetrieverPlugin*>::iterator it = plugins.begin();
         it != plugins.end(); it++) {
      delete it->second;
    }
  }

  TargetInformationRetrieverPlugin* TargetInformationRetrieverPluginLoader::load(const std::string& name) {
    if (plugins.find(name) != plugins.end()) {
      logger.msg(DEBUG, "Found TargetInformationRetrieverPlugin %s (it was loaded already)", name);
      return plugins[name];
    }

    if (name.empty()) {
      return NULL;
    }

    if(!factory_->load(FinderLoader::GetLibrariesList(), "HED:TargetInformationRetrieverPlugin", name)) {
      logger.msg(ERROR, "Unable to locate the \"%s\" plugin. Please refer to installation instructions and check if package providing support for %s plugin is installed", name, name);
      logger.msg(DEBUG, "TargetInformationRetrieverPlugin plugin \"%s\" not found.", name, name);
      return NULL;
    }

    TargetInformationRetrieverPlugin *p = factory_->GetInstance<TargetInformationRetrieverPlugin>("HED:TargetInformationRetrieverPlugin", name, NULL, false);

    if (!p) {
      logger.msg(ERROR, "Unable to locate the \"%s\" plugin. Please refer to installation instructions and check if package providing support for \"%s\" plugin is installed", name, name);
      logger.msg(DEBUG, "TargetInformationRetrieverPlugin %s could not be created.", name, name);
      return NULL;
    }

    plugins[name] = p;
    logger.msg(DEBUG, "Loaded TargetInformationRetrieverPlugin %s", name);
    return p;
  }

  std::list<std::string> TargetInformationRetrieverPluginLoader::getListOfPlugins() {
    std::list<ModuleDesc> modules;
    PluginsFactory factory(BaseConfig().MakeConfig(Config()).Parent());
    factory.scan(FinderLoader::GetLibrariesList(), modules);
    PluginsFactory::FilterByKind("HED:TargetInformationRetrieverPlugin", modules);
    std::list<std::string> names;
    for (std::list<ModuleDesc>::iterator it = modules.begin(); it != modules.end(); it++) {
      for (std::list<PluginDesc>::iterator it2 = it->plugins.begin(); it2 != it->plugins.end(); it2++) {
        names.push_back(it2->name);
      }
    }
    return names;
  }

}
