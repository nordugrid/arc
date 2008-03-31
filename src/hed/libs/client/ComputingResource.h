/**
 * GLUE2 ComputingResource class
 */
#ifndef ARCLIB_COMPUTINGRESOURCE
#define ARCLIB_COMPUTINGRESOURCE

#include "Resource.h"
#include "enums.h"
#include <string>

namespace Arc{

  class ComputingResource : public Arc::Resource{
  public:
    ComputingResource();
    ~ComputingResource();

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

  };//end class

}//end namespace

#endif
