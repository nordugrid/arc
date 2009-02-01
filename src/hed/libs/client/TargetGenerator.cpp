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
#include <arc/client/TargetRetriever.h>
#include <arc/client/ClientInterface.h>
#include <arc/client/UserConfig.h>
#include <arc/client/ACCLoader.h>

namespace Arc {

  Logger TargetGenerator::logger(Logger::getRootLogger(), "TargetGenerator");

  TargetGenerator::TargetGenerator(const UserConfig& usercfg,
				   const std::list<std::string>& clusters,
				   const std::list<std::string>& indexurls)
    : loader(NULL),
      threadCounter(0) {

    if (!usercfg.ResolveAlias(clusters, indexurls, clusterselect,
			      clusterreject, indexselect, indexreject))
      return;

    if (clusterselect.empty() && indexselect.empty())
      if (!usercfg.DefaultServices(clusterselect, indexselect))
	return;

    ACCConfig acccfg;
    NS ns;
    Config cfg(ns);
    acccfg.MakeConfig(cfg);
    int targetcnt = 0;

    for (URLListMap::iterator it = clusterselect.begin();
	 it != clusterselect.end(); it++)
      for (std::list<URL>::iterator it2 = it->second.begin();
	   it2 != it->second.end(); it2++) {

	XMLNode retriever = cfg.NewChild("ArcClientComponent");
	retriever.NewAttribute("name") = "TargetRetriever" + it->first;
	retriever.NewAttribute("id") = "retriever" + tostring(targetcnt);
	usercfg.ApplySecurity(retriever); // check return value ?
	XMLNode url = retriever.NewChild("URL") = it2->str();
	url.NewAttribute("ServiceType") = "computing";
	targetcnt++;
      }

    for (URLListMap::iterator it = indexselect.begin();
	 it != indexselect.end(); it++)
      for (std::list<URL>::iterator it2 = it->second.begin();
	   it2 != it->second.end(); it2++) {

	XMLNode retriever = cfg.NewChild("ArcClientComponent");
	retriever.NewAttribute("name") = "TargetRetriever" + it->first;
	retriever.NewAttribute("id") = "retriever" + tostring(targetcnt);
	usercfg.ApplySecurity(retriever); // check return value ?
	XMLNode url = retriever.NewChild("URL") = it2->str();
	url.NewAttribute("ServiceType") = "index";
	targetcnt++;
      }

    loader = new ACCLoader(cfg);
  }

  TargetGenerator::~TargetGenerator() {}

  void TargetGenerator::GetTargets(int targetType, int detailLevel) {

    if (!loader)
      return;

    logger.msg(DEBUG, "Running resource (target) discovery");

    TargetRetriever *TR;
    for (int i = 0;
	 (TR = dynamic_cast<TargetRetriever*>(loader->getACC("retriever" +
							     tostring(i))));
	 i++)
      TR->GetTargets(*this, targetType, detailLevel);
    while (threadCounter > 0)
      threadCond.wait(threadMutex);

    logger.msg(INFO, "Found %ld targets", foundTargets.size());

    for(std::list<ExecutionTarget>::iterator iter = foundTargets.begin();
	iter != foundTargets.end(); iter++){
      logger.msg(DEBUG, "Cluster: %s", iter->DomainName);
      logger.msg(DEBUG, "Health State: %s", iter->HealthState);
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

    for (URLListMap::iterator it = clusterreject.begin();
	 it != clusterreject.end(); it++)
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

    for (URLListMap::iterator it = indexreject.begin();
	 it != indexreject.end(); it++)
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
