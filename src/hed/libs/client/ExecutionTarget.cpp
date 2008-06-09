#include <arc/ArcConfig.h>
#include <arc/Logger.h>
#include <arc/XMLNode.h>
#include <arc/client/ExecutionTarget.h>
#include <arc/loader/Loader.h>
#include <arc/client/ClientInterface.h>

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
      ConnectivityOut(true),
      Source() {}

  ExecutionTarget::~ExecutionTarget() {}

  Submitter *ExecutionTarget::GetSubmitter() const {

    ACCConfig acccfg;
    NS ns;
    Config cfg(ns);
    acccfg.MakeConfig(cfg);

    XMLNode SubmitterComp = cfg.NewChild("ArcClientComponent");
    SubmitterComp.NewAttribute("name") = "Submitter" + GridFlavour;
    SubmitterComp.NewAttribute("id") = "submitter";
    SubmitterComp.NewChild("Endpoint") = url.str();
    SubmitterComp.NewChild("Source") = Source.str();
    SubmitterComp.NewChild("MappingQueue") = MappingQueue;

    const_cast<ExecutionTarget *>(this)->loader = new Loader(&cfg);

    return dynamic_cast<Submitter *>(loader->getACC("submitter"));

  }

} // namespace Arc
