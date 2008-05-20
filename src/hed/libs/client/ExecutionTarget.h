#include <arc/client/Submitter.h>
#include <arc/client/ACC.h>
#include <arc/URL.h>
#include <string>
#include <list>

#ifndef ARCLIB_EXECUTIONTARGET
#define ARCLIB_EXECUTIONTARGET

namespace Arc {

  class ExecutionTarget {
  public:
    ExecutionTarget();
    virtual ~ExecutionTarget();

    //One million attributes, possibly interesting for brokering

    //Below attributes inspired by Glue2:Location
    std::string Name;
    std::string Alias;
    std::string Owner;
    std::string PostCode;
    std::string Place;
    float Latitude;
    float Longitude;

    //Below attributes inspired by Glue2:Endpoint
    Arc::URL URL;
    std::string InterfaceName;
    std::string InterfaceVersion;
    std::string Implementor;
    std::string ImplementationName;
    std::string ImplementationVersion;
    std::string HealthState;
    std::string IssuerCA;
    std::string Staging;

    //The rest
    int TotalJobs;
    int RunningJobs;
    int WaitingJobs;
    int StagingJobs;
    int SuspendedJobs;
    int PreLRMSWaitingJobs;

    std::string MappingQueue;
    int MaxWallTime;
    int MinWallTime;
    int DefaultWallTime;
    int MaxCPUTime;
    int MinCPUTime;
    int DefaultCPUTime;
    int MaxTotalJobs;
    int MaxRunningJobs;
    int MaxWaitingJobs;
    int NodeMemory;
    int MaxPreLRMSWaitingJobs;
    int MaxUserRunningJobs;
    int MaxSlotsPerJobs;
    int MaxStageInStreams;
    int MaxStageOutStreams;
    std::string SchedulingPolicy;
    int MaxMemory;
    int MaxDiskSpace;
    Arc::URL DefaultStorageService;
    bool Preemption;
    std::string ServingState;
    int EstimatedAverageWaitingTime;
    int EstimatedWorstWaitingTime;
    int FreeSlots;
    int UsedSlots;
    int RequestedSlots;
    std::string ReservationPolicy;

    //Other
    Arc::Submitter *GetSubmitter();
    std::string GridFlavour;
    std::string Source;

  private:

    Arc::Loader *loader;

  };

} // namespace Arc

#endif
