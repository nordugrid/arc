#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/ArcConfig.h>
#include <arc/Logger.h>
#include <arc/XMLNode.h>
#include <arc/client/ExecutionTarget.h>
#include <arc/loader/Loader.h>
#include <arc/client/ClientInterface.h>
#include <arc/client/UserConfig.h>

namespace Arc {

  ExecutionTarget::ExecutionTarget()
    : Latitude(0),
      Longitude(0),
      TotalJobs(-1),
      RunningJobs(-1),
      WaitingJobs(-1),
      StagingJobs(-1),
      SuspendedJobs(-1),
      PreLRMSWaitingJobs(-1),
      LocalRunningJobs(-1),
      LocalWaitingJobs(-1),
      FreeSlots(-1),
      UsedSlots(-1),
      RequestedSlots(-1),
      MaxTotalJobs(-1),
      MaxRunningJobs(-1),
      MaxWaitingJobs(-1),
      NodeMemory(-1),
      MaxPreLRMSWaitingJobs(-1),
      MaxUserRunningJobs(-1),
      MaxSlotsPerJob(-1),
      MaxStageInStreams(-1),
      MaxStageOutStreams(-1),
      MaxMemory(-1),
      MaxDiskSpace(-1),
      DefaultStorageService(),
      Preemption(false),
      EstimatedAverageWaitingTime(-1),
      EstimatedWorstWaitingTime(-1),
      Reservation(false),
      BulkSubmission(false),
      TotalPhysicalCPUs(-1),
      TotalLogicalCPUs(-1),
      TotalSlots(-1),
      Homogeneity(true),
      WorkingAreaShared(true),
      WorkingAreaFree(-1),
      CacheFree(-1),
      VirtualMachine(false),
      CPUClockSpeed(-1),
      MainMemorySize(-1),
      ConnectivityIn(true),
      ConnectivityOut(true) {}

  ExecutionTarget::~ExecutionTarget() {}

  Submitter* ExecutionTarget::GetSubmitter(const UserConfig& ucfg) const {

    ACCConfig acccfg;
    NS ns;
    Config cfg(ns);
    acccfg.MakeConfig(cfg);

    XMLNode SubmitterComp = cfg.NewChild("ArcClientComponent");
    SubmitterComp.NewAttribute("name") = "Submitter" + GridFlavour;
    SubmitterComp.NewAttribute("id") = "submitter";
    if (!ucfg.ApplySecurity(SubmitterComp))
      return NULL;
    SubmitterComp.NewChild("Cluster") = Cluster.str();
    SubmitterComp.NewChild("Queue") = MappingQueue;
    SubmitterComp.NewChild("SubmissionEndpoint") = url.str();

    const_cast<ExecutionTarget*>(this)->loader = new Loader(&cfg);

    return dynamic_cast<Submitter*>(loader->getACC("submitter"));
  }

