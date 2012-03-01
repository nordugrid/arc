// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/ArcConfig.h>
#include <arc/Logger.h>
#include <arc/XMLNode.h>
#include <arc/client/ClientInterface.h>
#include <arc/client/ExecutionTarget.h>
#include <arc/client/Submitter.h>
#include <arc/UserConfig.h>
#include <arc/StringConv.h>

namespace Arc {

  Logger ExecutionTarget::logger(Logger::getRootLogger(), "ExecutionTarget");

  ExecutionTarget::ExecutionTarget()
    : Latitude(0),
      Longitude(0),
      MaxWallTime(-1),
      MaxTotalWallTime(-1),
      MinWallTime(-1),
      DefaultWallTime(-1),
      MaxCPUTime(-1),
      MaxTotalCPUTime(-1),
      MinCPUTime(-1),
      DefaultCPUTime(-1),
      MaxTotalJobs(-1),
      MaxRunningJobs(-1),
      MaxWaitingJobs(-1),
      MaxPreLRMSWaitingJobs(-1),
      MaxUserRunningJobs(-1),
      MaxSlotsPerJob(-1),
      MaxStageInStreams(-1),
      MaxStageOutStreams(-1),
      MaxMainMemory(-1),
      MaxVirtualMemory(-1),
      MaxDiskSpace(-1),
      Preemption(false),
      TotalJobs(-1),
      RunningJobs(-1),
      LocalRunningJobs(-1),
      WaitingJobs(-1),
      LocalWaitingJobs(-1),
      SuspendedJobs(-1),
      LocalSuspendedJobs(-1),
      StagingJobs(-1),
      PreLRMSWaitingJobs(-1),
      EstimatedAverageWaitingTime(-1),
      EstimatedWorstWaitingTime(-1),
      FreeSlots(-1),
      UsedSlots(-1),
      RequestedSlots(-1),
      Reservation(false),
      BulkSubmission(false),
      TotalPhysicalCPUs(-1),
      TotalLogicalCPUs(-1),
      TotalSlots(-1),
      Homogeneous(true),
      WorkingAreaShared(true),
      WorkingAreaTotal(-1),
      WorkingAreaFree(-1),
      WorkingAreaLifeTime(-1),
      CacheTotal(-1),
      CacheFree(-1),
      VirtualMachine(false),
      CPUClockSpeed(-1),
      MainMemorySize(-1),
      ConnectivityIn(false),
      ConnectivityOut(false) {}

  ExecutionTarget::ExecutionTarget(const ExecutionTarget& target) {
    Copy(target);
  }

  ExecutionTarget::ExecutionTarget(const long int addrptr) {
    Copy(*(ExecutionTarget*)addrptr);
  }

  ExecutionTarget& ExecutionTarget::operator=(const ExecutionTarget& target) {
    Copy(target);
    return *this;
  }

  ExecutionTarget::~ExecutionTarget() {}

