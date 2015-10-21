// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/ArcConfig.h>
#include <arc/Logger.h>
#include <arc/compute/JobDescription.h>
#include <arc/loader/FinderLoader.h>

#include "JobDescriptionParserPlugin.h"

namespace Arc {

  Logger JobDescriptionParserPlugin::logger(Logger::getRootLogger(),
                                      "JobDescriptionParserPlugin");

  JobDescriptionParserPlugin::JobDescriptionParserPlugin(PluginArgument* parg): Plugin(parg) {}

  JobDescriptionParserPlugin::~JobDescriptionParserPlugin() {}

  std::string& JobDescriptionParserPlugin::SourceLanguage(JobDescription& j) const { return j.sourceLanguage; }

  JobDescriptionParserPluginLoader::JobDescriptionParserPluginLoader()
    : Loader(BaseConfig().MakeConfig(Config()).Parent()), scaningDone(false) {}

  JobDescriptionParserPluginLoader::~JobDescriptionParserPluginLoader() {
    for (std::list<JobDescriptionParserPlugin*>::iterator it = jdps.begin();
         it != jdps.end(); it++)
      delete *it;
  }

  void JobDescriptionParserPluginLoader::scan() {
    factory_->scan(FinderLoader::GetLibrariesList(), jdpDescs);

    PluginsFactory::FilterByKind("HED:JobDescriptionParserPlugin", jdpDescs);
    scaningDone = true;
  }

  JobDescriptionParserPlugin* JobDescriptionParserPluginLoader::load(const std::string& name) {
    if (name.empty()) {
      return NULL;
    }

    if (!scaningDone) {
      scan();
    }

    if(!factory_->load(FinderLoader::GetLibrariesList(),
                       "HED:JobDescriptionParserPlugin", name)) {
      logger.msg(ERROR, "JobDescriptionParserPlugin plugin \"%s\" not found.", name);
      return NULL;
    }

    JobDescriptionParserPlugin *jdp = factory_->GetInstance<JobDescriptionParserPlugin>("HED:JobDescriptionParserPlugin", name, NULL, false);

    if (!jdp) {
      logger.msg(ERROR, "JobDescriptionParserPlugin %s could not be created", name);
      return NULL;
    }

    jdps.push_back(jdp);
    logger.msg(DEBUG, "Loaded JobDescriptionParserPlugin %s", name);
    return jdp;
  }

  JobDescriptionParserPluginLoader::iterator::iterator(JobDescriptionParserPluginLoader& jdpl) : jdpl(&jdpl) {
    LoadNext();
    current = this->jdpl->jdps.begin();
  }

  JobDescriptionParserPluginLoader::iterator& JobDescriptionParserPluginLoader::iterator::operator++() {
    LoadNext();
    current++;
    return *this;
  }

  void JobDescriptionParserPluginLoader::iterator::LoadNext() {
    if (!jdpl->scaningDone) {
      jdpl->scan();
    }

    while (!jdpl->jdpDescs.empty()) {
      JobDescriptionParserPlugin* loadedJDPL = NULL;

      while (!jdpl->jdpDescs.front().plugins.empty()) {
        loadedJDPL = jdpl->load(jdpl->jdpDescs.front().plugins.front().name);
        jdpl->jdpDescs.front().plugins.pop_front();
        if (loadedJDPL != NULL) {
          break;
        }
      }

      if (jdpl->jdpDescs.front().plugins.empty()) {
        jdpl->jdpDescs.pop_front();
      }

      if (loadedJDPL != NULL) {
        break;
      }
    }
  }
} // namespace Arc
