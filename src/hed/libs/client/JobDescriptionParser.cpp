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

  JobDescriptionParser::JobDescriptionParser() {}

  JobDescriptionParser::~JobDescriptionParser() {}

  std::string& JobDescriptionParser::SourceLanguage(JobDescription& j) const { return j.sourceLanguage; }

  JobDescription JobDescriptionParser::Parse(const std::string& source) const {
    JobDescription jobdesc;
    if (Parse(source, jobdesc)) {
      return jobdesc;
    }

    return JobDescription();
  }

  std::string JobDescriptionParser::UnParse(const JobDescription& job) const {
    std::string output;
    if (UnParse(job, output, "")) {
      return output;
    }

    return "";
  }

  JobDescriptionParserLoader::JobDescriptionParserLoader()
    : Loader(BaseConfig().MakeConfig(Config()).Parent()), scaningDone(false) {}

  JobDescriptionParserLoader::~JobDescriptionParserLoader() {
    for (std::list<JobDescriptionParser*>::iterator it = jdps.begin();
         it != jdps.end(); it++)
      delete *it;
  }

  void JobDescriptionParserLoader::scan() {
    factory_->scan(FinderLoader::GetLibrariesList(), jdpDescs);

    PluginsFactory::FilterByKind("HED:JobDescriptionParser", jdpDescs);
    scaningDone = true;
  }

  JobDescriptionParser* JobDescriptionParserLoader::load(const std::string& name) {
    if (name.empty()) {
      return NULL;
    }

    if (!scaningDone) {
      scan();
    }

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
    if (!jdpl.scaningDone) {
      jdpl.scan();
    }

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
