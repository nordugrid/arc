/**
 * GLUE2 ComputingResource class
 */
#ifndef GLUE2_COMPUTINGRESOURCE_T
#define GLUE2_COMPUTINGRESOURCE_T

#include "ExecutionEnvironment.h"
#include "ApplicationEnvironment.h"
#include "Resource.h"
#include "enums.h"
#include <string>

namespace Glue2{

  class ComputingResource_t : public Glue2::Resource_t{
  public:
    ComputingResource_t(){};
    ~ComputingResource_t(){};

    LRMSType_t LRMSType;
    std::string LRMSVersion;
    std::string LRMSOtherInfo;
    int TotalSlots;
    int SlotsUsedByLocalJobs;
    int SlotsUsedByGridJobs;
    int TotalPhysicalCPUs;
    int TotalLogicalCPUs;
    std::string TmpDir;
    std::string ScratchDir;
    std::string DataDir;
    bool Homogeneity;
    NetworkInfo_t NetworkInfo;
    std::string LogicalCPUDistribution;
    int GridAreaTotal;
    int GridAreaFree;
    int GridAreaLifeTime;
    int CacheTotal;
    int CacheFree;
    std::list<ExecutionEnvironment_t> ExecutionEnvironment;
    std::list<ApplicationEnvironment_t> ApplicationEnvironments;
    Benchmark_t Benchmark;


  };//end class

}//end namespace

#endif