  void ExecutionTarget::Copy(const ExecutionTarget& target) {

    // Attributes from 5.3 Location

    Address = target.Address;
    Place = target.Place;
    Country = target.Country;
    PostCode = target.PostCode;
    Latitude = target.Latitude;
    Longitude = target.Longitude;

    // Attributes from 5.5.1 Admin Domain

    DomainName = target.DomainName;
    Owner = target.Owner;

    // Attributes from 6.1 Computing Service

    ServiceName = target.ServiceName;
    ServiceType = target.ServiceType;

    // Attributes from 6.2 Computing Endpoint

    ComputingEndpoint = target.ComputingEndpoint;

    // Attributes from 6.3 Computing Share

    ComputingShareName = target.ComputingShareName;
    MappingQueue = target.MappingQueue;
    MaxWallTime = target.MaxWallTime;
    MaxTotalWallTime = target.MaxTotalWallTime;
    MinWallTime = target.MinWallTime;
    DefaultWallTime = target.DefaultWallTime;
    MaxCPUTime = target.MaxCPUTime;
    MaxTotalCPUTime = target.MaxTotalCPUTime;
    MinCPUTime = target.MinCPUTime;
    DefaultCPUTime = target.DefaultCPUTime;
    MaxTotalJobs = target.MaxTotalJobs;
    MaxRunningJobs = target.MaxRunningJobs;
    MaxWaitingJobs = target.MaxWaitingJobs;
    MaxPreLRMSWaitingJobs = target.MaxPreLRMSWaitingJobs;
    MaxUserRunningJobs = target.MaxUserRunningJobs;
    MaxSlotsPerJob = target.MaxSlotsPerJob;
    MaxStageInStreams = target.MaxStageInStreams;
    MaxStageOutStreams = target.MaxStageOutStreams;
    SchedulingPolicy = target.SchedulingPolicy;
    MaxMainMemory = target.MaxMainMemory;
    MaxVirtualMemory = target.MaxVirtualMemory;
    MaxDiskSpace = target.MaxDiskSpace;
    DefaultStorageService = target.DefaultStorageService;
    Preemption = target.Preemption;
    TotalJobs = target.TotalJobs;
    RunningJobs = target.RunningJobs;
    LocalRunningJobs = target.LocalRunningJobs;
    WaitingJobs = target.WaitingJobs;
    LocalWaitingJobs = target.LocalWaitingJobs;
    SuspendedJobs = target.SuspendedJobs;
    LocalSuspendedJobs = target.LocalSuspendedJobs;
    StagingJobs = target.StagingJobs;
    PreLRMSWaitingJobs = target.PreLRMSWaitingJobs;
    EstimatedAverageWaitingTime = target.EstimatedAverageWaitingTime;
    EstimatedWorstWaitingTime = target.EstimatedWorstWaitingTime;
    FreeSlots = target.FreeSlots;
    FreeSlotsWithDuration = target.FreeSlotsWithDuration;
    UsedSlots = target.UsedSlots;
    RequestedSlots = target.RequestedSlots;
    ReservationPolicy = target.ReservationPolicy;

    // Attributes from 6.4 Computing Manager

    ManagerProductName = target.ManagerProductName;
    ManagerProductVersion = target.ManagerProductVersion;
    Reservation = target.Reservation;
    BulkSubmission = target.BulkSubmission;
    TotalPhysicalCPUs = target.TotalPhysicalCPUs;
    TotalLogicalCPUs = target.TotalLogicalCPUs;
    TotalSlots = target.TotalSlots;
    Homogeneous = target.Homogeneous;
    NetworkInfo = target.NetworkInfo;
    WorkingAreaShared = target.WorkingAreaShared;
    WorkingAreaTotal = target.WorkingAreaTotal;
    WorkingAreaFree = target.WorkingAreaFree;
    WorkingAreaLifeTime = target.WorkingAreaLifeTime;
    CacheTotal = target.CacheTotal;
    CacheFree = target.CacheFree;

    // Attributes from 6.5 Benchmark

    Benchmarks = target.Benchmarks;

    // Attributes from 6.6 Execution Environment

    Platform = target.Platform;
    VirtualMachine = target.VirtualMachine;
    CPUVendor = target.CPUVendor;
    CPUModel = target.CPUModel;
    CPUVersion = target.CPUVersion;
    CPUClockSpeed = target.CPUClockSpeed;
    MainMemorySize = target.MainMemorySize;
    OperatingSystem = target.OperatingSystem;
    ConnectivityIn = target.ConnectivityIn;
    ConnectivityOut = target.ConnectivityOut;

    // Attributes from 6.7 Application Environment

    ApplicationEnvironments = target.ApplicationEnvironments;

    // Other

    GridFlavour = target.GridFlavour;
    Cluster = target.Cluster;
  }

