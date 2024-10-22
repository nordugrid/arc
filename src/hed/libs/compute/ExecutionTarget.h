// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_EXECUTIONTARGET_H__
#define __ARC_EXECUTIONTARGET_H__

/** \file
 * \brief Structures holding resource information.
 */

#include <list>
#include <map>
#include <set>
#include <string>

#include <arc/DateTime.h>
#include <arc/compute/Endpoint.h>
#include <arc/URL.h>
#include <arc/Utils.h>
#include <arc/compute/GLUE2Entity.h>
#include <arc/compute/JobDescription.h>
#include <arc/compute/Software.h>
#include <arc/compute/SubmissionStatus.h>

namespace Arc {

  class Job;
  class Logger;
  class SubmitterPlugin;
  class UserConfig;

  /*
   * == Design considerations ==
   * In order to resemble the GLUE2 structure, the vital GLUE2 classes seen from
   * ARC perspective have a analogue below. However when doing match making, it
   * is desirable to have a complete and flat layout of a single submission
   * target configuration. E.g. two ComputingShares in a ComputingService gives
   * two configurations. Add different submission endpoints and the number of
   * configurations goes up... In order not to duplicate resource info instances
   * CountedPointer class is used indirectly in the GLUE2 master class
   * ComputingServiceType through the GLUE2Entity class, and directly in the
   * flat ExecutionTarget class (submission target configuration).
   */

  /**
   * \defgroup resourceinfo Structures holding resource information
   * \ingroup compute
   * The listed structures are all used for holding resource information when
   * doing resource discovery and those structures are read when doing match
   * making.
   *
   * \ingroup compute
   */

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
   *
   * \ingroup resourceinfo
   * \headerfile ExecutionTarget.h arc/compute/ExecutionTarget.h
   */
  class ApplicationEnvironment
    : public Software {
  public:
    ApplicationEnvironment() : FreeSlots(-1), FreeJobs(-1), FreeUserSeats(-1) {}
    ApplicationEnvironment(const std::string& Name)
      : Software(Name), FreeSlots(-1), FreeJobs(-1), FreeUserSeats(-1)  {}
    ApplicationEnvironment(const std::string& Name, const std::string& Version)
      : Software(Name, Version), FreeSlots(-1), FreeJobs(-1), FreeUserSeats(-1)  {}
    ApplicationEnvironment& operator=(const Software& sv) {
      Software::operator=(sv);
      return *this;
    }

    std::string State;
    int FreeSlots;
    int FreeJobs;
    int FreeUserSeats;
  };

  /**
   * \ingroup resourceinfo
   * \headerfile ExecutionTarget.h arc/compute/ExecutionTarget.h
   */
  class LocationAttributes {
  public:
    LocationAttributes() : Latitude(0), Longitude(0) {}

    std::string Address;
    std::string Place;
    std::string Country;
    std::string PostCode;
    float Latitude;
    float Longitude;

    friend std::ostream& operator<<(std::ostream& out, const LocationAttributes& l);
  };

  /**
   * \ingroup resourceinfo
   * \headerfile ExecutionTarget.h arc/compute/ExecutionTarget.h
   */
  class AdminDomainAttributes {
  public:
    std::string Name;
    std::string Owner;

    friend std::ostream& operator<<(std::ostream& out, const AdminDomainAttributes& ad);
  };

  /**
   * \ingroup resourceinfo
   * \headerfile ExecutionTarget.h arc/compute/ExecutionTarget.h
   */
  class ExecutionEnvironmentAttributes {
  public:
    ExecutionEnvironmentAttributes()
      : VirtualMachine(false), CPUClockSpeed(-1), MainMemorySize(-1),
        ConnectivityIn(false), ConnectivityOut(false) {}
    std::string ID;
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

    friend std::ostream& operator<<(std::ostream&, const ExecutionEnvironmentAttributes&);
  };

  /**
   * The MappingPolicyAttribtues class maps the vital attributes of the GLUE2
   * MappingPolicy class. MappingPolicy can e.g. reflect VO mapping to LRMS
   * queues (ComputingShare). The ComputingShareType class contains a list of
   * references to MappingPolicyAttributes objects.
   *
   * \since Added in 5.1.0
   * \ingroup resourceinfo
   * \headerfile ExecutionTarget.h arc/compute/ExecutionTarget.h
   */
  class MappingPolicyAttributes {
  public:
    MappingPolicyAttributes() {}

