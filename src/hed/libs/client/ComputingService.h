/**
 * GLUE2 ComputingService class
 */
#ifndef GLUE2_COMPUTINGSERVICE_T
#define GLUE2_COMPUTINGSERVICE_T

#include "ComputingEndpoint.h"
#include "ComputingShare.h"
#include "ComputingResource.h"
#include "ComputingActivity.h"
#include "Contact.h"
#include "Service.h"
#include <string>
#include <list>

namespace Glue2{

  class ComputingService_t : public Glue2::Service_t{
  public:
    ComputingService_t(){};
    ~ComputingService_t(){};
    
    int TotalJobs;
    int RunningJobs;
    int WaitingJobs;
    int StagingJobs;
    int SuspendedJobs;
    int PreLRMSWaitingJobs;
    Glue2::Contact_t Contact;
    
    std::list<ComputingEndpoint_t> ComputingEndpoint;
    std::list<ComputingShare_t> ComputingShare;
    std::list<ComputingResource_t> ComputingResource;
    std::list<ComputingActivity_t> ComputingActivities;    

  };//end class

}//end namespace

#endif
