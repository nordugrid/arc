#ifndef __ARC_EXECUTIONTARGET_H__
#define __ARC_EXECUTIONTARGET_H__

#include <list>
#include <map>
#include <string>

#include <arc/DateTime.h>
#include <arc/URL.h>

namespace Arc {

  class ACCLoader;
  class Submitter;
  class UserConfig;

  struct ApplicationEnvironment {
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
    ExecutionTarget(const ExecutionTarget& target);
    ExecutionTarget(const long int addrptr);
    ExecutionTarget& operator=(const ExecutionTarget& target);
    virtual ~ExecutionTarget();

  private:
    void Copy(const ExecutionTarget& target);

  public:
    Submitter* GetSubmitter(const UserConfig& ucfg) const;
    void Print(bool longlist) const;

    // Attributes from 5.3 Location

    std::string Address;
    std::string Place;
    std::string Country;
    std::string PostCode;
    float Latitude;
    float Longitude;

    // Attributes from 5.5.1 Admin Domain

    std::string DomainName;
    std::string Owner;

    // Attributes from 6.1 Computing Service

    std::string ServiceName;
    std::string ServiceType;

    // Attributes from 6.2 Computing Endpoint

    URL url;
    std::list<std::string> Capability;
    std::string Technology;
    std::string InterfaceName;
    std::list<std::string> InterfaceVersion;
    std::list<std::string> InterfaceExtension;
    std::list<std::string> SupportedProfile;
    std::string Implementor;
    std::string ImplementationName;
    std::string ImplementationVersion;
    std::string QualityLevel;
    std::string HealthState;
    std::string HealthStateInfo;
    std::string ServingState;
    std::string IssuerCA;
    std::list<std::string> TrustedCA;
    Time DowntimeStarts;
    Time DowntimeEnds;
    std::string Staging;
    std::list<std::string> JobDescriptions;

    // Attributes from 6.3 Computing Share

    std::string MappingQueue;
    Period MaxWallTime;
    Period MaxTotalWallTime; // not in current Glue2 draft
    Period MinWallTime;
    Period DefaultWallTime;
    Period MaxCPUTime;
    Period MaxTotalCPUTime;
    Period MinCPUTime;
    Period DefaultCPUTime;
    int MaxTotalJobs;
    int MaxRunningJobs;
    int MaxWaitingJobs;
    int MaxPreLRMSWaitingJobs;
    int MaxUserRunningJobs;
    int MaxSlotsPerJob;
    int MaxStageInStreams;
    int MaxStageOutStreams;
    std::string SchedulingPolicy;
    int MaxMainMemory;
    int MaxVirtualMemory;
    int MaxDiskSpace;
    URL DefaultStorageService;
    bool Preemption;
    int TotalJobs;
    int RunningJobs;
    int LocalRunningJobs;
    int WaitingJobs;
    int LocalWaitingJobs;
    int SuspendedJobs;
    int LocalSuspendedJobs;
    int StagingJobs;
    int PreLRMSWaitingJobs;
    Period EstimatedAverageWaitingTime;
    Period EstimatedWorstWaitingTime;
    int FreeSlots;
    std::map<Period, int> FreeSlotsWithDuration;
    int UsedSlots;
    int RequestedSlots;
    std::string ReservationPolicy;

    // Attributes from 6.4 Computing Manager

    std::string ManagerProductName;
    std::string ManagerProductVersion;
    bool Reservation;
    bool BulkSubmission;
    int TotalPhysicalCPUs;
    int TotalLogicalCPUs;
    int TotalSlots;
    bool Homogeneous;
    std::list<std::string> NetworkInfo;
    bool WorkingAreaShared;
    int WorkingAreaTotal;
    int WorkingAreaFree;
    Period WorkingAreaLifeTime;
    int CacheTotal;
    int CacheFree;

    // Attributes from 6.5 Benchmark

    std::map<std::string, double> Benchmarks;

    // Attributes from 6.6 Execution Environment

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

    // Attributes from 6.7 Application Environment

    std::list<ApplicationEnvironment> ApplicationEnvironments;

    // Other

    std::string GridFlavour;
    URL Cluster; // contains the URL of the infosys that provided the info

  private:
    ACCLoader *loader;
  };

} // namespace Arc

#endif // __ARC_EXECUTIONTARGET_H__
