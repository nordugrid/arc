#include "TargetGenerator.h"
#include "TargetRetriever.h"
#include <arc/loader/Loader.h>
#include <arc/misc/ClientInterface.h>
#include <arc/XMLNode.h>
#include <arc/ArcConfig.h>
#include <arc/IString.h>

#include <iostream>
#include <algorithm>

namespace Arc {

  TargetGenerator::TargetGenerator(Config& cfg) {

    ACCloader = new Loader(&cfg);

  }

  TargetGenerator::~TargetGenerator() {

    if (ACCloader)
      delete ACCloader;
  }


  void TargetGenerator::GetTargets(int TargetType, int DetailLevel) {

    //Get retrievers
    bool AreThereRetrievers = true;
    char RetrieverID[20];
    int RetrieverNumber = 1;
    while (AreThereRetrievers) {
      sprintf(RetrieverID, "retriever%d", RetrieverNumber);
      TargetRetriever *TR =
	dynamic_cast<TargetRetriever *>(ACCloader->getACC(RetrieverID));
      if (TR) {
	//Get those targets ...
	//TargetType: Execution = 0, Storage = 1, ...
	//DetailLevel: Minimum = 0, Processed = 1, Full = 2
	//All options not yet implemented
	TR->GetTargets(*this, TargetType, DetailLevel);
	RetrieverNumber++;
      }
      else
	AreThereRetrievers = false;
    } //end loop over existing retrievers

    std::cout << "Number of Targets found: " << FoundTargets.size() << std::endl;

  }

  bool TargetGenerator::AddService(std::string NewService) {
    bool added = false;
    //lock this function call
    Glib::Mutex::Lock ServiceWriteLock(ServiceMutex);
    if (std::find(FoundServices.begin(), FoundServices.end(), NewService) ==
	FoundServices.end()) {
      FoundServices.push_back(NewService);
      added = true;
    }
    return added;
  }

  void TargetGenerator::AddTarget(ExecutionTarget NewTarget) {
    //lock this function call
    Glib::Mutex::Lock TargetWriteLock(TargetMutex);
    FoundTargets.push_back(NewTarget);
  }

  bool TargetGenerator::DoIAlreadyExist(std::string NewServer) {
    //lock this function call
    Glib::Mutex::Lock ServerAddLock(ServerMutex);
    bool existence = true;

    //if not already found add to list of giises
    if (std::find(CheckedInfoServers.begin(), CheckedInfoServers.end(),
		  NewServer) == CheckedInfoServers.end()) {
      CheckedInfoServers.push_back(NewServer);
      existence = false;
    }
    return existence;
  }

  void TargetGenerator::PrintTargetInfo(bool longlist) {
    for (std::list<ExecutionTarget>::iterator cli = FoundTargets.begin();
	 cli != FoundTargets.end(); cli++) {

      std::cout << IString("Cluster: %s", cli->Name) << std::endl;
      if (!cli->Alias.empty())
	std::cout << IString(" Alias: %s", cli->Alias) << std::endl;

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
	  std::cout << IString(" Max Wall Time: %i", cli->MaxWallTime) << std::endl;
	if (cli->MinWallTime != -1)
	  std::cout << IString(" Min Wall Time: %i", cli->MinWallTime) << std::endl;
	if (cli->DefaultWallTime != -1)
	  std::cout << IString(" Default Wall Time: %i", cli->DefaultWallTime) << std::endl;
	if (cli->MaxCPUTime != -1)
	  std::cout << IString(" Max CPU Time: %i", cli->MaxCPUTime) << std::endl;
	if (cli->MinCPUTime != -1)
	  std::cout << IString(" Min CPU Time: %i", cli->MinCPUTime) << std::endl;
	if (cli->DefaultCPUTime != -1)
	  std::cout << IString(" Default CPU Time: %i", cli->DefaultCPUTime) << std::endl;
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
	if (cli->MaxSlotsPerJobs != -1)
	  std::cout << IString(" Max Slots Per Job: %i", cli->MaxSlotsPerJobs) << std::endl;
	if (cli->MaxStageInStreams != -1)
	  std::cout << IString(" Max Stage In Streams: %i", cli->MaxStageInStreams) << std::endl;
	if (cli->MaxStageOutStreams != -1)
	  std::cout << IString(" Max Stage Out Streams: %i", cli->MaxStageOutStreams) << std::endl;
	if (cli->MaxStageOutStreams != -1)
	  std::cout << IString(" Max Stage Out Streams: %i", cli->MaxStageOutStreams) << std::endl;

      } //end if long
      std::cout << std::endl;
    }
  }

} // namespace Arc
