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

  struct Benchmark{
    std::string Type;
    double Value;
  };

  struct ApplicationEnvironment{
    std::string Name;
    std::string Version;
    std::string State;
    int FreeSlots;
    int FreeJobs;
    int FreeUserSeats;
  };

  class ExecutionTarget {
  public:
    ExecutionTarget();
    virtual ~ExecutionTarget();

    //Domain/Location attributes
    std::string DomainName;
    std::string Owner;
    std::string Address;
    std::string Place;
    std::string PostCode;
    float Latitude;
    float Longitude;

    //ComputingService and ComputingEndpoint attributes
    std::string CEID;
    std::string CEName;
    std::string Capability;
    std::string Type;
    std::string QualityLevel;
    URL url;
    std::string Technology;
    std::string Interface;
    std::string InterfaceExtension;
    std::string SupportedProfile;
    std::string Implementor;
    std::string ImplementationName;
    std::string ImplementationVersion;
    std::string HealthState;
    std::string ServingState;
    std::string IssuerCA;
    std::string TrustedCA;
    Time DowntimeStarts;
    Time DowntimeEnds;
    std::string Staging;
    std::string Jobdescription;

    //ComputingService/ComputingShare load attributes
    int TotalJobs;
    int RunningJobs;
    int WaitingJobs;
    int StagingJobs;
    int SuspendedJobs;
    int PreLRMSWaitingJobs;
    int LocalRunningJobs;
    int LocalWaitingJobs;
    int FreeSlots;
    std::string FreeSlotsWithDuration;
    int UsedSlots;
    int RequestedSlots;

    //ComputingShare
    std::string MappingQueue;
    Period MaxWallTime;
    Period MaxTotalWallTime;
    Period MinWallTime;
    Period DefaultWallTime;
    Period MaxCPUTime;
    Period MaxTotalCPUTime;
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
    Period EstimatedAverageWaitingTime;
    Period EstimatedWorstWaitingTime;
    std::string ReservationPolicy;

    //ComputingManager
    std::string ManagerType;
    bool Reservation;
    bool BulkSubmission;
    int TotalPhysicalCPUs;
    int TotalLogicalCPUs;
    int TotalSlots;
    bool Homogeneity;
    std::string NetworkInfo;
    bool WorkingAreaShared;
    int WorkingAreaFree;
    Period WorkingAreaLifeTime;
    int CacheFree;

    //ExecutionEnvironment
    std::string Platform;
    bool VirtualMachine;
    std::string CPUVendor;
    std::string CPUModel;
    std::string CPUVersion;
    int CPUClockSpeed;
    int MainMemorySize;
    std::string OSFamily;
    std::string OSName;
    std::string OSVersion;
    bool ConnectivityIn;
    bool ConnectivityOut;
    std::list<Benchmark> Benchmarks;

    //ApplicationEnvironment
    std::list<ApplicationEnvironment> ApplicationEnvironments;

    //Other
    Submitter *GetSubmitter() const;
    std::string GridFlavour;
    URL Source;
    URL Cluster;

  private:

    Loader *loader;

  };

} // namespace Arc

#endif // __ARC_EXECUTIONTARGET_H__
