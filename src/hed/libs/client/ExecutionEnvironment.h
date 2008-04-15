/**
 * GLUE2 ExecutionEnvironment_t class
 */
#ifndef GLUE2_EXECUTIONENVIRONMENT_T
#define GLUE2_EXECUTIONENVIRONMENT_T

#include "enums.h"
#include <arc/URL.h>
#include <string>

namespace Glue2{

  class ExecutionEnvironment_t{
  public:

    ExecutionEnvironment_t(){};
    ~ExecutionEnvironment_t(){};

    std::string LocalID;
    Platform_t PlatformType;
    bool VirtualMachine;
    int TotalInstances;
    int UsedInstances;
    int UnavailableInstances;
    int PhysicalCPUs;
    int LogicalCPUs;
    CPUMultiplicity_t CPUMultiplicity;
    std::string CPUVendor;
    std::string CPUModel;
    std::string CPUVersion;
    int CPUClockSpeed;
    float CPUTimeScalingFactor;
    float WallTimeScalingFactor;
    long MainMemorySize;
    long VirtualMemorySize;
    OSFamily_t OSFamily;
    OSName_t OSName;
    std::string OSVersion;
    bool ConnectivityIn;
    bool ConnectivityOut;
    NetworkInfo_t NetworkInfo;
    std::list<Benchmark_t> Benchmark;
    std::list<std::string> ComputingShareLocalID;
    std::list<std::string> ApplicationEnvironmentLocalID;
    std::list<Arc::URL> ComputingActivityID;

  };//end class executionenvironment_t

} //end namespace

#endif