    /// A global unique ID
    std::string ID;

    /// Scheme adopted to define the policy rules
    std::string Scheme;

    /**
     * List of policy rules. E.g. exact match of DN or VO:
     * 'dn:/C=XX/O=YYYY/OU=Personal Certificate/L=ZZZZ/CN=NAME SURNAME' or 'vo:/vo_a'
     **/
    std::list<std::string> Rule;

    friend std::ostream& operator<<(std::ostream&, const MappingPolicyAttributes&);
  };

  /**
   * \ingroup resourceinfo
   * \headerfile ExecutionTarget.h arc/compute/ExecutionTarget.h
   */
  class ComputingManagerAttributes {
  public:
    ComputingManagerAttributes() :
      Reservation(false), BulkSubmission(false),
      TotalPhysicalCPUs(-1), TotalLogicalCPUs(-1), TotalSlots(-1),
      Homogeneous(true),
      WorkingAreaShared(true), WorkingAreaTotal(-1), WorkingAreaFree(-1), WorkingAreaLifeTime(-1),
      CacheTotal(-1), CacheFree(-1) {}

    std::string ID;
    std::string ProductName;
    std::string ProductVersion;
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

    friend std::ostream& operator<<(std::ostream&, const ComputingManagerAttributes&);
  };

  /**
   * \ingroup resourceinfo
   * \headerfile ExecutionTarget.h arc/compute/ExecutionTarget.h
   */
  class ComputingShareAttributes {
  public:
    ComputingShareAttributes() :
      MaxWallTime(-1), MaxTotalWallTime(-1), MinWallTime(-1), DefaultWallTime(-1),
      MaxCPUTime(-1), MaxTotalCPUTime(-1), MinCPUTime(-1), DefaultCPUTime(-1),
      MaxTotalJobs(-1), MaxRunningJobs(-1), MaxWaitingJobs(-1),
      MaxPreLRMSWaitingJobs(-1), MaxUserRunningJobs(-1), MaxSlotsPerJob(-1),
      MaxStageInStreams(-1), MaxStageOutStreams(-1),
      MaxMainMemory(-1), MaxVirtualMemory(-1), MaxDiskSpace(-1),
      Preemption(false),
      TotalJobs(-1), RunningJobs(-1), LocalRunningJobs(-1), WaitingJobs(-1), LocalWaitingJobs(-1),
      SuspendedJobs(-1), LocalSuspendedJobs(-1), StagingJobs(-1), PreLRMSWaitingJobs(-1),
      EstimatedAverageWaitingTime(-1), EstimatedWorstWaitingTime(-1),
      FreeSlots(-1), UsedSlots(-1), RequestedSlots(-1) {}

    std::string ID;
    /// Name String 0..1
    /**
     * Human-readable name.
     * This variable represents the ComputingShare.Name attribute of GLUE2.
     **/
    std::string Name;
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

    /// MaxMainMemory UInt64 0..1 MB
    /**
     * The maximum physical RAM that a job is allowed to use; if the limit is
     * hit, then the LRMS MAY kill the job.
     * A negative value specifies that this member is undefined.
     */
    int MaxMainMemory;

    /// MaxVirtualMemory UInt64 0..1 MB
    /**
     * The maximum total memory size (RAM plus swap) that a job is allowed to
     * use; if the limit is hit, then the LRMS MAY kill the job.
     * A negative value specifies that this member is undefined.
     */
    int MaxVirtualMemory;

    /// MaxDiskSpace UInt64 0..1 GB
    /**
     * The maximum disk space that a job is allowed use in the working; if the
     * limit is hit, then the LRMS MAY kill the job.
     * A negative value specifies that this member is undefined.
     */
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

    /// FreeSlotsWithDuration std::map<Period, int>
    /**
     * This attribute express the number of free slots with their time limits.
     * The keys in the std::map are the time limit (Period) for the number of
     * free slots stored as the value (int). If no time limit has been specified
     * for a set of free slots then the key will equal Period(LONG_MAX).
     */
    std::map<Period, int> FreeSlotsWithDuration;
    int UsedSlots;
    int RequestedSlots;
    std::string ReservationPolicy;

    friend std::ostream& operator<<(std::ostream&, const ComputingShareAttributes&);
  };

