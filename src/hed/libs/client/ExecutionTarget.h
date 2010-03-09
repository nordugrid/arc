// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_EXECUTIONTARGET_H__
#define __ARC_EXECUTIONTARGET_H__

#include <list>
#include <map>
#include <string>

#include <arc/DateTime.h>
#include <arc/URL.h>
#include <arc/client/JobDescription.h>
#include <arc/client/Software.h>
#include <arc/client/Submitter.h>

namespace Arc {

  class Submitter;
  class UserConfig;

  /// ApplicationEnvironment
  /**
   * The ApplicationEnviroment is closely related to the definition given in
   * GLUE2. By extending the Software class the two GLUE2 attributes AppName and
   * AppVersion are mapped to two private members. However these can be obtained
   * through the inheriated member methods getName and getVersion.
   *
   * GLUE2 description:
   * A description of installed application software or software environment
   * characteristics available within one or more Execution Environments.
   */
  class ApplicationEnvironment
    : public Software {
  public:
    ApplicationEnvironment() {}
    ApplicationEnvironment(const std::string& Name)
      : Software(Name) {}
    ApplicationEnvironment(const std::string& Name, const std::string& Version)
      : Software(Name, Version) {}
    ApplicationEnvironment& operator=(const Software& sv) {
      Software::operator=(sv);
      return *this;
    }
    std::string State;
    int FreeSlots;
    int FreeJobs;
    int FreeUserSeats;
  };


  /// ExecutionTarget
  /**
   * This class describe a target which accept computing jobs. All of the
   * members contained in this class, with a few exceptions, are directly
   * linked to attributes defined in the GLUE Specification v. 2.0
   * (GFD-R-P.147).
   */

  class ExecutionTarget {

  public:
    /// Create an ExecutionTarget
    /**
     * Default constructor to create an ExecutionTarget. Takes no
     * arguments.
     **/
    ExecutionTarget();

    /// Create an ExecutionTarget
    /**
     * Copy constructor.
     *
     * @param target ExecutionTarget to copy.
     **/
    ExecutionTarget(const ExecutionTarget& target);

    /// Create an ExecutionTarget
    /**
     * Copy constructor? Needed from Python?
     *
     * @param addrptr
     *
     **/
    ExecutionTarget(const long int addrptr);

    /// Create an ExecutionTarget
    /**
     * Assignment operator
     *
     * @param target is ExecutionTarget to copy.
     *
     **/
    ExecutionTarget& operator=(const ExecutionTarget& target);

    virtual ~ExecutionTarget();

  private:
    void Copy(const ExecutionTarget& target);

  public:

    /// Get Submitter to the computing resource represented by the ExecutionTarget
    /**
     * Method which returns a specialized Submitter which can be used
     * for submitting jobs to the computing resource represented by
     * the ExecutionTarget. In order to return the correct specialized
     * Submitter the GridFlavour variable must be correctly set.
     *
     * @param ucfg UserConfig object with paths to user credentials
     * etc.
     **/
    Submitter* GetSubmitter(const UserConfig& ucfg) const;


    /// Update ExecutionTarget after succesful job submission
    /**
     * Method to update the ExecutionTarget after a job succesfully
     * has been submitted to the computing resource it
     * represents. E.g. if a job is sent to the computing resource and
     * is expected to enter the queue, then the WaitingJobs attribute
     * is incremented with 1.
     *
     * @param jobdesc contains all information about the job
     * submitted.
     **/
    void Update(const JobDescription& jobdesc);

    /// Print the ExecutionTarget information to std::cout
    /**
     * Method to print the ExecutionTarget attributes to std::cout
     *
     * @param longlist is true for long list printing.
     **/
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
    Software Implementation;
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

    /// ComputingShareName String 0..1
    /**
     * Human-readable name.
     * This variable represents the ComputingShare.Name attribute of GLUE2.
     **/
    std::string ComputingShareName;

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

    /// MaxMainMemory UInt64 0..1 MB
    /**
     * The maximum physical RAM that a job is allowed to use; if the limit is
     * hit, then the LRMS MAY kill the job.
     * A negative value specifies that this member is undefined.
     */
    int64_t MaxMainMemory;

    /// MaxVirtualMemory UInt64 0..1 MB
    /**
     * The maximum total memory size (RAM plus swap) that a job is allowed to
     * use; if the limit is hit, then the LRMS MAY kill the job.
     * A negative value specifies that this member is undefined.
     */
    int64_t MaxVirtualMemory;

    /// MaxDiskSpace UInt64 0..1 GB
    /**
     * The maximum disk space that a job is allowed use in the working; if the
     * limit is hit, then the LRMS MAY kill the job.
     * A negative value specifies that this member is undefined.
     */
    int64_t MaxDiskSpace;

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

    /// OperatingSystem
    /**
     * The OperatingSystem member is not present in GLUE2 but contains the three
     * GLUE2 attributes OSFamily, OSName and OSVersion.
     * - OSFamily OSFamily_t 1
     *   * The general family to which the Execution Environment operating
     *   * system belongs.
     * - OSName OSName_t 0..1
     *   * The specific name of the operating sytem
     * - OSVersion String 0..1
     *   * The version of the operating system, as defined by the vendor.
     */
    Software OperatingSystem;

    bool ConnectivityIn;
    bool ConnectivityOut;

    /// ApplicationEnvironments
    /**
     * The ApplicationEnvironments member is a list of
     * ApplicationEnvironment's, defined in section 6.7 GLUE2.
     */
    std::list<ApplicationEnvironment> ApplicationEnvironments;

    // Other

    std::string GridFlavour;
    URL Cluster; // contains the URL of the infosys that provided the info

  private:
    SubmitterLoader loader;
  };

} // namespace Arc

#endif // __ARC_EXECUTIONTARGET_H__
