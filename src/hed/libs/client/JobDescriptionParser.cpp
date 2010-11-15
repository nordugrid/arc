// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/ArcConfig.h>
#include <arc/Logger.h>
#include <arc/client/JobDescription.h>
#include <arc/loader/FinderLoader.h>

#include "JobDescriptionParser.h"

namespace Arc {

  Logger JobDescriptionParser::logger(Logger::getRootLogger(),
                                      "JobDescriptionParser");

  JobDescriptionParser::JobDescriptionParser(const std::string& format) : format(format) {}

  JobDescriptionParser::~JobDescriptionParser() {}

  void JobDescriptionParser::AddHint(const std::string& key,const std::string& value) {
    if(key.empty()) return;
    hints[key] = value;
  }

  std::string JobDescriptionParser::GetHint(const std::string& key) const {
    std::map<std::string,std::string>::const_iterator h = hints.find(key);
    if(h == hints.end()) return "";
    return h->second;
  }

  void JobDescriptionParser::SetHints(const std::map<std::string,std::string>& hints) {
    this->hints = hints;
  }


  JobDescriptionParserLoader::JobDescriptionParserLoader()
    : Loader(BaseConfig().MakeConfig(Config()).Parent()) {
    factory_->scan(FinderLoader::GetLibrariesList(), jdpDescs);

    PluginsFactory::FilterByKind("HED:JobDescriptionParser", jdpDescs);
  }

  JobDescriptionParserLoader::~JobDescriptionParserLoader() {
    for (std::list<JobDescriptionParser*>::iterator it = jdps.begin();
         it != jdps.end(); it++)
      delete *it;
  }

  JobDescriptionParser* JobDescriptionParserLoader::load(const std::string& name) {
    if (name.empty())
      return NULL;

    if(!factory_->load(FinderLoader::GetLibrariesList(),
                       "HED:JobDescriptionParser", name)) {
      logger.msg(ERROR, "JobDescriptionParser plugin \"%s\" not found.", name);
      return NULL;
    }

    JobDescriptionParser *jdp = factory_->GetInstance<JobDescriptionParser>("HED:JobDescriptionParser", name, NULL, false);

    if (!jdp) {
      logger.msg(ERROR, "JobDescriptionParser %s could not be created", name);
      return NULL;
    }

    jdps.push_back(jdp);
    logger.msg(INFO, "Loaded JobDescriptionParser %s", name);
    return jdp;
  }

  JobDescriptionParserLoader::iterator::iterator(JobDescriptionParserLoader& jdpl) : jdpl(jdpl) {
    LoadNext();
   current = jdpl.jdps.begin();
  }

  JobDescriptionParserLoader::iterator& JobDescriptionParserLoader::iterator::operator++() {
    LoadNext();
    current++;
    return *this;
  }

  void JobDescriptionParserLoader::iterator::LoadNext() {
    while (!jdpl.jdpDescs.empty()) {
      JobDescriptionParser* loadedJDPL = NULL;

      while (!jdpl.jdpDescs.front().plugins.empty()) {
        loadedJDPL = jdpl.load(jdpl.jdpDescs.front().plugins.front().name);
        jdpl.jdpDescs.front().plugins.pop_front();
        if (loadedJDPL != NULL) {
          break;
        }
      }

      if (jdpl.jdpDescs.front().plugins.empty()) {
        jdpl.jdpDescs.pop_front();
      }

      if (loadedJDPL != NULL) {
        break;
      }
    }
  }
} // namespace Arc
