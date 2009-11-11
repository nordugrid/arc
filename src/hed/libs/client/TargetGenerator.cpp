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
    : threadCounter(0), usercfg(usercfg) {

    if (usercfg.GetSelectedServices(COMPUTING).empty() &&
        usercfg.GetSelectedServices(INDEX).empty())
      return;

    for (URLListMap::const_iterator it = usercfg.GetSelectedServices(COMPUTING).begin();
         it != usercfg.GetSelectedServices(COMPUTING).end(); it++)
      for (std::list<URL>::const_iterator it2 = it->second.begin();
           it2 != it->second.end(); it2++) {
        loader.load(it->first, usercfg, *it2, COMPUTING);
      }

    for (URLListMap::const_iterator it = usercfg.GetSelectedServices(INDEX).begin();
         it != usercfg.GetSelectedServices(INDEX).end(); it++)
      for (std::list<URL>::const_iterator it2 = it->second.begin();
           it2 != it->second.end(); it2++) {
        loader.load(it->first, usercfg, *it2, INDEX);
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
      Glib::Mutex::Lock threadLock(threadMutex);
      while (threadCounter > 0)
        threadCond.wait(threadMutex);
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
      Glib::Mutex::Lock threadLock(threadMutex);
      threadCounter++;
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
      Glib::Mutex::Lock threadLock(threadMutex);
      threadCounter++;
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

  void TargetGenerator::RetrieverDone() {
    Glib::Mutex::Lock threadLock(threadMutex);
    threadCounter--;
    if (threadCounter == 0)
      threadCond.signal();
  }

  void TargetGenerator::PrintTargetInfo(bool longlist) const {
    for (std::list<ExecutionTarget>::const_iterator cli = foundTargets.begin();
         cli != foundTargets.end(); cli++)
      cli->Print(longlist);
  }

} // namespace Arc