  Submitter* ExecutionTarget::GetSubmitter(const UserConfig& ucfg) const {
    Submitter* s = (const_cast<ExecutionTarget*>(this))->loader.load(GridFlavour, ucfg);
    if (s == NULL) {
      return s;
    }
    s->SetSubmissionTarget(*this);
    return s;
  }

  void ExecutionTarget::Update(const JobDescription& jobdesc) {

    //WorkingAreaFree
    if (jobdesc.Resources.DiskSpaceRequirement.DiskSpace) {
      WorkingAreaFree -= (int)(jobdesc.Resources.DiskSpaceRequirement.DiskSpace / 1024);
      if (WorkingAreaFree < 0)
        WorkingAreaFree = 0;
    }

    // FreeSlotsWithDuration
    if (!FreeSlotsWithDuration.empty()) {
      std::map<Period, int>::iterator cpuit, cpuit2;
      cpuit = FreeSlotsWithDuration.lower_bound((unsigned int)jobdesc.Resources.TotalCPUTime.range);
      if (cpuit != FreeSlotsWithDuration.end()) {
        if (jobdesc.Resources.SlotRequirement.NumberOfSlots >= cpuit->second)
          cpuit->second = 0;
        else
          for (cpuit2 = FreeSlotsWithDuration.begin();
               cpuit2 != FreeSlotsWithDuration.end(); cpuit2++) {
            if (cpuit2->first <= cpuit->first)
              cpuit2->second -= jobdesc.Resources.SlotRequirement.NumberOfSlots;
            else if (cpuit2->second >= cpuit->second) {
              cpuit2->second = cpuit->second;
              Period oldkey = cpuit->first;
              cpuit++;
              FreeSlotsWithDuration.erase(oldkey);
            }
          }

        if (cpuit->second == 0)
          FreeSlotsWithDuration.erase(cpuit->first);

        if (FreeSlotsWithDuration.empty()) {
          if (MaxWallTime != -1)
            FreeSlotsWithDuration[MaxWallTime] = 0;
          else
            FreeSlotsWithDuration[LONG_MAX] = 0;
        }
      }
    }

    //FreeSlots, UsedSlots, WaitingJobs
    if (FreeSlots >= abs(jobdesc.Resources.SlotRequirement.NumberOfSlots)) { //The job will start directly
      FreeSlots -= abs(jobdesc.Resources.SlotRequirement.NumberOfSlots);
      if (UsedSlots != -1)
        UsedSlots += abs(jobdesc.Resources.SlotRequirement.NumberOfSlots);
    }
    else if (WaitingJobs != -1)    //The job will enter the queue (or the cluster doesn't report FreeSlots)
      WaitingJobs += abs(jobdesc.Resources.SlotRequirement.NumberOfSlots);

    return;

  }