  void ExecutionTarget::Print(bool longlist) const {

    std::cout << IString("Cluster: %s", DomainName) << std::endl;
    if (!HealthState.empty())
      std::cout << IString(" Health State: %s", HealthState) << std::endl;

    if (longlist) {
      if (!Owner.empty())
	std::cout << IString(" Owner: %s", Owner) << std::endl;
      if (!PostCode.empty())
	std::cout << IString(" PostCode: %s", PostCode) << std::endl;
      if (!Place.empty())
	std::cout << IString(" Place: %s", Place) << std::endl;
      if (Latitude != 0)
	std::cout << IString(" Latitude: %f", Latitude) << std::endl;
      if (Longitude != 0)
	std::cout << IString(" Longitude: %f", Longitude) << std::endl;

      std::cout << std::endl << IString("Endpoint information") << std::endl;
      if (url)
	std::cout << IString(" URL: %s", url.str()) << std::endl;
      if (!Interface.empty()) {
	std::cout << IString(" Interface Name: %s", Interface) << std::endl;
	if (!InterfaceExtension.empty())
	  std::cout << IString(" Interface Extension: %s", InterfaceExtension)
		    << std::endl;
      }
      if (!Implementor.empty())
	std::cout << IString(" Implementor: %s", Implementor) << std::endl;
      if (!ImplementationName.empty()) {
	std::cout << IString(" Implementation Name: %s", ImplementationName)
		  << std::endl;
	if (!ImplementationVersion.empty())
	  std::cout << IString(" Implementation Version: %s",
			       ImplementationVersion) << std::endl;
      }
      if (!HealthState.empty())
	std::cout << IString(" Health State: %s", HealthState) << std::endl;
      if (!ServingState.empty())
	std::cout << IString(" Serving State: %s", ServingState) << std::endl;
      if (!IssuerCA.empty())
	std::cout << IString(" Issuer CA: %s", IssuerCA) << std::endl;
      if (!Staging.empty())
	std::cout << IString(" Staging: %s", Staging) << std::endl;

      std::cout << std::endl << IString("Queue information") << std::endl;
      if (!MappingQueue.empty())
	std::cout << IString(" Mapping Queue: %s", MappingQueue) << std::endl;
      if (MaxWallTime != -1)
	std::cout << IString(" Max Wall Time: %s", (std::string)MaxWallTime)
		  << std::endl;
      if (MinWallTime != -1)
	std::cout << IString(" Min Wall Time: %s", (std::string)MinWallTime)
		  << std::endl;
      if (DefaultWallTime != -1)
	std::cout << IString(" Default Wall Time: %s",
			     (std::string)DefaultWallTime) << std::endl;
      if (MaxCPUTime != -1)
	std::cout << IString(" Max CPU Time: %s", (std::string)MaxCPUTime)
		  << std::endl;
      if (MinCPUTime != -1)
	std::cout << IString(" Min CPU Time: %s", (std::string)MinCPUTime)
		  << std::endl;
      if (DefaultCPUTime != -1)
	std::cout << IString(" Default CPU Time: %s",
			     (std::string)DefaultCPUTime) << std::endl;
      if (MaxTotalJobs != -1)
	std::cout << IString(" Max Total Jobs: %i", MaxTotalJobs) << std::endl;
      if (MaxRunningJobs != -1)
	std::cout << IString(" Max Running Jobs: %i", MaxRunningJobs)
		  << std::endl;
      if (MaxWaitingJobs != -1)
	std::cout << IString(" Max Waiting Jobs: %i", MaxWaitingJobs)
		  << std::endl;
      if (MaxPreLRMSWaitingJobs != -1)
	std::cout << IString(" Max Pre LRMS Waiting Jobs: %i",
			     MaxPreLRMSWaitingJobs) << std::endl;
      if (MaxUserRunningJobs != -1)
	std::cout << IString(" Max User Running Jobs: %i", MaxUserRunningJobs)
		  << std::endl;
      if (MaxSlotsPerJob != -1)
	std::cout << IString(" Max Slots Per Job: %i", MaxSlotsPerJob)
		  << std::endl;
      if (MaxStageInStreams != -1)
	std::cout << IString(" Max Stage In Streams: %i", MaxStageInStreams)
		  << std::endl;
      if (MaxStageOutStreams != -1)
	std::cout << IString(" Max Stage Out Streams: %i", MaxStageOutStreams)
		  << std::endl;
      if (MaxMemory != -1)
	std::cout << IString(" Max Memory: %i", MaxMemory) << std::endl;
      if (MaxDiskSpace != -1)
	std::cout << IString(" Max Disk Space: %i", MaxDiskSpace) << std::endl;
      if (DefaultStorageService)
	std::cout << IString(" Default Storage Service: %s",
			     DefaultStorageService.str()) << std::endl;
      if (Preemption)
	std::cout << IString(" Supports Preemption") << std::endl;
      else
	std::cout << IString(" Doesn't Support Preemption") << std::endl;

    } //end if long

    std::cout << std::endl;

  } // end print

} // namespace Arc
