#ifndef __ARC_EXECUTIONTARGET_H__
#define __ARC_EXECUTIONTARGET_H__

#include <list>
#include <string>

#include <arc/DateTime.h>
#include <arc/URL.h>
#include <arc/client/ACC.h>
#include <arc/client/Submitter.h>

namespace Arc {

  class Loader;

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
    URL url;
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
    Period MaxWallTime;
    Period MinWallTime;
    Period DefaultWallTime;
    Period MaxCPUTime;
    Period MinCPUTime;
    Period DefaultCPUTime;
    int MaxTotalJobs;
    int MaxRunningJobs;
    int MaxWaitingJobs;
    int NodeMemory;
    int MaxPreLRMSWaitingJobs;
    int MaxUserRunningJobs;
    int MaxSlotsPerJob;
    int MaxStageInStreams;
    int MaxStageOutStreams;
    std::string SchedulingPolicy;
    int MaxMemory;
    int MaxDiskSpace;
    URL DefaultStorageService;
    bool Preemption;
    std::string ServingState;
    Period EstimatedAverageWaitingTime;
    Period EstimatedWorstWaitingTime;
    int FreeSlots;
    int UsedSlots;
    int RequestedSlots;
    std::string ReservationPolicy;

    std::list<std::string> RunTimeEnvironment;

    //Other
    Submitter *GetSubmitter() const;
    std::string GridFlavour;
    std::string Source;

  private:

    Loader *loader;

  };

} // namespace Arc

#endif // __ARC_EXECUTIONTARGET_H__
