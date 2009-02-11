#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/ArcConfig.h>
#include <arc/Logger.h>
#include <arc/XMLNode.h>
#include <arc/client/ACCLoader.h>
#include <arc/client/ClientInterface.h>
#include <arc/client/ExecutionTarget.h>
#include <arc/client/Submitter.h>
#include <arc/client/UserConfig.h>

namespace Arc {

  ExecutionTarget::ExecutionTarget()
    : Latitude(0),
      Longitude(0),
      DowntimeStarts(-1),
      DowntimeEnds(-1),
      MaxWallTime(-1),
      MaxTotalWallTime(-1),
      MinWallTime(-1),
      DefaultWallTime(-1),
      MaxCPUTime(-1),
      MaxTotalCPUTime(-1),
      MinCPUTime(-1),
      DefaultCPUTime(-1),
      MaxTotalJobs(-1),
      MaxRunningJobs(-1),
      MaxWaitingJobs(-1),
      MaxPreLRMSWaitingJobs(-1),
      MaxUserRunningJobs(-1),
      MaxSlotsPerJob(-1),
      MaxStageInStreams(-1),
      MaxStageOutStreams(-1),
      MaxMainMemory(-1),
      MaxVirtualMemory(-1),
      MaxDiskSpace(-1),
      Preemption(false),
      TotalJobs(-1),
      RunningJobs(-1),
      LocalRunningJobs(-1),
      WaitingJobs(-1),
      LocalWaitingJobs(-1),
      SuspendedJobs(-1),
      LocalSuspendedJobs(-1),
      StagingJobs(-1),
      PreLRMSWaitingJobs(-1),
      EstimatedAverageWaitingTime(-1),
      EstimatedWorstWaitingTime(-1),
      FreeSlots(-1),
      UsedSlots(-1),
      RequestedSlots(-1),
      Reservation(false),
      BulkSubmission(false),
      TotalPhysicalCPUs(-1),
      TotalLogicalCPUs(-1),
      TotalSlots(-1),
      Homogeneous(true),
      WorkingAreaShared(true),
      WorkingAreaTotal(-1),
      WorkingAreaFree(-1),
      CacheTotal(-1),
      CacheFree(-1),
      VirtualMachine(false),
      CPUClockSpeed(-1),
      MainMemorySize(-1),
      ConnectivityIn(false),
      ConnectivityOut(false) {}

  ExecutionTarget::ExecutionTarget(const long int addrptr) {
    *this = *((ExecutionTarget*)addrptr);
  }

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

    const_cast<ExecutionTarget*>(this)->loader = new ACCLoader(cfg);

