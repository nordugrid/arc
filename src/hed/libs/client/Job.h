#ifndef __ARC_JOB_H__
#define __ARC_JOB_H__

#include <arc/DateTime.h>
#include <arc/URL.h>
#include <string>

namespace Arc {

  class Job {

  public:

    Job();
    ~Job();

    // Information stored in the job list file
    // Obligatory inforamtion
    std::string Flavour;
    URL JobID;
    URL Cluster;
    // Optional information (ACCs fills if they need it)
    URL SubmissionEndpoint;
    URL InfoEndpoint;
    URL ISB;
    URL OSB;
    // ACC implementation dependent information
    URL AuxURL;
    std::string AuxInfo;

    // Information retrieved from the informtaion system
    std::string Name;
    std::string Type;
    URL IDFromEndpoint;
    std::string LocalIdFromManager;
    std::string JobDescription;
    std::string State;
    std::string RestartState;
    int ExitCode;
    std::string ComputingManagerExitCode;
    std::list<std::string> Error;
    int WaitingPosition;
    std::string UserDomain;
    std::string Owner;
    std::string LocalOwner;
    Period RequestedWallTime;
    Period RequestedTotalCPUTime;
    int RequestedMainMemory;
    int RequestedSlots;
    std::string StdIn;
    std::string StdOut;
    std::string StdErr;
    std::string LogDir;
    std::list<std::string> ExecutionNode;
    std::string ExecutionCE;
    std::string Queue;
    Period UsedWallTime;
    Period UsedTotalCPUTime;
    int UsedMainMemory;
    std::list<std::string> UsedApplicationEnvironment;
    int UsedSlots;
    Time LocalSubmissionTime;
    Time SubmissionTime;
    Time ComputingManagerSubmissionTime;
    Time StartTime;
    Time ComputingManagerEndTime;
    Time EndTime;
    Time WorkingAreaEraseTime;
    Time ProxyExpirationTime;
    std::string SubmissionHost;
    std::string SubmissionClientName;
    Time CreationTime;
    Period Validity;
    std::string OtherMessages;
    //Associations
    URL JobManagementEndpoint;
    URL DataStagingEndpoint;
    std::list<URL> OldJobIDs;
    //ExecutionEnvironment (condensed)
    bool VirtualMachine;
    std::string UsedCPUType;
    std::string UsedOSFamily;
    std::string UsedPlatform;

    void Print(bool longlist) const;
  };

} // namespace Arc

#endif // __ARC_JOB_H__
