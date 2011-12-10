// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <algorithm>
#include <iostream>

#include <arc/ArcConfig.h>
#include <arc/IString.h>
#include <arc/Logger.h>
#include <arc/StringConv.h>
#include <arc/XMLNode.h>
#include <arc/client/ClientInterface.h>
#include <arc/client/Job.h>
#include <arc/client/TargetGenerator.h>
#include <arc/loader/FinderLoader.h>
#include <arc/UserConfig.h>

namespace Arc {

  Logger TargetGenerator::logger(Logger::getRootLogger(), "TargetGenerator");

  TargetGenerator::TargetGenerator(const UserConfig& usercfg, unsigned int startRetrieval)
    : usercfg(usercfg) {

    std::list<std::string> libraries = FinderLoader::GetLibrariesList();
    std::list<ModuleDesc> modules;
    Config cfg;
    BaseConfig().MakeConfig(cfg);
    PluginsFactory factory(cfg);
    factory.scan(libraries, modules);
    PluginsFactory::FilterByKind("HED:TargetRetriever", modules);

    /* When loading a specific middleware plugin fails, subsequent loads
     * should fail aswell. Therefore it should be unecessary to load the
     * same plugin multiple times, if it failed the first time.
     */
    std::map<std::string, bool> pluginLoaded;
    for (std::list<std::string>::const_iterator it =
           usercfg.GetSelectedServices(COMPUTING).begin();
         it != usercfg.GetSelectedServices(COMPUTING).end(); it++) {
      std::string::size_type pos = it->find(':');
      std::string flavour = it->substr(0, pos);
      std::string service = it->substr(pos + 1);
      if (flavour == "*") {
        for (std::list<ModuleDesc>::iterator it3 = modules.begin();
             it3 != modules.end(); it3++)
          for (std::list<PluginDesc>::iterator it4 = it3->plugins.begin();
               it4 != it3->plugins.end(); it4++)
            if (pluginLoaded.find(it4->name) == pluginLoaded.end() ||
                pluginLoaded[it4->name])
              pluginLoaded[it4->name] = loader.load(it4->name, usercfg,
                                                    service, COMPUTING);
      }
      else
        if (pluginLoaded.find(flavour) == pluginLoaded.end() ||
            pluginLoaded[flavour])
          pluginLoaded[flavour] = loader.load(flavour, usercfg,
                                              service, COMPUTING);
    }

    for (std::list<std::string>::const_iterator it =
           usercfg.GetSelectedServices(INDEX).begin();
         it != usercfg.GetSelectedServices(INDEX).end(); it++) {
      std::string::size_type pos = it->find(':');
      std::string flavour = it->substr(0, pos);
      std::string service = it->substr(pos + 1);
      if (flavour == "*") {
        for (std::list<ModuleDesc>::iterator it3 = modules.begin();
             it3 != modules.end(); it3++)
          for (std::list<PluginDesc>::iterator it4 = it3->plugins.begin();
               it4 != it3->plugins.end(); it4++)
            if (pluginLoaded.find(it4->name) == pluginLoaded.end() ||
                pluginLoaded[it4->name])
              pluginLoaded[it4->name] = loader.load(it4->name, usercfg,
                                                    service, INDEX);
      }
      else
        if (pluginLoaded.find(flavour) == pluginLoaded.end() ||
            pluginLoaded[flavour])
          pluginLoaded[flavour] = loader.load(flavour, usercfg,
                                              service, INDEX);
    }

    if ((startRetrieval & 1) == 1) {
      RetrieveExecutionTargets();
    }
    if ((startRetrieval & 2) == 2) {
      RetrieveJobs();
    }
  }

  TargetGenerator::~TargetGenerator() {}

  void TargetGenerator::RetrieveExecutionTargets() {
    for (std::list<TargetRetriever*>::const_iterator it = loader.GetTargetRetrievers().begin();
         it != loader.GetTargetRetrievers().end(); it++) {
      (*it)->GetExecutionTargets(*this);
    }

    {
      threadCounter.wait();
    }

    logger.msg(INFO, "Found %ld targets", foundTargets.size());

    if (logger.getThreshold() == VERBOSE || logger.getThreshold() == DEBUG) {
      for (std::list<ExecutionTarget>::iterator iter = foundTargets.begin();
           iter != foundTargets.end(); iter++) {
        logger.msg(VERBOSE, "Resource: %s", iter->DomainName);
        logger.msg(VERBOSE, "Health State: %s", iter->HealthState);
      }
    }
  }

  void TargetGenerator::RetrieveJobs() {
    for (std::list<TargetRetriever*>::const_iterator it =
           loader.GetTargetRetrievers().begin();
         it != loader.GetTargetRetrievers().end(); it++) {
      (*it)->GetJobs(*this);
    }

    {
      threadCounter.wait();
    }

    logger.msg(INFO, "Found %ld jobs", foundJobs.size());
  }

  bool TargetGenerator::AddService(const std::string& flavour,
                                   const URL& url) {
    bool added = false;
    Glib::Mutex::Lock serviceLock(serviceMutex);
    if (std::find(foundServices[flavour].begin(), foundServices[flavour].end(),
                  url) == foundServices[flavour].end()) {
      foundServices[flavour].push_back(url);
      added = true;
    }
    return added;
  }

  bool TargetGenerator::AddIndexServer(const std::string& flavour,
                                       const URL& url) {
    bool added = false;
    Glib::Mutex::Lock indexServerLock(indexServerMutex);
    if (std::find(foundIndexServers[flavour].begin(),
                  foundIndexServers[flavour].end(), url) ==
        foundIndexServers[flavour].end()) {
      foundIndexServers[flavour].push_back(url);
      added = true;
    }
    return added;
  }

  void TargetGenerator::AddTarget(const ExecutionTarget& target) {
    Glib::Mutex::Lock targetLock(targetMutex);
    foundTargets.push_back(target);
  }

  void TargetGenerator::AddJob(const Job& job) {
    Glib::Mutex::Lock jobLock(jobMutex);
    foundJobs.push_back(job);
  }

  void TargetGenerator::SaveTargetInfoToStream(std::ostream& out, bool longlist) const {
    for (std::list<ExecutionTarget>::const_iterator cli = foundTargets.begin();
         cli != foundTargets.end(); cli++)
      cli->SaveToStream(out, longlist);
  }

  SimpleCounter& TargetGenerator::ServiceCounter(void) {
    return threadCounter;
  }

} // namespace Arc