    return dynamic_cast<Submitter*>(loader->getACC("submitter"));
  }

  void ExecutionTarget::Print(bool longlist) const {

    std::cout << IString("Cluster: %s", DomainName) << std::endl;
    if (!HealthState.empty())
      std::cout << IString(" Health State: %s", HealthState) << std::endl;

    if (longlist) {

      std::cout << std::endl << IString("Location information:") << std::endl;

      if (!Address.empty())
	std::cout << IString(" Address: %s", Address) << std::endl;
      if (!Place.empty())
	std::cout << IString(" Place: %s", Place) << std::endl;
      if (!Country.empty())
	std::cout << IString(" Country: %s", Country) << std::endl;
      if (!PostCode.empty())
	std::cout << IString(" Postal Code: %s", PostCode) << std::endl;
      if (Latitude != 0)
	std::cout << IString(" Latitude: %f", Latitude) << std::endl;
      if (Longitude != 0)
	std::cout << IString(" Longitude: %f", Longitude) << std::endl;

      std::cout << std::endl << IString("Domain information:") << std::endl;

      if (!Owner.empty())
	std::cout << IString(" Owner: %s", Owner) << std::endl;

      std::cout << std::endl << IString("Service information:") << std::endl;

      if (!ServiceName.empty())
	std::cout << IString(" Service Name: %s", ServiceName) << std::endl;
      if (!ServiceName.empty())
	std::cout << IString(" Service Type: %s", ServiceType) << std::endl;

      std::cout << std::endl << IString("Endpoint information") << std::endl;

      if (url)
	std::cout << IString(" URL: %s", url.str()) << std::endl;
      if (!Capability.empty()) {
	std::cout << IString(" Capabilities:") << std::endl;
	for (std::list<std::string>::const_iterator it = Capability.begin();
	     it != Capability.end(); it++)
	  std::cout << "  " << *it << std::endl;
      }
      if (!Technology.empty())
	std::cout << IString(" Technology: %s", Technology) << std::endl;
      if (!InterfaceName.empty())
	std::cout << IString(" Interface Name: %s", InterfaceName)
		  << std::endl;
      if (!InterfaceVersion.empty()) {
	std::cout << IString(" Interface Versions:") << std::endl;
	for (std::list<std::string>::const_iterator it =
	       InterfaceVersion.begin(); it != InterfaceVersion.end(); it++)
	  std::cout << "  " << *it << std::endl;
      }
      if (!InterfaceExtension.empty()) {
	std::cout << IString(" Interface Extensions:") << std::endl;
	for (std::list<std::string>::const_iterator it =
	       InterfaceExtension.begin();
	     it != InterfaceExtension.end(); it++)
	  std::cout << "  " << *it << std::endl;
      }
      if (!SupportedProfile.empty()) {
	std::cout << IString(" Supported Profiles:") << std::endl;
	for (std::list<std::string>::const_iterator it =
	       SupportedProfile.begin(); it != SupportedProfile.end(); it++)
	  std::cout << "  " << *it << std::endl;
      }
      if (!Implementor.empty())
	std::cout << IString(" Implementor: %s", Implementor) << std::endl;
      if (!ImplementationName.empty())
	std::cout << IString(" Implementation Name: %s", ImplementationName)
		  << std::endl;
      if (!ImplementationVersion.empty())
	std::cout << IString(" Implementation Version: %s",
			     ImplementationVersion) << std::endl;
      if (!QualityLevel.empty())
	std::cout << IString(" QualityLevel: %s", QualityLevel) << std::endl;
      if (!HealthState.empty())
	std::cout << IString(" Health State: %s", HealthState) << std::endl;
      if (!HealthStateInfo.empty())
	std::cout << IString(" Health State Info: %s", HealthStateInfo)
		  << std::endl;
      if (!ServingState.empty())
	std::cout << IString(" Serving State: %s", ServingState) << std::endl;
      if (!IssuerCA.empty())
	std::cout << IString(" Issuer CA: %s", IssuerCA) << std::endl;
      if (!TrustedCA.empty()) {
	std::cout << IString(" Trusted CAs:") << std::endl;
	for (std::list<std::string>::const_iterator it = TrustedCA.begin();
	     it != TrustedCA.end(); it++)
	  std::cout << "  " << *it << std::endl;
      }
      if (DowntimeStarts != -1)
	std::cout << IString(" Downtime Starts: %s", DowntimeStarts.str())
		  << std::endl;
      if (DowntimeEnds != -1)
	std::cout << IString(" Downtime Ends: %s", DowntimeEnds.str())
		  << std::endl;
      if (!Staging.empty())
	std::cout << IString(" Staging: %s", Staging) << std::endl;
      if (!JobDescriptions.empty()) {
	std::cout << IString(" Job Descriptions:") << std::endl;
	for (std::list<std::string>::const_iterator it =
	       JobDescriptions.begin(); it != JobDescriptions.end(); it++)
	  std::cout << "  " << *it << std::endl;
      }

      std::cout << std::endl << IString("Queue information") << std::endl;

      if (!MappingQueue.empty())
	std::cout << IString(" Mapping Queue: %s", MappingQueue) << std::endl;
      if (MaxWallTime != -1)
	std::cout << IString(" Max Wall Time: %s", (std::string)MaxWallTime)
		  << std::endl;
      if (MaxTotalWallTime != -1)
	std::cout << IString(" Max Total Wall Time: %s",
			     (std::string)MaxTotalWallTime) << std::endl;
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
      if (!SchedulingPolicy.empty())
	std::cout << IString(" Scheduling Policy: %s", SchedulingPolicy)
		  << std::endl;
      if (MaxMainMemory != -1)
	std::cout << IString(" Max Memory: %i", MaxMainMemory) << std::endl;
      if (MaxVirtualMemory != -1)
	std::cout << IString(" Max Virtual Memory: %i", MaxVirtualMemory)
		  << std::endl;
      if (MaxDiskSpace != -1)
	std::cout << IString(" Max Disk Space: %i", MaxDiskSpace) << std::endl;
      if (DefaultStorageService)
	std::cout << IString(" Default Storage Service: %s",
			     DefaultStorageService.str()) << std::endl;
      if (Preemption)
	std::cout << IString(" Supports Preemption") << std::endl;
      else
	std::cout << IString(" Doesn't Support Preemption") << std::endl;
      if (TotalJobs != -1)
	std::cout << IString(" Total Jobs: %i", TotalJobs) << std::endl;
      if (RunningJobs != -1)
	std::cout << IString(" Running Jobs: %i", RunningJobs) << std::endl;
      if (LocalRunningJobs != -1)
	std::cout << IString(" Local Running Jobs: %i", LocalRunningJobs)
		  << std::endl;
      if (WaitingJobs != -1)
	std::cout << IString(" Waiting Jobs: %i", WaitingJobs) << std::endl;
      if (LocalWaitingJobs != -1)
	std::cout << IString(" Local Waiting Jobs: %i", LocalWaitingJobs)
		  << std::endl;
      if (SuspendedJobs != -1)
	std::cout << IString(" Suspended Jobs: %i", SuspendedJobs)
		  << std::endl;
      if (LocalSuspendedJobs != -1)
	std::cout << IString(" Local Suspended Jobs: %i", LocalSuspendedJobs)
		  << std::endl;
      if (StagingJobs != -1)
	std::cout << IString(" Staging Jobs: %i", StagingJobs) << std::endl;
      if (PreLRMSWaitingJobs != -1)
	std::cout << IString(" Pre-LRMS Waiting Jobs: %i", PreLRMSWaitingJobs)
		  << std::endl;
      if (EstimatedAverageWaitingTime != -1)
	std::cout << IString(" Estimated Average Waiting Time: %s",
			     (std::string)EstimatedAverageWaitingTime)
		  << std::endl;
      if (EstimatedWorstWaitingTime != -1)
	std::cout << IString(" Estimated Worst Waiting Time: %s",
			     (std::string)EstimatedWorstWaitingTime)
		  << std::endl;
      if (FreeSlots != -1)
	std::cout << IString(" Free Slots: %i", FreeSlots) << std::endl;
      if (!FreeSlotsWithDuration.empty())
	std::cout << IString(" Free Slots With Duration: %s",
			     FreeSlotsWithDuration) << std::endl;
      if (UsedSlots != -1)
	std::cout << IString(" Used Slots: %i", UsedSlots) << std::endl;
      if (RequestedSlots != -1)
	std::cout << IString(" Requested Slots: %i", RequestedSlots)
		  << std::endl;
      if (!ReservationPolicy.empty())
	std::cout << IString(" Reservation Policy: %s", ReservationPolicy)
		  << std::endl;

      std::cout << std::endl << IString("Manager information") << std::endl;

      if (!ManagerProductName.empty())
	std::cout << IString(" Resource Manager: %s", ManagerProductName)
		  << std::endl;
      if (!ManagerProductVersion.empty())
	std::cout << IString(" Resource Manager Version: %s",
			     ManagerProductVersion) << std::endl;
      if (Reservation)
	std::cout << IString(" Supports Advance Reservations") << std::endl;
      else
	std::cout << IString(" Doesn't Support Advance Reservations")
		  << std::endl;
      if (BulkSubmission)
	std::cout << IString(" Supports Bulk Submission") << std::endl;
      else
	std::cout << IString(" Doesn't Support Bulk Submission") << std::endl;
      if (TotalPhysicalCPUs != -1)
	std::cout << IString(" Total Physical CPUs: %i", TotalPhysicalCPUs)
		  << std::endl;
      if (TotalLogicalCPUs != -1)
	std::cout << IString(" Total Logical CPUs: %i", TotalLogicalCPUs)
		  << std::endl;
      if (TotalSlots != -1)
	std::cout << IString(" Total Slots: %i", TotalSlots) << std::endl;
      if (Homogeneous)
	std::cout << IString(" Homogeneous Resource") << std::endl;
      else
	std::cout << IString(" Non-homogeneous Resource") << std::endl;
      if (!NetworkInfo.empty()) {
	std::cout << IString(" Network Information:") << std::endl;
	for (std::list<std::string>::const_iterator it = NetworkInfo.begin();
	     it != NetworkInfo.end(); it++)
	  std::cout << "  " << *it << std::endl;
      }
      if (WorkingAreaShared)
	std::cout << IString(" Working area is shared among jobs")
		  << std::endl;
      else
	std::cout << IString(" Working area is nor shared among jobs")
		  << std::endl;
      if (WorkingAreaTotal != -1)
	std::cout << IString(" Working Area Total Size: %i", WorkingAreaTotal)
		  << std::endl;
      if (WorkingAreaFree != -1)
	std::cout << IString(" Working Area Free Size: %i", WorkingAreaFree)
		  << std::endl;
      if (WorkingAreaLifeTime != -1)
	std::cout << IString(" Working Area Life Time: %s",
			     (std::string)WorkingAreaLifeTime) << std::endl;
      if (CacheTotal != -1)
	std::cout << IString(" Cache Area Total Size: %i", CacheTotal)
		  << std::endl;
      if (CacheFree != -1)
	std::cout << IString(" Cache Area Free Size: %i", CacheFree)
		  << std::endl;

      // Benchmarks

      if (!Benchmarks.empty()) {
	std::cout << IString(" Benchmark Information:") << std::endl;
	for (std::map<std::string, double>::const_iterator it =
	       Benchmarks.begin(); it != Benchmarks.end(); it++)
	  std::cout << "  " << it->first << ": " << it->second << std::endl;
      }

      std::cout << std::endl << IString("Execution Environment information")
		<< std::endl;

      if (!Platform.empty())
	std::cout << IString(" Platform: %s", Platform) << std::endl;
      if (VirtualMachine)
	std::cout << IString(" Execution environment is a virtual machine")
		  << std::endl;
      else
	std::cout << IString(" Execution environment is a physical machine")
		  << std::endl;
      if (!CPUVendor.empty())
	std::cout << IString(" CPU Vendor: %s", CPUVendor) << std::endl;
      if (!CPUModel.empty())
	std::cout << IString(" CPU Model: %s", CPUModel) << std::endl;
      if (!CPUVersion.empty())
	std::cout << IString(" CPU Version: %s", CPUVersion) << std::endl;
      if (CPUClockSpeed != -1)
	std::cout << IString(" CPU Clock Speed: %i", CPUClockSpeed)
		  << std::endl;
      if (MainMemorySize != -1)
	std::cout << IString(" Main Memory SIze: %i", MainMemorySize)
		  << std::endl;
      if (!OSFamily.empty())
	std::cout << IString(" OS Family: %s", OSFamily) << std::endl;
      if (!OSName.empty())
	std::cout << IString(" OS Name: %s", OSName) << std::endl;
      if (!OSVersion.empty())
	std::cout << IString(" OS Version: %s", OSVersion) << std::endl;
      if (ConnectivityIn)
	std::cout << IString(" Execution environment"
			     " supports inbound connections") << std::endl;
      else
	std::cout << IString(" Execution environment does not"
			     " support inbound connections") << std::endl;
      if (ConnectivityOut)
	std::cout << IString(" Execution environment"
			     " supports outbound connections") << std::endl;
      else
	std::cout << IString(" Execution environment does not"
			     " support outbound connections") << std::endl;

      // TODO: Information about RTEs

    } // end if long

    std::cout << std::endl;

  } // end print

} // namespace Arc
