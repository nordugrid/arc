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
    : Location(), AdminDomain(), ComputingService(),
      ComputingEndpoint(), ComputingShare(),
      ComputingManager(),
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
    Location = target.Location;

    // Attributes from 5.5.1 Admin Domain
    AdminDomain = target.AdminDomain;

    // Attributes from 6.1 Computing Service
    ComputingService = target.ComputingService;

    // Attributes from 6.2 Computing Endpoint
    ComputingEndpoint = target.ComputingEndpoint;

    // Attributes from 6.3 Computing Share
    ComputingShare = target.ComputingShare;

    // Attributes from 6.4 Computing Manager
    ComputingManager = target.ComputingManager;

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
      ComputingManager.WorkingAreaFree -= (int)(jobdesc.Resources.DiskSpaceRequirement.DiskSpace / 1024);
      if (ComputingManager.WorkingAreaFree < 0)
        ComputingManager.WorkingAreaFree = 0;
    }

    // FreeSlotsWithDuration
    if (!ComputingShare.FreeSlotsWithDuration.empty()) {
      std::map<Period, int>::iterator cpuit, cpuit2;
      cpuit = ComputingShare.FreeSlotsWithDuration.lower_bound((unsigned int)jobdesc.Resources.TotalCPUTime.range);
      if (cpuit != ComputingShare.FreeSlotsWithDuration.end()) {
        if (jobdesc.Resources.SlotRequirement.NumberOfSlots >= cpuit->second)
          cpuit->second = 0;
        else
          for (cpuit2 = ComputingShare.FreeSlotsWithDuration.begin();
               cpuit2 != ComputingShare.FreeSlotsWithDuration.end(); cpuit2++) {
            if (cpuit2->first <= cpuit->first)
              cpuit2->second -= jobdesc.Resources.SlotRequirement.NumberOfSlots;
            else if (cpuit2->second >= cpuit->second) {
              cpuit2->second = cpuit->second;
              Period oldkey = cpuit->first;
              cpuit++;
              ComputingShare.FreeSlotsWithDuration.erase(oldkey);
            }
          }

        if (cpuit->second == 0)
          ComputingShare.FreeSlotsWithDuration.erase(cpuit->first);

        if (ComputingShare.FreeSlotsWithDuration.empty()) {
          if (ComputingShare.MaxWallTime != -1)
            ComputingShare.FreeSlotsWithDuration[ComputingShare.MaxWallTime] = 0;
          else
            ComputingShare.FreeSlotsWithDuration[LONG_MAX] = 0;
        }
      }
    }

    //FreeSlots, UsedSlots, WaitingJobs
    if (ComputingShare.FreeSlots >= abs(jobdesc.Resources.SlotRequirement.NumberOfSlots)) { //The job will start directly
      ComputingShare.FreeSlots -= abs(jobdesc.Resources.SlotRequirement.NumberOfSlots);
      if (ComputingShare.UsedSlots != -1)
        ComputingShare.UsedSlots += abs(jobdesc.Resources.SlotRequirement.NumberOfSlots);
    }
    else if (ComputingShare.WaitingJobs != -1)    //The job will enter the queue (or the cluster doesn't report FreeSlots)
      ComputingShare.WaitingJobs += abs(jobdesc.Resources.SlotRequirement.NumberOfSlots);

    return;

  }

  void ExecutionTarget::SaveToStream(std::ostream& out, bool longlist) const {

    out << IString("Execution Service: %s", (!ComputingService.Name.empty() ? ComputingService.Name : Cluster.Host())) << std::endl;
    if (Cluster) {
      std::string formattedURL = Cluster.str();
      formattedURL.erase(std::remove(formattedURL.begin(), formattedURL.end(), ' '), formattedURL.end()); // Remove spaces.
      std::string::size_type pos = formattedURL.find("?"); // Do not output characters after the '?' character.
      out << IString(" URL: %s:%s", GridFlavour, formattedURL.substr(0, pos)) << std::endl;
    }
    if (!ComputingShare.Name.empty()) {
       out << IString(" Queue: %s", ComputingShare.Name) << std::endl;
    }
    if (!ComputingShare.MappingQueue.empty()) {
       out << IString(" Mapping queue: %s", ComputingShare.MappingQueue) << std::endl;
    }
    if (!ComputingEndpoint.HealthState.empty()){
      out << IString(" Health state: %s", ComputingEndpoint.HealthState) << std::endl;
    }
    if (longlist) {

      out << std::endl << IString("Location information:") << std::endl;

      if (!Location.Address.empty())
        out << IString(" Address: %s", Location.Address) << std::endl;
      if (!Location.Place.empty())
        out << IString(" Place: %s", Location.Place) << std::endl;
      if (!Location.Country.empty())
        out << IString(" Country: %s", Location.Country) << std::endl;
      if (!Location.PostCode.empty())
        out << IString(" Postal code: %s", Location.PostCode) << std::endl;
      if (Location.Latitude != 0)
        out << IString(" Latitude: %f", Location.Latitude) << std::endl;
      if (Location.Longitude != 0)
        out << IString(" Longitude: %f", Location.Longitude) << std::endl;

      out << std::endl << IString("Domain information:") << std::endl;

      if (!AdminDomain.Owner.empty())
        out << IString(" Owner: %s", AdminDomain.Owner) << std::endl;

      out << std::endl << IString("Service information:") << std::endl;

      if (!ComputingService.Name.empty())
        out << IString(" Service name: %s", ComputingService.Name) << std::endl;
      if (!ComputingService.Type.empty())
        out << IString(" Service type: %s", ComputingService.Type) << std::endl;

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

      if (!ComputingShare.Name.empty())
        out << IString(" Name: %s", ComputingShare.Name) << std::endl;
      if (ComputingShare.MaxWallTime != -1)
        out << IString(" Max wall-time: %s", ComputingShare.MaxWallTime.istr())
                  << std::endl;
      if (ComputingShare.MaxTotalWallTime != -1)
        out << IString(" Max total wall-time: %s",
                             ComputingShare.MaxTotalWallTime.istr()) << std::endl;
      if (ComputingShare.MinWallTime != -1)
        out << IString(" Min wall-time: %s", ComputingShare.MinWallTime.istr())
                  << std::endl;
      if (ComputingShare.DefaultWallTime != -1)
        out << IString(" Default wall-time: %s",
                             ComputingShare.DefaultWallTime.istr()) << std::endl;
      if (ComputingShare.MaxCPUTime != -1)
        out << IString(" Max CPU time: %s", ComputingShare.MaxCPUTime.istr())
                  << std::endl;
      if (ComputingShare.MinCPUTime != -1)
        out << IString(" Min CPU time: %s", ComputingShare.MinCPUTime.istr())
                  << std::endl;
      if (ComputingShare.DefaultCPUTime != -1)
        out << IString(" Default CPU time: %s",
                             ComputingShare.DefaultCPUTime.istr()) << std::endl;
      if (ComputingShare.MaxTotalJobs != -1)
        out << IString(" Max total jobs: %i", ComputingShare.MaxTotalJobs) << std::endl;
      if (ComputingShare.MaxRunningJobs != -1)
        out << IString(" Max running jobs: %i", ComputingShare.MaxRunningJobs)
                  << std::endl;
      if (ComputingShare.MaxWaitingJobs != -1)
        out << IString(" Max waiting jobs: %i", ComputingShare.MaxWaitingJobs)
                  << std::endl;
      if (ComputingShare.MaxPreLRMSWaitingJobs != -1)
        out << IString(" Max pre-LRMS waiting jobs: %i",
                             ComputingShare.MaxPreLRMSWaitingJobs) << std::endl;
      if (ComputingShare.MaxUserRunningJobs != -1)
        out << IString(" Max user running jobs: %i", ComputingShare.MaxUserRunningJobs)
                  << std::endl;
      if (ComputingShare.MaxSlotsPerJob != -1)
        out << IString(" Max slots per job: %i", ComputingShare.MaxSlotsPerJob)
                  << std::endl;
      if (ComputingShare.MaxStageInStreams != -1)
        out << IString(" Max stage in streams: %i", ComputingShare.MaxStageInStreams)
                  << std::endl;
      if (ComputingShare.MaxStageOutStreams != -1)
        out << IString(" Max stage out streams: %i", ComputingShare.MaxStageOutStreams)
                  << std::endl;
      if (!ComputingShare.SchedulingPolicy.empty())
        out << IString(" Scheduling policy: %s", ComputingShare.SchedulingPolicy)
                  << std::endl;
      if (ComputingShare.MaxMainMemory != -1)
        out << IString(" Max memory: %i", ComputingShare.MaxMainMemory) << std::endl;
      if (ComputingShare.MaxVirtualMemory != -1)
        out << IString(" Max virtual memory: %i", ComputingShare.MaxVirtualMemory)
                  << std::endl;
      if (ComputingShare.MaxDiskSpace != -1)
        out << IString(" Max disk space: %i", ComputingShare.MaxDiskSpace) << std::endl;
      if (ComputingShare.DefaultStorageService)
        out << IString(" Default Storage Service: %s",
                             ComputingShare.DefaultStorageService.str()) << std::endl;
      if (ComputingShare.Preemption)
        out << IString(" Supports preemption") << std::endl;
      else
        out << IString(" Doesn't support preemption") << std::endl;
      if (ComputingShare.TotalJobs != -1)
        out << IString(" Total jobs: %i", ComputingShare.TotalJobs) << std::endl;
      if (ComputingShare.RunningJobs != -1)
        out << IString(" Running jobs: %i", ComputingShare.RunningJobs) << std::endl;
      if (ComputingShare.LocalRunningJobs != -1)
        out << IString(" Local running jobs: %i", ComputingShare.LocalRunningJobs)
                  << std::endl;
      if (ComputingShare.WaitingJobs != -1)
        out << IString(" Waiting jobs: %i", ComputingShare.WaitingJobs) << std::endl;
      if (ComputingShare.LocalWaitingJobs != -1)
        out << IString(" Local waiting jobs: %i", ComputingShare.LocalWaitingJobs)
                  << std::endl;
      if (ComputingShare.SuspendedJobs != -1)
        out << IString(" Suspended jobs: %i", ComputingShare.SuspendedJobs)
                  << std::endl;
      if (ComputingShare.LocalSuspendedJobs != -1)
        out << IString(" Local suspended jobs: %i", ComputingShare.LocalSuspendedJobs)
                  << std::endl;
      if (ComputingShare.StagingJobs != -1)
        out << IString(" Staging jobs: %i", ComputingShare.StagingJobs) << std::endl;
      if (ComputingShare.PreLRMSWaitingJobs != -1)
        out << IString(" Pre-LRMS waiting jobs: %i", ComputingShare.PreLRMSWaitingJobs)
                  << std::endl;
      if (ComputingShare.EstimatedAverageWaitingTime != -1)
        out << IString(" Estimated average waiting time: %s",
                             ComputingShare.EstimatedAverageWaitingTime.istr())
                  << std::endl;
      if (ComputingShare.EstimatedWorstWaitingTime != -1)
        out << IString(" Estimated worst waiting time: %s",
                             ComputingShare.EstimatedWorstWaitingTime.istr())
                  << std::endl;
      if (ComputingShare.FreeSlots != -1)
        out << IString(" Free slots: %i", ComputingShare.FreeSlots) << std::endl;
      if (!ComputingShare.FreeSlotsWithDuration.empty()) {
        out << IString(" Free slots grouped according to time limits (limit: free slots):") << std::endl;
        for (std::map<Period, int>::const_iterator it =
               ComputingShare.FreeSlotsWithDuration.begin();
             it != ComputingShare.FreeSlotsWithDuration.end(); it++) {
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
      if (ComputingShare.UsedSlots != -1)
        out << IString(" Used slots: %i", ComputingShare.UsedSlots) << std::endl;
      if (ComputingShare.RequestedSlots != -1)
        out << IString(" Requested slots: %i", ComputingShare.RequestedSlots)
                  << std::endl;
      if (!ComputingShare.ReservationPolicy.empty())
        out << IString(" Reservation policy: %s", ComputingShare.ReservationPolicy)
                  << std::endl;

      out << std::endl << IString("Manager information:") << std::endl;

      if (!ComputingManager.ProductName.empty())
        out << IString(" Resource manager: %s", ComputingManager.ProductName)
                  << std::endl;
      if (!ComputingManager.ProductVersion.empty())
        out << IString(" Resource manager version: %s",
                             ComputingManager.ProductVersion) << std::endl;
      if (ComputingManager.Reservation)
        out << IString(" Supports advance reservations") << std::endl;
      else
        out << IString(" Doesn't support advance reservations")
                  << std::endl;
      if (ComputingManager.BulkSubmission)
        out << IString(" Supports bulk submission") << std::endl;
      else
        out << IString(" Doesn't support bulk Submission") << std::endl;
      if (ComputingManager.TotalPhysicalCPUs != -1)
        out << IString(" Total physical CPUs: %i", ComputingManager.TotalPhysicalCPUs)
                  << std::endl;
      if (ComputingManager.TotalLogicalCPUs != -1)
        out << IString(" Total logical CPUs: %i", ComputingManager.TotalLogicalCPUs)
                  << std::endl;
      if (ComputingManager.TotalSlots != -1)
        out << IString(" Total slots: %i", ComputingManager.TotalSlots) << std::endl;
      if (ComputingManager.Homogeneous)
        out << IString(" Homogeneous resource") << std::endl;
      else
        out << IString(" Non-homogeneous resource") << std::endl;
      if (!ComputingManager.NetworkInfo.empty()) {
        out << IString(" Network information:") << std::endl;
        for (std::list<std::string>::const_iterator it = ComputingManager.NetworkInfo.begin();
             it != ComputingManager.NetworkInfo.end(); it++)
          out << "  " << *it << std::endl;
      }
      if (ComputingManager.WorkingAreaShared)
        out << IString(" Working area is shared among jobs")
                  << std::endl;
      else
        out << IString(" Working area is not shared among jobs")
                  << std::endl;
      if (ComputingManager.WorkingAreaTotal != -1)
        out << IString(" Working area total size: %i", ComputingManager.WorkingAreaTotal)
                  << std::endl;
      if (ComputingManager.WorkingAreaFree != -1)
        out << IString(" Working area free size: %i", ComputingManager.WorkingAreaFree)
                  << std::endl;
      if (ComputingManager.WorkingAreaLifeTime != -1)
        out << IString(" Working area life time: %s",
                             ComputingManager.WorkingAreaLifeTime.istr()) << std::endl;
      if (ComputingManager.CacheTotal != -1)
        out << IString(" Cache area total size: %i", ComputingManager.CacheTotal)
                  << std::endl;
      if (ComputingManager.CacheFree != -1)
        out << IString(" Cache area free size: %i", ComputingManager.CacheFree)
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
