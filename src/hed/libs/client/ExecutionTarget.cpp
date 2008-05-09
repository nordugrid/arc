#include "ExecutionTarget.h"

namespace Arc {

  ExecutionTarget::ExecutionTarget() :
    Latitude(0),
    Longitude(0),
    TotalJobs(-1),
    RunningJobs(-1),
    WaitingJobs(-1),
    StagingJobs(-1),
    SuspendedJobs(-1),
    PreLRMSWaitingJobs(-1),
    MaxWallTime(-1),
    MinWallTime(-1),
    DefaultWallTime(-1),
    MaxCPUTime(-1),
    MinCPUTime(-1),
    DefaultCPUTime(-1),
    MaxTotalJobs(-1),
    MaxRunningJobs(-1),
    MaxWaitingJobs(-1),
    NodeMemory(-1),
    MaxPreLRMSWaitingJobs(-1),
    MaxUserRunningJobs(-1),
    MaxSlotsPerJobs(-1),
    MaxStageInStreams(-1),
    MaxStageOutStreams(-1),
    MaxMemory(-1),
    MaxDiskSpace(-1),
    DefaultStorageService(),
    Preemption(false),
    EstimatedAverageWaitingTime(-1),
    EstimatedWorstWaitingTime(-1),
    FreeSlots(-1),
    UsedSlots(-1),
    RequestedSlots(-1){}

  ExecutionTarget::~ExecutionTarget() {}

} // namespace Arc
