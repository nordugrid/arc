// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_JOB_H__
#define __ARC_JOB_H__

#include <arc/DateTime.h>
#include <arc/URL.h>
#include <arc/client/JobState.h>
#include <string>

namespace Arc {

  class Logger;

  /// Job
  /**
   * This class describe a Grid job. Most of the members contained in
   * this class are directly linked to the ComputingActivity defined
   * in the GLUE Specification v. 2.0 (GFD-R-P.147).
   */
  class Job {
  public:

    /// Create a Job object
    /**
     * Default constructor. Takes no arguments.
     */
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

    // Information retrieved from the information system
    std::string Name;
    std::string Type;
    URL IDFromEndpoint;
    std::string LocalIDFromManager;
    std::string JobDescription;
    JobState State;
    std::string RestartState;
    std::map<std::string, std::string> AuxStates; //for all state models
    std::map<std::string, std::string> RestartStates; //for all state models
    int ExitCode;
    std::string ComputingManagerExitCode;
    std::list<std::string> Error;
    int WaitingPosition;
    std::string UserDomain;
    std::string Owner;
    std::string LocalOwner;
    Period RequestedTotalWallTime;
    Period RequestedTotalCPUTime;
    int RequestedMainMemory; // Deprecated??
    int RequestedSlots;
    std::list<std::string> RequestedApplicationEnvironment;
    std::string StdIn;
    std::string StdOut;
    std::string StdErr;
    std::string LogDir;
    std::list<std::string> ExecutionNode;
    std::string ExecutionCE; // Deprecated??
    std::string Queue;
    Period UsedTotalWallTime;
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
    std::list<std::string> OtherMessages;
    //Associations
    URL JobManagementEndpoint;
    URL DataStagingEndpoint;
    std::list<std::string> ActivityOldId;
    //ExecutionEnvironment (condensed)
    bool VirtualMachine;
    std::string UsedCPUType;
    std::string UsedOSFamily;
    std::string UsedPlatform;

    /// DEPRECATED: Print the Job information to std::cout
    /**
     * This method is DEPRECATED, use the SaveToStream method instead. Method to
     * print the Job attributes to std::cout
     *
     * @param longlist is boolean for long listing (more details).
     * @see SaveToStream
     */
    void Print(bool longlist) const;

    /// Print the Job information to a std::ostream object
    /**
     * This method is used to print Job attributes to a std::ostream object.
     *
     * @param out is the std::ostream object to print the attributes to.
     * @param longlist is boolean for long listing (more details).
     */
    void SaveToStream(std::ostream& out, bool longlist) const;

    static bool CompareJobID(const Job* a, const Job* b);
    static bool CompareSubmissionTime(const Job* a, const Job* b);
    static bool CompareJobName(const Job* a, const Job* b);

  private:
    static Logger logger;
  };

} // namespace Arc

#endif // __ARC_JOB_H__
