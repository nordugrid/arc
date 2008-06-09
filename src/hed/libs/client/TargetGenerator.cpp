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

namespace Arc {
  
  Logger TargetGenerator::logger(Logger::getRootLogger(), "TargetGenerator");
  
  TargetGenerator::TargetGenerator(const std::list<std::string>& clusterselect,
				   const std::list<std::string>& /* clusterreject */,
				   const std::list<std::string>& giisurls) 
    : threadCounter(0){
    
    Arc::ACCConfig acccfg;
    Arc::NS ns;
    Arc::Config mcfg(ns);
    acccfg.MakeConfig(mcfg);
    
    bool ClustersSpecified = false;
    bool IndexServersSpecified = false;
    int TargetURL = 1;

    //Read acc config file (final version should read from install area)
    Arc::XMLNode ArclibConfig;
    bool ConfigExists = ArclibConfig.ReadFromFile("TestConfig.xml");
    
    //first add to config element the specified target clusters (if any)
    for (std::list<std::string>::const_iterator it = clusterselect.begin(); it != clusterselect.end(); it++){
      
      std::list<std::string> aliasTO;
      if(ConfigExists){
	aliasTO = ResolveAlias(*it, ArclibConfig);
      } else{
	aliasTO.push_back(*it);
      }

      for (std::list<std::string>::const_iterator iter = aliasTO.begin(); iter != aliasTO.end(); iter++){
	
	size_t colon = (*iter).find_first_of(":");
	std::string GridFlavour = (*iter).substr(0, colon);
	std::string SomeURL = (*iter).substr(colon+1);
	
	Arc::XMLNode ThisRetriever = mcfg.NewChild("ArcClientComponent");
	ThisRetriever.NewAttribute("name") = "TargetRetriever"+ (std::string) GridFlavour;
	ThisRetriever.NewAttribute("id") = "retriever" + Arc::tostring(TargetURL);
	Arc::XMLNode ThisRetriever1 = ThisRetriever.NewChild("URL") = (std::string) SomeURL;
	ThisRetriever1.NewAttribute("ServiceType") = "computing";
	
	TargetURL++;
	ClustersSpecified = true;
      }
    }
    
    //if no cluster url are given next steps are index servers (giis'es in ARC0)
    if (!ClustersSpecified){ //means that -c option takes priority over -g
      for (std::list<std::string>::const_iterator it = giisurls.begin(); it != giisurls.end(); it++){      
	
	size_t colon = (*it).find_first_of(":");
	std::string GridFlavour = (*it).substr(0, colon);
	std::string SomeURL = (*it).substr(colon+1);
	
	Arc::XMLNode ThisRetriever = mcfg.NewChild("ArcClientComponent");
	ThisRetriever.NewAttribute("name") = "TargetRetriever"+ (std::string) GridFlavour;
	ThisRetriever.NewAttribute("id") = "retriever" + Arc::tostring(TargetURL);
	Arc::XMLNode ThisRetriever1 = ThisRetriever.NewChild("URL") = (std::string) SomeURL;
	ThisRetriever1.NewAttribute("ServiceType") = "index";
	
	TargetURL++;
	IndexServersSpecified = true;
      }
    }
    
    //if neither clusters nor index servers are specified, read from config file
    if(!ClustersSpecified && !IndexServersSpecified){

      XMLNodeList defaults = ArclibConfig.XPathLookup("//Defaults//URL", Arc::NS());
      
      for (XMLNodeList::iterator iter = defaults.begin(); iter != defaults.end(); iter++){
	std::string ThisServer = (std::string) (*iter);
	
	size_t colon = ThisServer.find_first_of(":");
	std::string GridFlavour = ThisServer.substr(0, colon);
	std::string SomeURL = ThisServer.substr(colon+1);
	
	Arc::XMLNode ThisRetriever = mcfg.NewChild("ArcClientComponent");
	ThisRetriever.NewAttribute("name") = "TargetRetriever"+ (std::string) GridFlavour;
	ThisRetriever.NewAttribute("id") = "retriever" + Arc::tostring(TargetURL);
	Arc::XMLNode ThisRetriever1 = ThisRetriever.NewChild("URL") = (std::string) SomeURL;
	ThisRetriever1.NewAttribute("ServiceType") = "index";
	
	TargetURL++;

      }
    }

    if(TargetURL == 1){
      std::cout<<"No Target URL specified (no alias match), no targets will be found"<<std::endl;
    }

    //finally, initialize loader
    loader = new Loader(&mcfg);
  }
  
  TargetGenerator::~TargetGenerator() {}

  void TargetGenerator::GetTargets(int targetType, int detailLevel) {
    TargetRetriever *TR;
    for (int i = 1;
	 (TR = dynamic_cast<TargetRetriever *>(loader->getACC("retriever" +
							      tostring(i))));
	 i++)
      TR->GetTargets(*this, targetType, detailLevel);
    while (threadCounter > 0)
      threadCond.wait(threadMutex);
    logger.msg(INFO, "Number of Targets found: %ld", foundTargets.size());
  }