  /**
   * \ingroup resourceinfo
   * \headerfile ExecutionTarget.h arc/compute/ExecutionTarget.h
   */
  class ComputingEndpointAttributes {
  public:
    ComputingEndpointAttributes() : DowntimeStarts(-1), DowntimeEnds(-1),
      TotalJobs(-1), RunningJobs(-1), WaitingJobs(-1),
      StagingJobs(-1), SuspendedJobs(-1), PreLRMSWaitingJobs(-1) {}

    std::string ID;
    std::string URLString;
    std::string InterfaceName;
    std::string HealthState;
    std::string HealthStateInfo;
    std::string QualityLevel;
    std::set<std::string> Capability;
    std::string Technology;
    std::list<std::string> InterfaceVersion;
    std::list<std::string> InterfaceExtension;
    std::list<std::string> SupportedProfile;
    std::string Implementor;
    Software Implementation;
    std::string ServingState;
    std::string IssuerCA;
    std::list<std::string> TrustedCA;
    Time DowntimeStarts;
    Time DowntimeEnds;
    std::string Staging;
    int TotalJobs;
    int RunningJobs;
    int WaitingJobs;
    int StagingJobs;
    int SuspendedJobs;
    int PreLRMSWaitingJobs;
    // This is singular in the GLUE2 doc: JobDescription
    std::list<std::string> JobDescriptions;

    friend std::ostream& operator<<(std::ostream&, const ComputingEndpointAttributes&);
  };

  /**
   * \ingroup resourceinfo
   * \headerfile ExecutionTarget.h arc/compute/ExecutionTarget.h
   */
  class ComputingServiceAttributes {
  public:
    ComputingServiceAttributes() :
      TotalJobs(-1), RunningJobs(-1), WaitingJobs(-1),
      StagingJobs(-1), SuspendedJobs(-1), PreLRMSWaitingJobs(-1) {}
    std::string ID;
    std::string Name;
    std::string Type;
    std::set<std::string> Capability;
    std::string QualityLevel;
    int TotalJobs;
    int RunningJobs;
    int WaitingJobs;
    int StagingJobs;
    int SuspendedJobs;
    int PreLRMSWaitingJobs;

    // Other
    Endpoint InformationOriginEndpoint; // this ComputingService was generated while this Endpoint was queried

    friend std::ostream& operator<<(std::ostream& out, const ComputingServiceAttributes& cs);
  };

  /**
   * \ingroup resourceinfo
   * \headerfile ExecutionTarget.h arc/compute/ExecutionTarget.h
   */
  class LocationType : public GLUE2Entity<LocationAttributes> {};

  /**
   * \ingroup resourceinfo
   * \headerfile ExecutionTarget.h arc/compute/ExecutionTarget.h
   */
  class AdminDomainType : public GLUE2Entity<AdminDomainAttributes> {};

  /**
   * \ingroup resourceinfo
   * \headerfile ExecutionTarget.h arc/compute/ExecutionTarget.h
   */
  class ExecutionEnvironmentType : public GLUE2Entity<ExecutionEnvironmentAttributes> {};

  /**
   * Wrapper class for the MappingPolicyAttribtues class.
   *
   * \since Added in 5.1.0
   * \ingroup resourceinfo
   * \headerfile ExecutionTarget.h arc/compute/ExecutionTarget.h
   **/
   class MappingPolicyType : public GLUE2Entity<MappingPolicyAttributes> {};

  /**
   * \ingroup resourceinfo
   * \headerfile ExecutionTarget.h arc/compute/ExecutionTarget.h
   */
  class ComputingManagerType : public GLUE2Entity<ComputingManagerAttributes> {
  public:
    ComputingManagerType() : Benchmarks(new std::map<std::string, double>), ApplicationEnvironments(new std::list<ApplicationEnvironment>) {}
    // TODO: Currently using int as key, use std::string instead for holding ID.
    std::map<int, ExecutionEnvironmentType> ExecutionEnvironment;
    CountedPointer< std::map<std::string, double> > Benchmarks;
    /// ApplicationEnvironments
    /**
     * The ApplicationEnvironments member is a list of
     * ApplicationEnvironment's, defined in section 6.7 GLUE2.
     */
    CountedPointer< std::list<ApplicationEnvironment> > ApplicationEnvironments;
  };

