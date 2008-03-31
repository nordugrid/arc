/**
 * GLUE2 ComputingService class
 */
#ifndef ARCLIB_COMPUTINGSERVICE
#define ARCLIB_COMPUTINGSERVICE

#include "ComputingEndpoint.h"
#include "ComputingShare.h"
#include "ComputingResource.h"
#include "ComputingActivity.h"
#include <string>
#include <list>

namespace Arc{

  class ComputingService : public Arc::Service{
  public:
    ComputingService();
    ~ComputingService();
    
    int TotalJobs;
    int RunningJobs;
    int WaitingJobs;
    int StagingJobs;
    int SuspendedJobs;
    int PreLRMSWaitingJobs;
    
    std::list<ComputingEndpoint> m_ComputingEndpoint;
    std::list<ComputingShare> m_ComputingShare;
    std::list<ComputingResource> m_ComputingResource;
    std::list<ComputingActivity> m_ComputingActivities;    

  };//end class

}//end namespace

#endif
