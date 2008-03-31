/**
 * GLUE2 ComputingShare class
 */
#ifndef ARCLIB_COMPUTINGSHARE
#define ARCLIB_COMPUTINGSHARE

#include "Share.h"
#include "enums.h"
#include <arc/URL.h>
#include <string>
#include <stdint.h>

namespace Arc{

  class ComputingShare : public Arc::Share{
  public:
    ComputingShare();
    ~ComputingShare();

    std::string MappingQueue;
    int64_t MaxWallTime;
    int64_t MinWallTime;
    int64_t DefaultWallTime;
    int64_t MaxCPUTime;
    int64_t MaxCPUsTime; //improved name suggested
    int64_t MinCPUTime;
    int64_t DefaultCPUTime;
    int MaxTotalJobs;
    int MaxRunningJobs;
    int MaxWaitingJobs;
    int MaxPreLRMSWaitingJobs;
    int MaxUserRunningJobs;
    int MaxSlotsPerJob;
    int MaxStageInStreams;
    int MaxStageOutStreams;
    SchedulingPolicy_t SchedulingPolicy;
    int64_t MaxMemory;
    int64_t MaxDiskSpace;
    Arc::URL DefaultStorageService;
    bool Preemption;
    ServingState_t ServingState;
    int TotalJobs;
    int RunningJobs;
    int LocalRunningJobs;
    int WaitingJobs;
    int LocalWaitingJobs;
    int StagingJobs;
    int SuspendedJobs;
    int PreLRMSWaitingJobs;
    int64_t EstimatedAverageWaitingTime;
    int64_t EstimatedWorstWaitingTime;
    int64_t FreeSlots;
    int64_t UsedSlots;
    int64_t RequestedSlots;

  }; //end class

} //end namespace

#endif