  /**
   * \ingroup resourceinfo
   * \headerfile ExecutionTarget.h arc/compute/ExecutionTarget.h
   */
  class ComputingShareType : public GLUE2Entity<ComputingShareAttributes> {
  public:
    // TODO: Currently using int, use std::string instead for holding ID.
    std::set<int> ComputingEndpointIDs;

    /**
     * \since Added in 5.1.0
     **/
    std::map<int, MappingPolicyType> MappingPolicy;
  };

  /**
   * \ingroup resourceinfo
   * \headerfile ExecutionTarget.h arc/compute/ExecutionTarget.h
   */
  class ComputingEndpointType : public GLUE2Entity<ComputingEndpointAttributes> {
  public:
    // TODO: Currently using int, use std::string instead for holding ID.
    std::set<int> ComputingShareIDs;
  };

  /**
   * \ingroup compute
   * \ingroup resourceinfo
   * \headerfile ExecutionTarget.h arc/compute/ExecutionTarget.h
   */
  class ComputingServiceType : public GLUE2Entity<ComputingServiceAttributes> {
  public:
    template<typename T>
    void GetExecutionTargets(T& container) const;

    LocationType Location;
    AdminDomainType AdminDomain;
    // TODO: Currently using int as key, use std::string instead for holding ID.
    std::map<int, ComputingEndpointType> ComputingEndpoint;
    std::map<int, ComputingShareType> ComputingShare;
    std::map<int, ComputingManagerType> ComputingManager;

    friend std::ostream& operator<<(std::ostream&, const ComputingServiceType&);
  private:
    template<typename T>
    void AddExecutionTarget(T& container, const ExecutionTarget& et) const;
    static Logger logger;
  };

  // to avoid "explicit specialization before instantiation" errors
  template <>
  void ComputingServiceType::AddExecutionTarget< std::list<ExecutionTarget> >(std::list<ExecutionTarget>& etList, const ExecutionTarget& et) const;

  /// ExecutionTarget
  /**
   * This class describe a target which accept computing jobs. All of the
   * members contained in this class, with a few exceptions, are directly
   * linked to attributes defined in the GLUE Specification v. 2.0
   * (GFD-R-P.147).
   *
   * \ingroup compute
   * \ingroup resourceinfo
   * \headerfile ExecutionTarget.h arc/compute/ExecutionTarget.h
   */
  class ExecutionTarget {
  public:
    /// Create an ExecutionTarget
    /**
     * Default constructor to create an ExecutionTarget. Takes no
     * arguments.
     **/
    ExecutionTarget() :
      Location(new LocationAttributes()),
      AdminDomain(new AdminDomainAttributes()),
      ComputingService(new ComputingServiceAttributes()),
      ComputingEndpoint(new ComputingEndpointAttributes()),
      ComputingShare(new ComputingShareAttributes()),
      ComputingManager(new ComputingManagerAttributes()),
      ExecutionEnvironment(new ExecutionEnvironmentAttributes()),
      Benchmarks(new std::map<std::string, double>()),
      ApplicationEnvironments(new std::list<ApplicationEnvironment>()) {};

    /// Create an ExecutionTarget
    /**
     * Copy constructor.
     *
     * @param t ExecutionTarget to copy.
     **/
    ExecutionTarget(const ExecutionTarget& t) :
      Location(t.Location), AdminDomain(t.AdminDomain), ComputingService(t.ComputingService),
      ComputingEndpoint(t.ComputingEndpoint), OtherEndpoints(t.OtherEndpoints),
      ComputingShare(t.ComputingShare), MappingPolicies(t.MappingPolicies),
      ComputingManager(t.ComputingManager), ExecutionEnvironment(t.ExecutionEnvironment),
      Benchmarks(t.Benchmarks), ApplicationEnvironments(t.ApplicationEnvironments) {}

    /**
     * \since Changed in 5.1.0. List of MappingPolicyAttributes objects must
     *  also be passed.
     **/
    ExecutionTarget(const CountedPointer<LocationAttributes>& l,
                    const CountedPointer<AdminDomainAttributes>& a,
                    const CountedPointer<ComputingServiceAttributes>& cse,
                    const CountedPointer<ComputingEndpointAttributes>& ce,
                    const std::list< CountedPointer<ComputingEndpointAttributes> >& oe,
                    const CountedPointer<ComputingShareAttributes>& csh,
                    const std::list< CountedPointer<MappingPolicyAttributes> >& mp,
                    const CountedPointer<ComputingManagerAttributes>& cm,
                    const CountedPointer<ExecutionEnvironmentAttributes>& ee,
                    const CountedPointer< std::map<std::string, double> >& b,
                    const CountedPointer< std::list<ApplicationEnvironment> >& ae) :
      Location(l), AdminDomain(a), ComputingService(cse),
      ComputingEndpoint(ce), OtherEndpoints(oe),
      ComputingShare(csh), MappingPolicies(mp),
      ComputingManager(cm), ExecutionEnvironment(ee),
      Benchmarks(b), ApplicationEnvironments(ae) {}