  void ExecutionTarget::SaveToStream(std::ostream& out, bool longlist) const {

    out << IString("Execution Service: %s", (!ServiceName.empty() ? ServiceName : Cluster.Host())) << std::endl;
    if (Cluster) {
      std::string formattedURL = Cluster.str();
      formattedURL.erase(std::remove(formattedURL.begin(), formattedURL.end(), ' '), formattedURL.end()); // Remove spaces.
      std::string::size_type pos = formattedURL.find("?"); // Do not output characters after the '?' character.
      out << IString(" URL: %s:%s", GridFlavour, formattedURL.substr(0, pos)) << std::endl;
    }
    if (!ComputingShareName.empty()) {
       out << IString(" Queue: %s", ComputingShareName) << std::endl;
    }
    if (!MappingQueue.empty()) {
       out << IString(" Mapping queue: %s", MappingQueue) << std::endl;
    }
    if (!ComputingEndpoint.HealthState.empty()){
      out << IString(" Health state: %s", ComputingEndpoint.HealthState) << std::endl;
    }
    if (longlist) {

      out << std::endl << IString("Location information:") << std::endl;

      if (!Address.empty())
        out << IString(" Address: %s", Address) << std::endl;
      if (!Place.empty())
        out << IString(" Place: %s", Place) << std::endl;
      if (!Country.empty())
        out << IString(" Country: %s", Country) << std::endl;
      if (!PostCode.empty())
        out << IString(" Postal code: %s", PostCode) << std::endl;
      if (Latitude != 0)
        out << IString(" Latitude: %f", Latitude) << std::endl;
      if (Longitude != 0)
        out << IString(" Longitude: %f", Longitude) << std::endl;

      out << std::endl << IString("Domain information:") << std::endl;

      if (!Owner.empty())
        out << IString(" Owner: %s", Owner) << std::endl;

      out << std::endl << IString("Service information:") << std::endl;

      if (!ServiceName.empty())
        out << IString(" Service name: %s", ServiceName) << std::endl;
      if (!ServiceName.empty())
        out << IString(" Service type: %s", ServiceType) << std::endl;

      out << std::endl << IString("Endpoint information:") << std::endl;

      if (!ComputingEndpoint.URLString.empty())
        out << IString(" URL: %s", ComputingEndpoint.URLString) << std::endl;
      if (!ComputingEndpoint.Capability.empty()) {
        out << IString(" Capabilities:") << std::endl;
        for (std::list<std::string>::const_iterator it = ComputingEndpoint.Capability.begin();
             it != ComputingEndpoint.Capability.end(); it++)
          out << "  " << *it << std::endl;
      }
      if (!ComputingEndpoint.Technology.empty())
        out << IString(" Technology: %s", ComputingEndpoint.Technology) << std::endl;
      if (!ComputingEndpoint.InterfaceName.empty())
        out << IString(" Interface name: %s", ComputingEndpoint.InterfaceName) << std::endl;
      if (!ComputingEndpoint.InterfaceVersion.empty()) {
        out << IString(" Interface versions:") << std::endl;
        for (std::list<std::string>::const_iterator it = ComputingEndpoint.InterfaceVersion.begin();
             it != ComputingEndpoint.InterfaceVersion.end(); ++it)
          out << "  " << *it << std::endl;
      }
      if (!ComputingEndpoint.InterfaceExtension.empty()) {
        out << IString(" Interface extensions:") << std::endl;
        for (std::list<std::string>::const_iterator it = ComputingEndpoint.InterfaceExtension.begin();
             it != ComputingEndpoint.InterfaceExtension.end(); ++it)
          out << "  " << *it << std::endl;
      }
      if (!ComputingEndpoint.SupportedProfile.empty()) {
        out << IString(" Supported Profiles:") << std::endl;
        for (std::list<std::string>::const_iterator it = ComputingEndpoint.SupportedProfile.begin();
             it != ComputingEndpoint.SupportedProfile.end(); ++it)
          out << "  " << *it << std::endl;
      }
      if (!ComputingEndpoint.Implementor.empty())
        out << IString(" Implementor: %s", ComputingEndpoint.Implementor) << std::endl;
      if (!ComputingEndpoint.Implementation().empty())
        out << IString(" Implementation name: %s", (std::string)ComputingEndpoint.Implementation)
                  << std::endl;
      if (!ComputingEndpoint.QualityLevel.empty())
        out << IString(" Quality level: %s", ComputingEndpoint.QualityLevel) << std::endl;
      if (!ComputingEndpoint.HealthState.empty())
        out << IString(" Health state: %s", ComputingEndpoint.HealthState) << std::endl;
      if (!ComputingEndpoint.HealthStateInfo.empty())
        out << IString(" Health state info: %s", ComputingEndpoint.HealthStateInfo)
                  << std::endl;
      if (!ComputingEndpoint.ServingState.empty())
        out << IString(" Serving state: %s", ComputingEndpoint.ServingState) << std::endl;

      if (ApplicationEnvironments.size() > 0) {
        out << IString(" Installed application environments:") << std::endl;
        for (std::list<ApplicationEnvironment>::const_iterator it = ApplicationEnvironments.begin();
             it != ApplicationEnvironments.end(); it++) {
          out << "  " << *it << std::endl;
        }
      }

      if (ConnectivityIn)
        out << IString(" Execution environment"
                             " supports inbound connections") << std::endl;
      else
        out << IString(" Execution environment does not"
                             " support inbound connections") << std::endl;
      if (ConnectivityOut)
        out << IString(" Execution environment"
                             " supports outbound connections") << std::endl;
      else
        out << IString(" Execution environment does not"
                             " support outbound connections") << std::endl;

      if (!ComputingEndpoint.IssuerCA.empty())
        out << IString(" Issuer CA: %s", ComputingEndpoint.IssuerCA) << std::endl;
      if (!ComputingEndpoint.TrustedCA.empty()) {
        out << IString(" Trusted CAs:") << std::endl;
        for (std::list<std::string>::const_iterator it = ComputingEndpoint.TrustedCA.begin();
             it != ComputingEndpoint.TrustedCA.end(); ++it)
          out << "  " << *it << std::endl;
      }
      if (ComputingEndpoint.DowntimeStarts != -1)
        out << IString(" Downtime starts: %s", ComputingEndpoint.DowntimeStarts.str())<< std::endl;
      if (ComputingEndpoint.DowntimeEnds != -1)
        out << IString(" Downtime ends: %s", ComputingEndpoint.DowntimeEnds.str()) << std::endl;
      if (!ComputingEndpoint.Staging.empty())
        out << IString(" Staging: %s", ComputingEndpoint.Staging) << std::endl;
      if (!ComputingEndpoint.JobDescriptions.empty()) {
        out << IString(" Job descriptions:") << std::endl;
        for (std::list<std::string>::const_iterator it = ComputingEndpoint.JobDescriptions.begin();
             it != ComputingEndpoint.JobDescriptions.end(); ++it)
          out << "  " << *it << std::endl;
      }

      out << std::endl << IString("Queue information:") << std::endl;

      if (!ComputingShareName.empty())
        out << IString(" Name: %s", ComputingShareName) << std::endl;
      if (MaxWallTime != -1)
        out << IString(" Max wall-time: %s", MaxWallTime.istr())
                  << std::endl;
      if (MaxTotalWallTime != -1)
        out << IString(" Max total wall-time: %s",
                             MaxTotalWallTime.istr()) << std::endl;
      if (MinWallTime != -1)
        out << IString(" Min wall-time: %s", MinWallTime.istr())
                  << std::endl;
      if (DefaultWallTime != -1)
        out << IString(" Default wall-time: %s",
                             DefaultWallTime.istr()) << std::endl;
      if (MaxCPUTime != -1)
        out << IString(" Max CPU time: %s", MaxCPUTime.istr())
                  << std::endl;
      if (MinCPUTime != -1)
        out << IString(" Min CPU time: %s", MinCPUTime.istr())
                  << std::endl;
      if (DefaultCPUTime != -1)
        out << IString(" Default CPU time: %s",
                             DefaultCPUTime.istr()) << std::endl;
      if (MaxTotalJobs != -1)
        out << IString(" Max total jobs: %i", MaxTotalJobs) << std::endl;
      if (MaxRunningJobs != -1)
        out << IString(" Max running jobs: %i", MaxRunningJobs)
                  << std::endl;
      if (MaxWaitingJobs != -1)
        out << IString(" Max waiting jobs: %i", MaxWaitingJobs)
                  << std::endl;
      if (MaxPreLRMSWaitingJobs != -1)
        out << IString(" Max pre-LRMS waiting jobs: %i",
                             MaxPreLRMSWaitingJobs) << std::endl;
      if (MaxUserRunningJobs != -1)
        out << IString(" Max user running jobs: %i", MaxUserRunningJobs)
                  << std::endl;
      if (MaxSlotsPerJob != -1)
        out << IString(" Max slots per job: %i", MaxSlotsPerJob)
                  << std::endl;
      if (MaxStageInStreams != -1)
        out << IString(" Max stage in streams: %i", MaxStageInStreams)
                  << std::endl;
      if (MaxStageOutStreams != -1)
        out << IString(" Max stage out streams: %i", MaxStageOutStreams)
                  << std::endl;
      if (!SchedulingPolicy.empty())
        out << IString(" Scheduling policy: %s", SchedulingPolicy)
                  << std::endl;
      if (MaxMainMemory != -1)
        out << IString(" Max memory: %i", MaxMainMemory) << std::endl;
      if (MaxVirtualMemory != -1)
        out << IString(" Max virtual memory: %i", MaxVirtualMemory)
                  << std::endl;
      if (MaxDiskSpace != -1)
        out << IString(" Max disk space: %i", MaxDiskSpace) << std::endl;
      if (DefaultStorageService)
        out << IString(" Default Storage Service: %s",
                             DefaultStorageService.str()) << std::endl;
      if (Preemption)
        out << IString(" Supports preemption") << std::endl;
      else
        out << IString(" Doesn't support preemption") << std::endl;
      if (TotalJobs != -1)
        out << IString(" Total jobs: %i", TotalJobs) << std::endl;
      if (RunningJobs != -1)
        out << IString(" Running jobs: %i", RunningJobs) << std::endl;
      if (LocalRunningJobs != -1)
        out << IString(" Local running jobs: %i", LocalRunningJobs)
                  << std::endl;
      if (WaitingJobs != -1)
        out << IString(" Waiting jobs: %i", WaitingJobs) << std::endl;
      if (LocalWaitingJobs != -1)
        out << IString(" Local waiting jobs: %i", LocalWaitingJobs)
                  << std::endl;
      if (SuspendedJobs != -1)
        out << IString(" Suspended jobs: %i", SuspendedJobs)
                  << std::endl;
      if (LocalSuspendedJobs != -1)
        out << IString(" Local suspended jobs: %i", LocalSuspendedJobs)
                  << std::endl;
      if (StagingJobs != -1)
        out << IString(" Staging jobs: %i", StagingJobs) << std::endl;
      if (PreLRMSWaitingJobs != -1)
        out << IString(" Pre-LRMS waiting jobs: %i", PreLRMSWaitingJobs)
                  << std::endl;
      if (EstimatedAverageWaitingTime != -1)
        out << IString(" Estimated average waiting time: %s",
                             EstimatedAverageWaitingTime.istr())
                  << std::endl;
      if (EstimatedWorstWaitingTime != -1)
        out << IString(" Estimated worst waiting time: %s",
                             EstimatedWorstWaitingTime.istr())
                  << std::endl;
      if (FreeSlots != -1)
        out << IString(" Free slots: %i", FreeSlots) << std::endl;
      if (!FreeSlotsWithDuration.empty()) {
        out << IString(" Free slots grouped according to time limits (limit: free slots):") << std::endl;
        for (std::map<Period, int>::const_iterator it =
               FreeSlotsWithDuration.begin();
             it != FreeSlotsWithDuration.end(); it++) {
          if (it->first != Period(LONG_MAX)) {
            out << IString("  %s: %i", it->first.istr(), it->second)
                      << std::endl;
          }
          else {
            out << IString("  unspecified: %i", it->second)
                      << std::endl;
          }
        }
      }
      if (UsedSlots != -1)
        out << IString(" Used slots: %i", UsedSlots) << std::endl;
      if (RequestedSlots != -1)
        out << IString(" Requested slots: %i", RequestedSlots)
                  << std::endl;
      if (!ReservationPolicy.empty())
        out << IString(" Reservation policy: %s", ReservationPolicy)
                  << std::endl;

      out << std::endl << IString("Manager information:") << std::endl;

      if (!ManagerProductName.empty())
        out << IString(" Resource manager: %s", ManagerProductName)
                  << std::endl;
      if (!ManagerProductVersion.empty())
        out << IString(" Resource manager version: %s",
                             ManagerProductVersion) << std::endl;
      if (Reservation)
        out << IString(" Supports advance reservations") << std::endl;
      else
        out << IString(" Doesn't support advance reservations")
                  << std::endl;
      if (BulkSubmission)
        out << IString(" Supports bulk submission") << std::endl;
      else
        out << IString(" Doesn't support bulk Submission") << std::endl;
      if (TotalPhysicalCPUs != -1)
        out << IString(" Total physical CPUs: %i", TotalPhysicalCPUs)
                  << std::endl;
      if (TotalLogicalCPUs != -1)
        out << IString(" Total logical CPUs: %i", TotalLogicalCPUs)
                  << std::endl;
      if (TotalSlots != -1)
        out << IString(" Total slots: %i", TotalSlots) << std::endl;
      if (Homogeneous)
        out << IString(" Homogeneous resource") << std::endl;
      else
        out << IString(" Non-homogeneous resource") << std::endl;
      if (!NetworkInfo.empty()) {
        out << IString(" Network information:") << std::endl;
        for (std::list<std::string>::const_iterator it = NetworkInfo.begin();
             it != NetworkInfo.end(); it++)
          out << "  " << *it << std::endl;
      }
      if (WorkingAreaShared)
        out << IString(" Working area is shared among jobs")
                  << std::endl;
      else
        out << IString(" Working area is not shared among jobs")
                  << std::endl;
      if (WorkingAreaTotal != -1)
        out << IString(" Working area total size: %i", WorkingAreaTotal)
                  << std::endl;
      if (WorkingAreaFree != -1)
        out << IString(" Working area free size: %i", WorkingAreaFree)
                  << std::endl;
      if (WorkingAreaLifeTime != -1)
        out << IString(" Working area life time: %s",
                             WorkingAreaLifeTime.istr()) << std::endl;
      if (CacheTotal != -1)
        out << IString(" Cache area total size: %i", CacheTotal)
                  << std::endl;
      if (CacheFree != -1)
        out << IString(" Cache area free size: %i", CacheFree)
                  << std::endl;

      // Benchmarks

      if (!Benchmarks.empty()) {
        out << IString(" Benchmark information:") << std::endl;
        for (std::map<std::string, double>::const_iterator it =
               Benchmarks.begin(); it != Benchmarks.end(); it++)
          out << "  " << it->first << ": " << it->second << std::endl;
      }

      out << std::endl << IString("Execution environment information:")
                << std::endl;

      if (!Platform.empty())
        out << IString(" Platform: %s", Platform) << std::endl;
      if (VirtualMachine)
        out << IString(" Execution environment is a virtual machine")
                  << std::endl;
      else
        out << IString(" Execution environment is a physical machine")
                  << std::endl;
      if (!CPUVendor.empty())
        out << IString(" CPU vendor: %s", CPUVendor) << std::endl;
      if (!CPUModel.empty())
        out << IString(" CPU model: %s", CPUModel) << std::endl;
      if (!CPUVersion.empty())
        out << IString(" CPU version: %s", CPUVersion) << std::endl;
      if (CPUClockSpeed != -1)
        out << IString(" CPU clock speed: %i", CPUClockSpeed)
                  << std::endl;
      if (MainMemorySize != -1)
        out << IString(" Main memory size: %i", MainMemorySize)
                  << std::endl;

      if (!OperatingSystem.getFamily().empty())
        out << IString(" OS family: %s", OperatingSystem.getFamily()) << std::endl;
      if (!OperatingSystem.getName().empty())
        out << IString(" OS name: %s", OperatingSystem.getName()) << std::endl;
      if (!OperatingSystem.getVersion().empty())
        out << IString(" OS version: %s", OperatingSystem.getVersion()) << std::endl;
    } // end if long

    out << std::endl;

  } // end print

} // namespace Arc
