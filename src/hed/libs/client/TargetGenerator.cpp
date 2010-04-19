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
#include <arc/client/TargetGenerator.h>
#include <arc/client/ClientInterface.h>
#include <arc/UserConfig.h>

namespace Arc {

  Logger TargetGenerator::logger(Logger::getRootLogger(), "TargetGenerator");

  TargetGenerator::TargetGenerator(const UserConfig& usercfg)
    : usercfg(usercfg) {

    /* When loading a specific middleware plugin fails, subsequent loads
     * should fail aswell. Therefore it should be unecessary to load the
     * same plugin multiple times, if it failed the first time.
     */
    std::map<std::string, bool> pluginLoaded;
    for (URLListMap::const_iterator it = usercfg.GetSelectedServices(COMPUTING).begin();
         it != usercfg.GetSelectedServices(COMPUTING).end(); it++)
      for (std::list<URL>::const_iterator it2 = it->second.begin();
           it2 != it->second.end(); it2++) {
        if (pluginLoaded[it->first] = (loader.load(it->first, usercfg, *it2, COMPUTING) != NULL))
          break;
      }

    for (URLListMap::const_iterator it = usercfg.GetSelectedServices(INDEX).begin();
         it != usercfg.GetSelectedServices(INDEX).end(); it++) {
      if (pluginLoaded.find(it->first) != pluginLoaded.end() && !pluginLoaded[it->first]) // Do not try to load if it failed above.
        continue;
      for (std::list<URL>::const_iterator it2 = it->second.begin();
           it2 != it->second.end(); it2++) {
        if (loader.load(it->first, usercfg, *it2, INDEX) == NULL)
          break;
      }
    }
  }

  TargetGenerator::~TargetGenerator() {

    if (foundJobs.size() > 0)
      for (std::list<XMLNode*>::iterator it = foundJobs.begin();
           it != foundJobs.end(); it++)
        delete *it;
  }

  void TargetGenerator::GetTargets(int targetType, int detailLevel) {

    logger.msg(VERBOSE, "Running resource (target) discovery");

    for (std::list<TargetRetriever*>::const_iterator it =
           loader.GetTargetRetrievers().begin();
         it != loader.GetTargetRetrievers().end(); it++)
      (*it)->GetTargets(*this, targetType, detailLevel);

    {
      threadCounter.wait();
    }

    logger.msg(INFO, "Found %ld targets", foundTargets.size());

    for (std::list<ExecutionTarget>::iterator iter = foundTargets.begin();
         iter != foundTargets.end(); iter++) {
      logger.msg(VERBOSE, "Cluster: %s", iter->DomainName);
      logger.msg(VERBOSE, "Health State: %s", iter->HealthState);
    }

  }

  const std::list<ExecutionTarget>& TargetGenerator::FoundTargets() const {
    return foundTargets;
  }

  std::list<ExecutionTarget>& TargetGenerator::ModifyFoundTargets() {
    return foundTargets;
  }

  const std::list<XMLNode*>& TargetGenerator::FoundJobs() const {
    return foundJobs;
  }

  bool TargetGenerator::AddService(const URL& url) {

    for (URLListMap::const_iterator it = usercfg.GetRejectedServices(COMPUTING).begin();
         it != usercfg.GetRejectedServices(COMPUTING).end(); it++)
      if (std::find(it->second.begin(), it->second.end(), url) !=
          it->second.end()) {
        logger.msg(INFO, "Rejecting service: %s", url.str());
        return false;
      }

    bool added = false;
    Glib::Mutex::Lock serviceLock(serviceMutex);
    if (std::find(foundServices.begin(), foundServices.end(), url) ==
        foundServices.end()) {
      foundServices.push_back(url);
      added = true;
    }
    return added;
  }

  bool TargetGenerator::AddIndexServer(const URL& url) {

    for (URLListMap::const_iterator it = usercfg.GetRejectedServices(INDEX).begin();
         it != usercfg.GetRejectedServices(INDEX).end(); it++)
      if (std::find(it->second.begin(), it->second.end(), url) !=
          it->second.end()) {
        logger.msg(INFO, "Rejecting service: %s", url.str());
        return false;
      }

    bool added = false;
    Glib::Mutex::Lock indexServerLock(indexServerMutex);
    if (std::find(foundIndexServers.begin(), foundIndexServers.end(), url) ==
        foundIndexServers.end()) {
      foundIndexServers.push_back(url);
      added = true;
    }
    return added;
  }

  void TargetGenerator::AddTarget(const ExecutionTarget& target) {
    Glib::Mutex::Lock targetLock(targetMutex);
    foundTargets.push_back(target);
  }


  void TargetGenerator::AddJob(const XMLNode& job) {
    Glib::Mutex::Lock jobLock(jobMutex);
    NS ns;
    XMLNode *j = new XMLNode(ns, "");
    j->Replace(job);
    foundJobs.push_back(j);
  }

  void TargetGenerator::PrintTargetInfo(bool longlist) const {
    for (std::list<ExecutionTarget>::const_iterator cli = foundTargets.begin();
         cli != foundTargets.end(); cli++)
      cli->Print(longlist);
  }

  SimpleCounter& TargetGenerator::ServiceCounter(void) {
    return threadCounter;
  }

} // namespace Arc