    /// Create an ExecutionTarget
    /**
     * Copy constructor? Needed from Python?
     *
     * @param addrptr
     *
     **/
    ExecutionTarget(long int addrptr) :
      Location((*(ExecutionTarget*)addrptr).Location),
      AdminDomain((*(ExecutionTarget*)addrptr).AdminDomain),
      ComputingService((*(ExecutionTarget*)addrptr).ComputingService),
      ComputingEndpoint((*(ExecutionTarget*)addrptr).ComputingEndpoint),
      OtherEndpoints((*(ExecutionTarget*)addrptr).OtherEndpoints),
      ComputingShare((*(ExecutionTarget*)addrptr).ComputingShare),
      MappingPolicies((*(ExecutionTarget*)addrptr).MappingPolicies),
      ComputingManager((*(ExecutionTarget*)addrptr).ComputingManager),
      ExecutionEnvironment((*(ExecutionTarget*)addrptr).ExecutionEnvironment),
      Benchmarks((*(ExecutionTarget*)addrptr).Benchmarks),
      ApplicationEnvironments((*(ExecutionTarget*)addrptr).ApplicationEnvironments) {}


    ExecutionTarget& operator=(const ExecutionTarget& et) {
      Location = et.Location; AdminDomain = et.AdminDomain; ComputingService = et.ComputingService;
      ComputingEndpoint = et.ComputingEndpoint; ComputingEndpoint = et.ComputingEndpoint;
      ComputingShare = et.ComputingShare; MappingPolicies = et.MappingPolicies;
      ComputingManager = et.ComputingManager; Benchmarks = et.Benchmarks;
      ExecutionEnvironment = et.ExecutionEnvironment; ApplicationEnvironments = et.ApplicationEnvironments;
      return *this;
    }

    ~ExecutionTarget() {};

    SubmissionStatus Submit(const UserConfig& ucfg, const JobDescription& jobdesc, Job& job) const;

    /// Update ExecutionTarget after successful job submission
    /**
     * Method to update the ExecutionTarget after a job successfully
     * has been submitted to the computing resource it
     * represents. E.g. if a job is sent to the computing resource and
     * is expected to enter the queue, then the WaitingJobs attribute
     * is incremented with 1.
     *
     * @param jobdesc contains all information about the job
     * submitted.
     **/
    void RegisterJobSubmission(const JobDescription& jobdesc) const;

    /// Print the ExecutionTarget information
    /**
     * Method to print the ExecutionTarget attributes to a std::ostream object.
     *
     * @param out the std::ostream to print the attributes to.
     * @param et ExecutionTarget from which to obtain information
     * @return the input ostream object is returned.
     **/
    friend std::ostream& operator<<(std::ostream& out, const ExecutionTarget& et);

    static void GetExecutionTargets(const std::list<ComputingServiceType>& csList, std::list<ExecutionTarget>& etList);

    CountedPointer<LocationAttributes> Location;
    CountedPointer<AdminDomainAttributes> AdminDomain;
    CountedPointer<ComputingServiceAttributes> ComputingService;
    CountedPointer<ComputingEndpointAttributes> ComputingEndpoint;
    std::list< CountedPointer<ComputingEndpointAttributes> > OtherEndpoints;
    CountedPointer<ComputingShareAttributes> ComputingShare;
    std::list< CountedPointer<MappingPolicyAttributes> > MappingPolicies;
    CountedPointer<ComputingManagerAttributes> ComputingManager;
    CountedPointer<ExecutionEnvironmentAttributes> ExecutionEnvironment;
    CountedPointer< std::map<std::string, double> > Benchmarks;
    CountedPointer< std::list<ApplicationEnvironment> > ApplicationEnvironments;

  private:
    static Logger logger;
  };

} // namespace Arc

#endif // __ARC_EXECUTIONTARGET_H__
