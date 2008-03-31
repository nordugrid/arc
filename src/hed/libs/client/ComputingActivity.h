/**
 * GLUE2 ComputingActivity class
 */
#ifndef ARCLIB_COMPUTINGACTIVITY
#define ARCLIB_COMPUTINGACTIVITY

#include "Activity.h"
#include "enums.h"
#include <arc/DateTime.h>
#include <string>
#include <list>

namespace Arc{


  class ComputingActivity : public Arc::Activity{
  public:
    ComputingActivity();
    ~ComputingActivity();

    std::string LRMSID;
    std::string Name;
    ComputingActivityState_t State;
    ComputingActivityState_t RestartState;    
    int ExitCode;
    std::string LRMSExitCode;
    std::string Error;
    int LRMSWaitingPosition;
    std::string UserDomain;
    std::string Owner;
    std::string LocalOwner;
    int RequestedWallTime;
    int RequestedCPUTime;
    std::list<std::string> RequestedApplicationEnvironment;
    int RequestedCPUs;
    std::string Stdin;
    std::string Stdout;
    std::string Stderr;
    std::string LogDir;
    std::list<std::string> ExecutionNode;
    std::string QueueName;
    int UsedWallTime;
    int UsedCPUTime;
    int UsedMainMemory;
    Arc::Time SubmissionTime;
    Arc::Time LRMSSubmissionTime;
    Arc::Time StartTime;
    Arc::Time LRMSEndTime;
    Arc::Time EndTime;
    Arc::Time GridAreaEraseTime;
    Arc::Time ProxyExpirationTime;
    std::string SubmissionHost;
    std::string SubmissionClientName;
    std::list<std::string> OtherMessages;


  }; //end class

} //end namespace

#endif