  const std::list<ExecutionTarget> TargetGenerator::FoundTargets() const {
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
	 cli != foundTargets.end(); cli++) {

      std::cout << IString("Cluster: %s", cli->DomainName) << std::endl;
      if (!cli->HealthState.empty())
	std::cout << IString(" Health State: %s", cli->HealthState) << std::endl;

      if (longlist) {
	if (!cli->Owner.empty())
	  std::cout << IString(" Owner: %s", cli->Owner) << std::endl;
	if (!cli->PostCode.empty())
	  std::cout << IString(" PostCode: %s", cli->PostCode) << std::endl;
	if (!cli->Place.empty())
	  std::cout << IString(" Place: %s", cli->Place) << std::endl;
	if (cli->Latitude != 0)
	  std::cout << IString(" Latitude: %f", cli->Latitude) << std::endl;
	if (cli->Longitude != 0)
	  std::cout << IString(" Longitude: %f", cli->Longitude) << std::endl;

	/*
	std::cout << IString("Endpoint information") << std::endl;
	if (cli->url)
	  std::cout << IString(" URL: %s", cli->url.str()) << std::endl;
	if (!cli->InterfaceName.empty())
	  std::cout << IString(" Interface Name: %s", cli->InterfaceName) << std::endl;
	if (!cli->InterfaceVersion.empty())
	  std::cout << IString(" Interface Version: %s", cli->InterfaceVersion) << std::endl;
	if (!cli->Implementor.empty())
	  std::cout << IString(" Implementor: %s", cli->Implementor) << std::endl;
	if (!cli->ImplementationName.empty())
	  std::cout << IString(" Implementation Name: %s", cli->ImplementationName) << std::endl;
	if (!cli->ImplementationVersion.empty())
	  std::cout << IString(" Implementation Version: %s", cli->ImplementationVersion) << std::endl;
	if (!cli->HealthState.empty())
	  std::cout << IString(" Health State: %s", cli->HealthState) << std::endl;
	if (!cli->IssuerCA.empty())
	  std::cout << IString(" Issuer CA: %s", cli->IssuerCA) << std::endl;
	if (!cli->Staging.empty())
	  std::cout << IString(" Staging: %s", cli->Staging) << std::endl;

	std::cout << IString("Queue information") << std::endl;
	if (!cli->MappingQueue.empty())
	  std::cout << IString(" Mapping Queue: %s", cli->MappingQueue) << std::endl;
	if (cli->MaxWallTime != -1)
	  std::cout << IString(" Max Wall Time: %s", (std::string)cli->MaxWallTime) << std::endl;
	if (cli->MinWallTime != -1)
	  std::cout << IString(" Min Wall Time: %s", (std::string)cli->MinWallTime) << std::endl;
	if (cli->DefaultWallTime != -1)
	  std::cout << IString(" Default Wall Time: %s", (std::string)cli->DefaultWallTime) << std::endl;
	if (cli->MaxCPUTime != -1)
	  std::cout << IString(" Max CPU Time: %s", (std::string)cli->MaxCPUTime) << std::endl;
	if (cli->MinCPUTime != -1)
	  std::cout << IString(" Min CPU Time: %s", (std::string)cli->MinCPUTime) << std::endl;
	if (cli->DefaultCPUTime != -1)
	  std::cout << IString(" Default CPU Time: %s", (std::string)cli->DefaultCPUTime) << std::endl;
	if (cli->MaxTotalJobs != -1)
	  std::cout << IString(" Max Total Jobs: %i", cli->MaxTotalJobs) << std::endl;
	if (cli->MaxRunningJobs != -1)
	  std::cout << IString(" Max Running Jobs: %i", cli->MaxRunningJobs) << std::endl;
	if (cli->MaxWaitingJobs != -1)
	  std::cout << IString(" Max Waiting Jobs: %i", cli->MaxWaitingJobs) << std::endl;
	if (cli->MaxPreLRMSWaitingJobs != -1)
	  std::cout << IString(" Max Pre LRMS Waiting Jobs: %i", cli->MaxPreLRMSWaitingJobs) << std::endl;
	if (cli->MaxUserRunningJobs != -1)
	  std::cout << IString(" Max User Running Jobs: %i", cli->MaxUserRunningJobs) << std::endl;
	if (cli->MaxSlotsPerJob != -1)
	  std::cout << IString(" Max Slots Per Job: %i", cli->MaxSlotsPerJob) << std::endl;
	if (cli->MaxStageInStreams != -1)
	  std::cout << IString(" Max Stage In Streams: %i", cli->MaxStageInStreams) << std::endl;
	if (cli->MaxStageOutStreams != -1)
	  std::cout << IString(" Max Stage Out Streams: %i", cli->MaxStageOutStreams) << std::endl;
	if (cli->MaxStageOutStreams != -1)
	  std::cout << IString(" Max Stage Out Streams: %i", cli->MaxStageOutStreams) << std::endl;
	*/

      } //end if long
      std::cout << std::endl;
    }
  }

  std::list<std::string> TargetGenerator::ResolveAlias(std::string lookup, XMLNode cfg){
    
    std::list<std::string> ToBeReturned;
    
    //find alias in alias listing
    XMLNodeList ResultList = cfg.XPathLookup("//Alias[@name='"+lookup+"']", Arc::NS());
    if(ResultList.empty()){
      ToBeReturned.push_back(lookup);
    } else{

      XMLNode Result = *ResultList.begin();
      
      //read out urls from found alias
      XMLNodeList URLs = Result.XPathLookup("//URL", Arc::NS());
      
      for(XMLNodeList::iterator iter = URLs.begin(); iter != URLs.end(); iter++){
	ToBeReturned.push_back((std::string) (*iter));
      }
      //finally check for other aliases with existing alias
      
      for(XMLNode node = Result["Alias"]; node; ++node){
	std::list<std::string> MoreURLs = ResolveAlias(node, cfg);
	ToBeReturned.insert(ToBeReturned.end(), MoreURLs.begin(), MoreURLs.end());
      }
    }

    return ToBeReturned;

  }

} // namespace Arc
