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
      DowntimeStarts(-1),
      DowntimeEnds(-1),
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

    url = target.url;
    Capability = target.Capability;
    Technology = target.Technology;
    InterfaceName = target.InterfaceName;
    InterfaceVersion = target.InterfaceVersion;
    InterfaceExtension = target.InterfaceExtension;
    SupportedProfile = target.SupportedProfile;
    Implementor = target.Implementor;
    Implementation = target.Implementation;
    QualityLevel = target.QualityLevel;
    HealthState = target.HealthState;
    HealthStateInfo = target.HealthStateInfo;
    ServingState = target.ServingState;
    IssuerCA = target.IssuerCA;
    TrustedCA = target.TrustedCA;
    DowntimeStarts = target.DowntimeStarts;
    DowntimeEnds = target.DowntimeEnds;
    Staging = target.Staging;
    JobDescriptions = target.JobDescriptions;

    // Attributes from 6.3 Computing Share

    ComputingShareName = target.ComputingShareName;
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

  void ExecutionTarget::Print(bool longlist) const {
    logger.msg(WARNING, "The ExecutionTarget::Print method is DEPRECATED, use the ExecutionTarget::SaveToStream method instead.");
    SaveToStream(std::cout, longlist);
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
    if (!HealthState.empty()){
      out << IString(" Health State: %s", HealthState) << std::endl;
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
        out << IString(" Postal Code: %s", PostCode) << std::endl;
      if (Latitude != 0)
        out << IString(" Latitude: %f", Latitude) << std::endl;
      if (Longitude != 0)
        out << IString(" Longitude: %f", Longitude) << std::endl;

      out << std::endl << IString("Domain information:") << std::endl;

      if (!Owner.empty())
        out << IString(" Owner: %s", Owner) << std::endl;

      out << std::endl << IString("Service information:") << std::endl;

      if (!ServiceName.empty())
        out << IString(" Service Name: %s", ServiceName) << std::endl;
      if (!ServiceName.empty())
        out << IString(" Service Type: %s", ServiceType) << std::endl;

      out << std::endl << IString("Endpoint information:") << std::endl;

      if (url)
        out << IString(" URL: %s", url.str()) << std::endl;
      if (!Capability.empty()) {
        out << IString(" Capabilities:") << std::endl;
        for (std::list<std::string>::const_iterator it = Capability.begin();
             it != Capability.end(); it++)
          out << "  " << *it << std::endl;
      }
      if (!Technology.empty())
        out << IString(" Technology: %s", Technology) << std::endl;
      if (!InterfaceName.empty())
        out << IString(" Interface Name: %s", InterfaceName)
                  << std::endl;
      if (!InterfaceVersion.empty()) {
        out << IString(" Interface Versions:") << std::endl;
        for (std::list<std::string>::const_iterator it =
               InterfaceVersion.begin(); it != InterfaceVersion.end(); it++)
          out << "  " << *it << std::endl;
      }
      if (!InterfaceExtension.empty()) {
        out << IString(" Interface Extensions:") << std::endl;
        for (std::list<std::string>::const_iterator it =
               InterfaceExtension.begin();
             it != InterfaceExtension.end(); it++)
          out << "  " << *it << std::endl;
      }
      if (!SupportedProfile.empty()) {
        out << IString(" Supported Profiles:") << std::endl;
        for (std::list<std::string>::const_iterator it =
               SupportedProfile.begin(); it != SupportedProfile.end(); it++)
          out << "  " << *it << std::endl;
      }
      if (!Implementor.empty())
        out << IString(" Implementor: %s", Implementor) << std::endl;
      if (!Implementation().empty())
        out << IString(" Implementation Name: %s", (std::string)Implementation)
                  << std::endl;
      if (!QualityLevel.empty())
        out << IString(" Quality Level: %s", QualityLevel) << std::endl;
      if (!HealthState.empty())
        out << IString(" Health State: %s", HealthState) << std::endl;
      if (!HealthStateInfo.empty())
        out << IString(" Health State Info: %s", HealthStateInfo)
                  << std::endl;
      if (!ServingState.empty())
        out << IString(" Serving State: %s", ServingState) << std::endl;

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

      if (!IssuerCA.empty())
        out << IString(" Issuer CA: %s", IssuerCA) << std::endl;
      if (!TrustedCA.empty()) {
        out << IString(" Trusted CAs:") << std::endl;
        for (std::list<std::string>::const_iterator it = TrustedCA.begin();
             it != TrustedCA.end(); it++)
          out << "  " << *it << std::endl;
      }
      if (DowntimeStarts != -1)
        out << IString(" Downtime Starts: %s", DowntimeStarts.str())
                  << std::endl;
      if (DowntimeEnds != -1)
        out << IString(" Downtime Ends: %s", DowntimeEnds.str())
                  << std::endl;
      if (!Staging.empty())
        out << IString(" Staging: %s", Staging) << std::endl;
      if (!JobDescriptions.empty()) {
        out << IString(" Job Descriptions:") << std::endl;
        for (std::list<std::string>::const_iterator it =
               JobDescriptions.begin(); it != JobDescriptions.end(); it++)
          out << "  " << *it << std::endl;
      }

      out << std::endl << IString("Queue information:") << std::endl;

      if (!ComputingShareName.empty())
        out << IString(" Name: %s", ComputingShareName) << std::endl;
      if (MaxWallTime != -1)
        out << IString(" Max Wall Time: %s", MaxWallTime.istr())
                  << std::endl;
      if (MaxTotalWallTime != -1)
        out << IString(" Max Total Wall Time: %s",
                             MaxTotalWallTime.istr()) << std::endl;
      if (MinWallTime != -1)
        out << IString(" Min Wall Time: %s", MinWallTime.istr())
                  << std::endl;
      if (DefaultWallTime != -1)
        out << IString(" Default Wall Time: %s",
                             DefaultWallTime.istr()) << std::endl;
      if (MaxCPUTime != -1)
        out << IString(" Max CPU Time: %s", MaxCPUTime.istr())
                  << std::endl;
      if (MinCPUTime != -1)
        out << IString(" Min CPU Time: %s", MinCPUTime.istr())
                  << std::endl;
      if (DefaultCPUTime != -1)
        out << IString(" Default CPU Time: %s",
                             DefaultCPUTime.istr()) << std::endl;
      if (MaxTotalJobs != -1)
        out << IString(" Max Total Jobs: %i", MaxTotalJobs) << std::endl;
      if (MaxRunningJobs != -1)
        out << IString(" Max Running Jobs: %i", MaxRunningJobs)
                  << std::endl;
      if (MaxWaitingJobs != -1)
        out << IString(" Max Waiting Jobs: %i", MaxWaitingJobs)
                  << std::endl;
      if (MaxPreLRMSWaitingJobs != -1)
        out << IString(" Max Pre-LRMS Waiting Jobs: %i",
                             MaxPreLRMSWaitingJobs) << std::endl;
      if (MaxUserRunningJobs != -1)
        out << IString(" Max User Running Jobs: %i", MaxUserRunningJobs)
                  << std::endl;
      if (MaxSlotsPerJob != -1)
        out << IString(" Max Slots Per Job: %i", MaxSlotsPerJob)
                  << std::endl;
      if (MaxStageInStreams != -1)
        out << IString(" Max Stage In Streams: %i", MaxStageInStreams)
                  << std::endl;
      if (MaxStageOutStreams != -1)
        out << IString(" Max Stage Out Streams: %i", MaxStageOutStreams)
                  << std::endl;
      if (!SchedulingPolicy.empty())
        out << IString(" Scheduling Policy: %s", SchedulingPolicy)
                  << std::endl;
      if (MaxMainMemory != -1)
        out << IString(" Max Memory: %i", MaxMainMemory) << std::endl;
      if (MaxVirtualMemory != -1)
        out << IString(" Max Virtual Memory: %i", MaxVirtualMemory)
                  << std::endl;
      if (MaxDiskSpace != -1)
        out << IString(" Max Disk Space: %i", MaxDiskSpace) << std::endl;
      if (DefaultStorageService)
        out << IString(" Default Storage Service: %s",
                             DefaultStorageService.str()) << std::endl;
      if (Preemption)
        out << IString(" Supports Preemption") << std::endl;
      else
        out << IString(" Doesn't Support Preemption") << std::endl;
      if (TotalJobs != -1)
        out << IString(" Total Jobs: %i", TotalJobs) << std::endl;
      if (RunningJobs != -1)
        out << IString(" Running Jobs: %i", RunningJobs) << std::endl;
      if (LocalRunningJobs != -1)
        out << IString(" Local Running Jobs: %i", LocalRunningJobs)
                  << std::endl;
      if (WaitingJobs != -1)
        out << IString(" Waiting Jobs: %i", WaitingJobs) << std::endl;
      if (LocalWaitingJobs != -1)
        out << IString(" Local Waiting Jobs: %i", LocalWaitingJobs)
                  << std::endl;
      if (SuspendedJobs != -1)
        out << IString(" Suspended Jobs: %i", SuspendedJobs)
                  << std::endl;
      if (LocalSuspendedJobs != -1)
        out << IString(" Local Suspended Jobs: %i", LocalSuspendedJobs)
                  << std::endl;
      if (StagingJobs != -1)
        out << IString(" Staging Jobs: %i", StagingJobs) << std::endl;
      if (PreLRMSWaitingJobs != -1)
        out << IString(" Pre-LRMS Waiting Jobs: %i", PreLRMSWaitingJobs)
                  << std::endl;
      if (EstimatedAverageWaitingTime != -1)
        out << IString(" Estimated Average Waiting Time: %s",
                             EstimatedAverageWaitingTime.istr())
                  << std::endl;
      if (EstimatedWorstWaitingTime != -1)
        out << IString(" Estimated Worst Waiting Time: %s",
                             EstimatedWorstWaitingTime.istr())
                  << std::endl;
      if (FreeSlots != -1)
        out << IString(" Free Slots: %i", FreeSlots) << std::endl;
      if (!FreeSlotsWithDuration.empty()) {
        out << IString(" Free Slots Grouped According To Time Limits (limit: free slots):") << std::endl;
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
        out << IString(" Used Slots: %i", UsedSlots) << std::endl;
      if (RequestedSlots != -1)
        out << IString(" Requested Slots: %i", RequestedSlots)
                  << std::endl;
      if (!ReservationPolicy.empty())
        out << IString(" Reservation Policy: %s", ReservationPolicy)
                  << std::endl;

      out << std::endl << IString("Manager information:") << std::endl;

      if (!ManagerProductName.empty())
        out << IString(" Resource Manager: %s", ManagerProductName)
                  << std::endl;
      if (!ManagerProductVersion.empty())
        out << IString(" Resource Manager Version: %s",
                             ManagerProductVersion) << std::endl;
      if (Reservation)
        out << IString(" Supports Advance Reservations") << std::endl;
      else
        out << IString(" Doesn't Support Advance Reservations")
                  << std::endl;
      if (BulkSubmission)
        out << IString(" Supports Bulk Submission") << std::endl;
      else
        out << IString(" Doesn't Support Bulk Submission") << std::endl;
      if (TotalPhysicalCPUs != -1)
        out << IString(" Total Physical CPUs: %i", TotalPhysicalCPUs)
                  << std::endl;
      if (TotalLogicalCPUs != -1)
        out << IString(" Total Logical CPUs: %i", TotalLogicalCPUs)
                  << std::endl;
      if (TotalSlots != -1)
        out << IString(" Total Slots: %i", TotalSlots) << std::endl;
      if (Homogeneous)
        out << IString(" Homogeneous Resource") << std::endl;
      else
        out << IString(" Non-homogeneous Resource") << std::endl;
      if (!NetworkInfo.empty()) {
        out << IString(" Network Information:") << std::endl;
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
        out << IString(" Working Area Total Size: %i", WorkingAreaTotal)
                  << std::endl;
      if (WorkingAreaFree != -1)
        out << IString(" Working Area Free Size: %i", WorkingAreaFree)
                  << std::endl;
      if (WorkingAreaLifeTime != -1)
        out << IString(" Working Area Life Time: %s",
                             WorkingAreaLifeTime.istr()) << std::endl;
      if (CacheTotal != -1)
        out << IString(" Cache Area Total Size: %i", CacheTotal)
                  << std::endl;
      if (CacheFree != -1)
        out << IString(" Cache Area Free Size: %i", CacheFree)
                  << std::endl;

      // Benchmarks

      if (!Benchmarks.empty()) {
        out << IString(" Benchmark Information:") << std::endl;
        for (std::map<std::string, double>::const_iterator it =
               Benchmarks.begin(); it != Benchmarks.end(); it++)
          out << "  " << it->first << ": " << it->second << std::endl;
      }

      out << std::endl << IString("Execution Environment information:")
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
        out << IString(" CPU Vendor: %s", CPUVendor) << std::endl;
      if (!CPUModel.empty())
        out << IString(" CPU Model: %s", CPUModel) << std::endl;
      if (!CPUVersion.empty())
        out << IString(" CPU Version: %s", CPUVersion) << std::endl;
      if (CPUClockSpeed != -1)
        out << IString(" CPU Clock Speed: %i", CPUClockSpeed)
                  << std::endl;
      if (MainMemorySize != -1)
        out << IString(" Main Memory Size: %i", MainMemorySize)
                  << std::endl;

      if (!OperatingSystem.getFamily().empty())
        out << IString(" OS Family: %s", OperatingSystem.getFamily()) << std::endl;
      if (!OperatingSystem.getName().empty())
        out << IString(" OS Name: %s", OperatingSystem.getName()) << std::endl;
      if (!OperatingSystem.getVersion().empty())
        out << IString(" OS Version: %s", OperatingSystem.getVersion()) << std::endl;
    } // end if long

    out << std::endl;

  } // end print

} // namespace Arc
