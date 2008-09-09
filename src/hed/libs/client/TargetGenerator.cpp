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

namespace Arc {
  
  Logger TargetGenerator::logger(Logger::getRootLogger(), "TargetGenerator");
  
  TargetGenerator::TargetGenerator(const UserConfig& ucfg,
				   const std::list<std::string>& clusterselect,
				   const std::list<std::string>& /* clusterreject */,
				   const std::list<std::string>& indexurls) 
    : threadCounter(0){

    ACCConfig acccfg;
    NS ns;
    Config mcfg(ns);
    acccfg.MakeConfig(mcfg);

    bool ClustersSpecified = false;
    bool IndexServersSpecified = false;
    int TargetURL = 0;

    // first add to config element the specified target clusters (if any)
    for (std::list<std::string>::const_iterator it = clusterselect.begin();
	 it != clusterselect.end(); it++) {

      std::string::size_type colon = it->find_first_of(":");
      std::string GridFlavour = it->substr(0, colon);
      std::string SomeURL = it->substr(colon + 1);

      XMLNode ThisRetriever = mcfg.NewChild("ArcClientComponent");
      ThisRetriever.NewAttribute("name") = "TargetRetriever" + GridFlavour;
      ThisRetriever.NewAttribute("id") = "retriever" + tostring(TargetURL);
      ucfg.ApplySecurity(ThisRetriever); // check return value ?
      XMLNode ThisURL = ThisRetriever.NewChild("URL") = SomeURL;
      ThisURL.NewAttribute("ServiceType") = "computing";

      TargetURL++;
      ClustersSpecified = true;
    }

    // next steps are index servers (giis'es in ARC0)
    for (std::list<std::string>::const_iterator it = indexurls.begin();
	 it != indexurls.end(); it++) {

      std::string::size_type colon = it->find_first_of(":");
      std::string GridFlavour = it->substr(0, colon);
      std::string SomeURL = it->substr(colon + 1);

      XMLNode ThisRetriever = mcfg.NewChild("ArcClientComponent");
      ThisRetriever.NewAttribute("name") = "TargetRetriever" + GridFlavour;
      ThisRetriever.NewAttribute("id") = "retriever" + tostring(TargetURL);
      ucfg.ApplySecurity(ThisRetriever); // check return value ?
      XMLNode ThisURL = ThisRetriever.NewChild("URL") = SomeURL;
      ThisURL.NewAttribute("ServiceType") = "index";

      TargetURL++;
      IndexServersSpecified = true;
    }

    if (TargetURL == 0) {
      std::cout << "No Target URL specified (or no alias match), "
	"no targets will be found" << std::endl;
    }

    //finally, initialize loader
    loader = new Loader(&mcfg);
  }

  TargetGenerator::~TargetGenerator() {}

  void TargetGenerator::GetTargets(int targetType, int detailLevel) {
    TargetRetriever *TR;
    for (int i = 0;
	 (TR = dynamic_cast<TargetRetriever*>(loader->getACC("retriever" +
							     tostring(i))));
	 i++)
      TR->GetTargets(*this, targetType, detailLevel);
    while (threadCounter > 0)
      threadCond.wait(threadMutex);
    logger.msg(INFO, "Number of Targets found: %ld", foundTargets.size());
  }

  const std::list<ExecutionTarget>& TargetGenerator::FoundTargets() const {
    return foundTargets;
  }

  bool TargetGenerator::AddService(const URL& url) {
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
