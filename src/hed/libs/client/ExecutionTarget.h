#include "GLUE2/Endpoint.h"
#include "GLUE2/Location.h"
#include "GLUE2/ApplicationEnvironment.h"
#include <arc/client/ACC.h>
#include <string>
#include <list>

#ifndef ARCLIB_EXECUTIONTARGET
#define ARCLIB_EXECUTIONTARGET

namespace Arc {

  class ExecutionTarget : public ACC {
  public:
    ExecutionTarget();
    virtual ~ExecutionTarget();

    //One million attributes, possibly interesting for brokering
    Glue2::Location_t Location;
    Glue2::Endpoint_t Endpoint;
    std::string Name;

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
    int MaxStaegOutStreams;
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

    std::list<Glue2::ApplicationEnvironment_t> ApplicationEnvironments;

  };

} // namespace Arc

#endif
