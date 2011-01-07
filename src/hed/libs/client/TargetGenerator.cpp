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

  TargetGenerator::TargetGenerator(const UserConfig& usercfg, unsigned int startDiscovery)
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
    for (URLListMap::const_iterator it = usercfg.GetSelectedServices(COMPUTING).begin();
         it != usercfg.GetSelectedServices(COMPUTING).end(); it++)
      for (std::list<URL>::const_iterator it2 = it->second.begin();
           it2 != it->second.end(); it2++) {
        if (it->first == "*") {
          for (std::list<ModuleDesc>::iterator it3 = modules.begin();
               it3 != modules.end(); it3++)
            for (std::list<PluginDesc>::iterator it4 = it3->plugins.begin();
                 it4 != it3->plugins.end(); it4++)
              if (pluginLoaded.find(it4->name) == pluginLoaded.end() ||
                  pluginLoaded[it4->name])
                pluginLoaded[it4->name] = loader.load(it4->name, usercfg,
                                                      *it2, COMPUTING);
        }
        else
          if (pluginLoaded.find(it->first) == pluginLoaded.end() ||
              pluginLoaded[it->first])
            pluginLoaded[it->first] = loader.load(it->first, usercfg,
                                                  *it2, COMPUTING);
      }

    for (URLListMap::const_iterator it =
           usercfg.GetSelectedServices(INDEX).begin();
         it != usercfg.GetSelectedServices(INDEX).end(); it++) {
      for (std::list<URL>::const_iterator it2 = it->second.begin();
           it2 != it->second.end(); it2++) {
        if (it->first == "*") {
          for (std::list<ModuleDesc>::iterator it3 = modules.begin();
               it3 != modules.end(); it3++)
            for (std::list<PluginDesc>::iterator it4 = it3->plugins.begin();
                 it4 != it3->plugins.end(); it4++)
              if (pluginLoaded.find(it4->name) == pluginLoaded.end() ||
                  pluginLoaded[it4->name])
                pluginLoaded[it4->name] = loader.load(it4->name, usercfg,
                                                      *it2, INDEX);
        }
        else
          if (pluginLoaded.find(it->first) == pluginLoaded.end() ||
              pluginLoaded[it->first])
            pluginLoaded[it->first] = loader.load(it->first, usercfg,
                                                  *it2, INDEX);
      }
    }

    if ((startDiscovery & 1) == 1)
      GetExecutionTargets();
    if ((startDiscovery & 2) == 2)
      GetJobs();
  }

  TargetGenerator::~TargetGenerator() {
    if (xmlFoundJobs.size() > 0) {
      for (std::list<XMLNode*>::iterator it = xmlFoundJobs.begin();
           it != xmlFoundJobs.end(); it++) {
        delete *it;
      }
    }
  }

  void TargetGenerator::GetTargets(int targetType, int detailLevel) {
    logger.msg(WARNING, "The TargetGenerator::GetTargets method is DEPRECATED, use the GetExecutionTargets or GetJobs method instead.");

    logger.msg(VERBOSE, "Running resource (target) discovery");

    if (targetType == 0) {
      GetExecutionTargets();
    }
    else if (targetType == 1) {
      GetJobs();
    }
  }

  void TargetGenerator::GetExecutionTargets() {
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

  void TargetGenerator::GetJobs() {
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

  const std::list<ExecutionTarget>& TargetGenerator::FoundTargets() const {
    return foundTargets;
  }

  std::list<ExecutionTarget>& TargetGenerator::ModifyFoundTargets() {
    logger.msg(WARNING, "The TargetGenerator::ModifyFoundTargets method is DEPRECATED, use the FoundTargets method instead.");
    return foundTargets;
  }

  const std::list<XMLNode*>& TargetGenerator::FoundJobs() const {
    logger.msg(WARNING, "The TargetGenerator::FoundJobs method is DEPRECATED, use the GetFoundJobs method instead.");

    if (foundJobs.size() != xmlFoundJobs.size()) {
      std::list<Job>::const_iterator it = foundJobs.begin();
      if (foundJobs.size() > xmlFoundJobs.size()) {
        for (std::list<XMLNode*>::const_iterator xit = xmlFoundJobs.begin(); xit != xmlFoundJobs.end(); xit++) {
          it++;
        }
      }

      for (; it != foundJobs.end(); it++) {
        // Ugly hack used here in order to preserve API, i.e. const-ness of the method.
        const_cast<TargetGenerator*>(this)->xmlFoundJobs.push_back(new XMLNode(NS(), "Job"));
        it->ToXML(*(const_cast<TargetGenerator*>(this)->xmlFoundJobs.back()));
      }
    }

    return xmlFoundJobs;
  }

  const std::list<Job>& TargetGenerator::GetFoundJobs() const {
    return foundJobs;
  }

  bool TargetGenerator::AddService(const std::string flavour, const URL& url) {

    for (URLListMap::const_iterator it = usercfg.GetRejectedServices(COMPUTING).begin();
         it != usercfg.GetRejectedServices(COMPUTING).end(); it++)
      if (std::find(it->second.begin(), it->second.end(), url) !=
          it->second.end()) {
        logger.msg(INFO, "Rejecting service: %s", url.str());
        return false;
      }

    bool added = false;
    Glib::Mutex::Lock serviceLock(serviceMutex);
    if (std::find(foundServices[flavour].begin(), foundServices[flavour].end(),
                  url) == foundServices[flavour].end()) {
      foundServices[flavour].push_back(url);
      added = true;
    }
    return added;
  }

  bool TargetGenerator::AddIndexServer(const std::string flavour, const URL& url) {

    for (URLListMap::const_iterator it = usercfg.GetRejectedServices(INDEX).begin();
         it != usercfg.GetRejectedServices(INDEX).end(); it++)
      if (std::find(it->second.begin(), it->second.end(), url) !=
          it->second.end()) {
        logger.msg(INFO, "Rejecting service: %s", url.str());
        return false;
      }

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

  void TargetGenerator::AddJob(const XMLNode& job) {
    logger.msg(WARNING, "The TargetGenerator::AddJob(const XMLNode&) method is DEPRECATED, use the AddJob(const Job&) method instead.");
    Glib::Mutex::Lock jobLock(jobMutex);
    Job j = job;
    foundJobs.push_back(j);
  }

  void TargetGenerator::AddJob(const Job& job) {
    Glib::Mutex::Lock jobLock(jobMutex);
    foundJobs.push_back(job);
  }

  void TargetGenerator::PrintTargetInfo(bool longlist) const {
    logger.msg(WARNING, "The TargetGenerator::PrintTargetInfo method is DEPRECATED, use the TargetGenerator::SaveTargetInfoToStream method instead.");
    SaveTargetInfoToStream(std::cout, longlist);
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
