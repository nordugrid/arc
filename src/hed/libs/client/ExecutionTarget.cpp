// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/ArcConfig.h>
#include <arc/Logger.h>
#include <arc/XMLNode.h>
#include <arc/client/Broker.h>
#include <arc/client/ExecutionTarget.h>
#include <arc/client/Submitter.h>
#include <arc/UserConfig.h>
#include <arc/StringConv.h>

namespace Arc {

  Logger ExecutionTarget::logger(Logger::getRootLogger(), "ExecutionTarget");
  Logger ComputingServiceType::logger(Logger::getRootLogger(), "ComputingServiceType");

  template<typename T>
  void ComputingServiceType::GetExecutionTargets(T& container) const {
    // TODO: Currently assuming only one ComputingManager and one ExecutionEnvironment.
    for (std::map<int, ComputingEndpointType>::const_iterator itCE = ComputingEndpoint.begin();
         itCE != ComputingEndpoint.end(); ++itCE) {
      if (!Attributes->OriginalEndpoint.RequestedSubmissionInterfaceName.empty()) {
        // If this endpoint has a non-preferred job interface, we skip it
        if (itCE->second->InterfaceName != Attributes->OriginalEndpoint.RequestedSubmissionInterfaceName) {
          logger.msg(INFO,
            "Skipping ComputingEndpoint '%s', because it has '%s' interface instead of the requested '%s'.",
            itCE->second->URLString, itCE->second->InterfaceName, Attributes->OriginalEndpoint.RequestedSubmissionInterfaceName);
          continue;
        }
      }
      if (!itCE->second.ComputingShareIDs.empty()) {
        for (std::set<int>::const_iterator itCSIDs = itCE->second.ComputingShareIDs.begin();
             itCSIDs != itCE->second.ComputingShareIDs.end(); ++itCSIDs) {
          std::map<int, ComputingShareType>::const_iterator itCS = ComputingShare.find(*itCSIDs);
          if (itCS != ComputingShare.end() && !ComputingManager.empty() && !ComputingManager.begin()->second.ExecutionEnvironment.empty()) {
            AddExecutionTarget<T>(container, ExecutionTarget(Location.Attributes, AdminDomain.Attributes,
                                                             Attributes, itCE->second.Attributes,
                                                             itCS->second.Attributes, ComputingManager.begin()->second.Attributes,
                                                             ComputingManager.begin()->second.ExecutionEnvironment.begin()->second.Attributes, ComputingManager.begin()->second.Benchmarks,
                                                             ComputingManager.begin()->second.ApplicationEnvironments));
          }
        }
      }
      else {
        for (std::map<int, ComputingShareType>::const_iterator itCS = ComputingShare.begin();
             itCS != ComputingShare.end(); ++itCS) {
          if (!ComputingManager.empty() && !ComputingManager.begin()->second.ExecutionEnvironment.empty()) {
            AddExecutionTarget<T>(container, ExecutionTarget(Location.Attributes, AdminDomain.Attributes,
                                                             Attributes, itCE->second.Attributes,
                                                             itCS->second.Attributes, ComputingManager.begin()->second.Attributes,
                                                             ComputingManager.begin()->second.ExecutionEnvironment.begin()->second.Attributes, ComputingManager.begin()->second.Benchmarks,
                                                             ComputingManager.begin()->second.ApplicationEnvironments));
          }
        }
      }
    }
  }

  template void ComputingServiceType::GetExecutionTargets< std::list<ExecutionTarget> >(std::list<ExecutionTarget>&) const;
  template void ComputingServiceType::GetExecutionTargets<ExecutionTargetSet>(ExecutionTargetSet&) const;

  template<typename T>
  void ComputingServiceType::AddExecutionTarget(T&, const ExecutionTarget&) const {}
  
  template<>
  void ComputingServiceType::AddExecutionTarget< std::list<ExecutionTarget> >(std::list<ExecutionTarget>& etList, const ExecutionTarget& et) const {
    etList.push_back(et);
  }
  
  template<>
  void ComputingServiceType::AddExecutionTarget<ExecutionTargetSet>(ExecutionTargetSet& etSet, const ExecutionTarget& et) const {
    etSet.insert(et);
  }

  bool ExecutionTarget::Submit(const UserConfig& ucfg, const JobDescription& jobdesc, Job& job) const {
    return Submitter(ucfg).Submit(*this, jobdesc, job);
  }

  void ExecutionTarget::GetExecutionTargets(const std::list<ComputingServiceType>& csList, std::list<ExecutionTarget>& etList) {
    for (std::list<ComputingServiceType>::const_iterator it = csList.begin();
         it != csList.end(); ++it) {
      it->GetExecutionTargets(etList);
    }
  }

  void ExecutionTarget::RegisterJobSubmission(const JobDescription& jobdesc) const {

    //WorkingAreaFree
    if (jobdesc.Resources.DiskSpaceRequirement.DiskSpace) {
      ComputingManager->WorkingAreaFree -= (int)(jobdesc.Resources.DiskSpaceRequirement.DiskSpace / 1024);
      if (ComputingManager->WorkingAreaFree < 0)
        ComputingManager->WorkingAreaFree = 0;
    }

    // FreeSlotsWithDuration
    if (!ComputingShare->FreeSlotsWithDuration.empty()) {
      std::map<Period, int>::iterator cpuit, cpuit2;
      cpuit = ComputingShare->FreeSlotsWithDuration.lower_bound((unsigned int)jobdesc.Resources.TotalCPUTime.range);
      if (cpuit != ComputingShare->FreeSlotsWithDuration.end()) {
        if (jobdesc.Resources.SlotRequirement.NumberOfSlots >= cpuit->second)
          cpuit->second = 0;
        else
          for (cpuit2 = ComputingShare->FreeSlotsWithDuration.begin();
               cpuit2 != ComputingShare->FreeSlotsWithDuration.end(); cpuit2++) {
            if (cpuit2->first <= cpuit->first)
              cpuit2->second -= jobdesc.Resources.SlotRequirement.NumberOfSlots;
            else if (cpuit2->second >= cpuit->second) {
              cpuit2->second = cpuit->second;
              Period oldkey = cpuit->first;
              cpuit++;
              ComputingShare->FreeSlotsWithDuration.erase(oldkey);
            }
          }

        if (cpuit->second == 0)
          ComputingShare->FreeSlotsWithDuration.erase(cpuit->first);

        if (ComputingShare->FreeSlotsWithDuration.empty()) {
          if (ComputingShare->MaxWallTime != -1)
            ComputingShare->FreeSlotsWithDuration[ComputingShare->MaxWallTime] = 0;
          else
            ComputingShare->FreeSlotsWithDuration[LONG_MAX] = 0;
        }
      }
    }

    //FreeSlots, UsedSlots, WaitingJobs
    if (ComputingShare->FreeSlots >= abs(jobdesc.Resources.SlotRequirement.NumberOfSlots)) { //The job will start directly
      ComputingShare->FreeSlots -= abs(jobdesc.Resources.SlotRequirement.NumberOfSlots);
      if (ComputingShare->UsedSlots != -1)
        ComputingShare->UsedSlots += abs(jobdesc.Resources.SlotRequirement.NumberOfSlots);
    }
    else if (ComputingShare->WaitingJobs != -1)    //The job will enter the queue (or the cluster doesn't report FreeSlots)
      ComputingShare->WaitingJobs += abs(jobdesc.Resources.SlotRequirement.NumberOfSlots);

    return;
  }

  std::ostream& operator<<(std::ostream& out, const LocationAttributes& l) {
                             out << IString("Location information:") << std::endl;
    if (!l.Address.empty())  out << IString(" Address: %s", l.Address) << std::endl;
    if (!l.Place.empty())    out << IString(" Place: %s", l.Place) << std::endl;
    if (!l.Country.empty())  out << IString(" Country: %s", l.Country) << std::endl;
    if (!l.PostCode.empty()) out << IString(" Postal code: %s", l.PostCode) << std::endl;
    if (l.Latitude > 0)      out << IString(" Latitude: %f", l.Latitude) << std::endl;
    if (l.Longitude > 0)     out << IString(" Longitude: %f", l.Longitude) << std::endl;
    return out;
  }

  std::ostream& operator<<(std::ostream& out, const AdminDomainAttributes& ad) {
                           out << IString("Domain information:") << std::endl;
    if (!ad.Owner.empty()) out << IString(" Owner: %s", ad.Owner) << std::endl;
    return out;
  }

  std::ostream& operator<<(std::ostream& out, const ComputingServiceAttributes& cs) {
    if (!cs.Name.empty()) out << IString(" Name: %s", cs.Name) << std::endl;
    if (!cs.Type.empty()) out << IString(" Type: %s", cs.Type) << std::endl;
    return out;
  }

  void ComputingEndpointAttributes::SaveToStream(std::ostream& out, bool alldetails) const {
    if (!alldetails) {
      if (!URLString.empty())     out << IString(" URL: %s", URLString) << std::endl;
      if (!InterfaceName.empty()) out << IString(" Interface: %s", InterfaceName) << std::endl;
      if (!HealthState.empty())   out << IString(" Healthstate: %s", HealthState) << std::endl;
      return;
    }

    if (!URLString.empty())        out << IString(" URL: %s", URLString) << std::endl;
    if (!InterfaceName.empty())    out << IString(" Interface: %s", InterfaceName) << std::endl;
    if (!InterfaceVersion.empty()) {
                                   out << IString(" Interface versions:") << std::endl;
      for (std::list<std::string>::const_iterator it = InterfaceVersion.begin();
           it != InterfaceVersion.end(); ++it) out << "  " << *it << std::endl;
    }
    if (!InterfaceExtension.empty()) {
                                   out << IString(" Interface extensions:") << std::endl;
      for (std::list<std::string>::const_iterator it = InterfaceExtension.begin();
           it != InterfaceExtension.end(); ++it) out << "  " << *it << std::endl;
    }
    if (!Capability.empty()) {
                                   out << IString(" Capabilities:") << std::endl;
      for (std::list<std::string>::const_iterator it = Capability.begin();
           it != Capability.end(); ++it) out << "  " << *it << std::endl;
    }
    if (!Technology.empty())       out << IString(" Technology: %s", Technology) << std::endl;
    if (!SupportedProfile.empty()) {
                                      out << IString(" Supported Profiles:") << std::endl;
      for (std::list<std::string>::const_iterator it = SupportedProfile.begin();
           it != SupportedProfile.end(); ++it) out << "  " << *it << std::endl;
    }
    if (!Implementor.empty())      out << IString(" Implementor: %s", Implementor) << std::endl;
    if (!Implementation().empty()) out << IString(" Implementation name: %s", (std::string)Implementation) << std::endl;
    if (!QualityLevel.empty())     out << IString(" Quality level: %s", QualityLevel) << std::endl;
    if (!HealthState.empty())      out << IString(" Health state: %s", HealthState) << std::endl;
    if (!HealthStateInfo.empty())  out << IString(" Health state info: %s", HealthStateInfo) << std::endl;
    if (!ServingState.empty())     out << IString(" Serving state: %s", ServingState) << std::endl;
    if (!IssuerCA.empty())         out << IString(" Issuer CA: %s", IssuerCA) << std::endl;
    if (!TrustedCA.empty()) {
                                      out << IString(" Trusted CAs:") << std::endl;
      for (std::list<std::string>::const_iterator it = TrustedCA.begin();
           it != TrustedCA.end(); ++it) out << "  " << *it << std::endl;
    }
    if (DowntimeStarts > -1)       out << IString(" Downtime starts: %s", DowntimeStarts.str())<< std::endl;
    if (DowntimeEnds > -1)         out << IString(" Downtime ends: %s", DowntimeEnds.str()) << std::endl;
    if (!Staging.empty())          out << IString(" Staging: %s", Staging) << std::endl;
    if (!JobDescriptions.empty()) {
                                      out << IString(" Job descriptions:") << std::endl;
      for (std::list<std::string>::const_iterator it = JobDescriptions.begin();
           it != JobDescriptions.end(); ++it) out << "  " << *it << std::endl;
    }
  }

  void ComputingShareAttributes::SaveToStream(std::ostream& out, bool alldetails) const {
    if (!alldetails) {
      if (!Name.empty())                    out << IString(" Name: %s", Name) << std::endl;
      if (RunningJobs > -1)                 out << IString(" Running jobs: %i", RunningJobs) << std::endl;
      if (WaitingJobs > -1)                 out << IString(" Waiting jobs: %i", WaitingJobs) << std::endl;
      return;
    }

    // Following attributes is not printed:
    // Period MaxTotalCPUTime;
    // Period MaxTotalWallTime; // not in current Glue2 draft
    // std::string MappingQueue;
    // std::string ID;
    
    if (!Name.empty())                    out << IString(" Name: %s", Name) << std::endl;
    if (MaxWallTime > -1)                 out << IString(" Max wall-time: %s", MaxWallTime.istr()) << std::endl;
    if (MaxTotalWallTime > -1)            out << IString(" Max total wall-time: %s", MaxTotalWallTime.istr()) << std::endl;
    if (MinWallTime > -1)                 out << IString(" Min wall-time: %s", MinWallTime.istr()) << std::endl;
    if (DefaultWallTime > -1)             out << IString(" Default wall-time: %s", DefaultWallTime.istr()) << std::endl;
    if (MaxCPUTime > -1)                  out << IString(" Max CPU time: %s", MaxCPUTime.istr()) << std::endl;
    if (MinCPUTime > -1)                  out << IString(" Min CPU time: %s", MinCPUTime.istr()) << std::endl;
    if (DefaultCPUTime > -1)              out << IString(" Default CPU time: %s", DefaultCPUTime.istr()) << std::endl;
    if (MaxTotalJobs > -1)                out << IString(" Max total jobs: %i", MaxTotalJobs) << std::endl;
    if (MaxRunningJobs > -1)              out << IString(" Max running jobs: %i", MaxRunningJobs) << std::endl;
    if (MaxWaitingJobs > -1)              out << IString(" Max waiting jobs: %i", MaxWaitingJobs) << std::endl;
    if (MaxPreLRMSWaitingJobs > -1)       out << IString(" Max pre-LRMS waiting jobs: %i", MaxPreLRMSWaitingJobs) << std::endl;
    if (MaxUserRunningJobs > -1)          out << IString(" Max user running jobs: %i", MaxUserRunningJobs) << std::endl;
    if (MaxSlotsPerJob > -1)              out << IString(" Max slots per job: %i", MaxSlotsPerJob) << std::endl;
    if (MaxStageInStreams > -1)           out << IString(" Max stage in streams: %i", MaxStageInStreams) << std::endl;
    if (MaxStageOutStreams > -1)          out << IString(" Max stage out streams: %i", MaxStageOutStreams) << std::endl;
    if (!SchedulingPolicy.empty())        out << IString(" Scheduling policy: %s", SchedulingPolicy) << std::endl;
    if (MaxMainMemory > -1)               out << IString(" Max memory: %i", MaxMainMemory) << std::endl;
    if (MaxVirtualMemory > -1)            out << IString(" Max virtual memory: %i", MaxVirtualMemory) << std::endl;
    if (MaxDiskSpace > -1)                out << IString(" Max disk space: %i", MaxDiskSpace) << std::endl;
    if (DefaultStorageService)            out << IString(" Default Storage Service: %s", DefaultStorageService.str()) << std::endl;
    if (Preemption)                       out << IString(" Supports preemption") << std::endl;
    else                                     out << IString(" Doesn't support preemption") << std::endl;
    if (TotalJobs > -1)                   out << IString(" Total jobs: %i", TotalJobs) << std::endl;
    if (RunningJobs > -1)                 out << IString(" Running jobs: %i", RunningJobs) << std::endl;
    if (LocalRunningJobs > -1)            out << IString(" Local running jobs: %i", LocalRunningJobs) << std::endl;
    if (WaitingJobs > -1)                 out << IString(" Waiting jobs: %i", WaitingJobs) << std::endl;
    if (LocalWaitingJobs > -1)            out << IString(" Local waiting jobs: %i", LocalWaitingJobs) << std::endl;
    if (SuspendedJobs > -1)               out << IString(" Suspended jobs: %i", SuspendedJobs) << std::endl;
    if (LocalSuspendedJobs > -1)          out << IString(" Local suspended jobs: %i", LocalSuspendedJobs) << std::endl;
    if (StagingJobs > -1)                 out << IString(" Staging jobs: %i", StagingJobs) << std::endl;
    if (PreLRMSWaitingJobs > -1)          out << IString(" Pre-LRMS waiting jobs: %i", PreLRMSWaitingJobs) << std::endl;
    if (EstimatedAverageWaitingTime > -1) out << IString(" Estimated average waiting time: %s", EstimatedAverageWaitingTime.istr()) << std::endl;
    if (EstimatedWorstWaitingTime > -1)   out << IString(" Estimated worst waiting time: %s", EstimatedWorstWaitingTime.istr()) << std::endl;
    if (FreeSlots > -1)                   out << IString(" Free slots: %i", FreeSlots) << std::endl;
    if (!FreeSlotsWithDuration.empty()) {
                                             out << IString(" Free slots grouped according to time limits (limit: free slots):") << std::endl;
      for (std::map<Period, int>::const_iterator it = FreeSlotsWithDuration.begin();
           it != FreeSlotsWithDuration.end(); ++it) {
        if (it->first != Period(LONG_MAX))   out << IString("  %s: %i", it->first.istr(), it->second) << std::endl;
        else                                 out << IString("  unspecified: %i", it->second) << std::endl;
      }
    }
    if (UsedSlots > -1)                   out << IString(" Used slots: %i", UsedSlots) << std::endl;
    if (RequestedSlots > -1)              out << IString(" Requested slots: %i", RequestedSlots) << std::endl;
    if (!ReservationPolicy.empty())       out << IString(" Reservation policy: %s", ReservationPolicy) << std::endl;
  }

  void ComputingManagerAttributes::SaveToStream(std::ostream& out, bool alldetails) const {
    if (!alldetails) {
      if (!ProductName.empty()) {
        out << IString(" Resource manager: %s", ProductName);
        if (!ProductVersion.empty()) out << IString(" (%s)", ProductVersion);
        out << std::endl;
      }
      if (TotalPhysicalCPUs > -1)  out << IString(" Total physical CPUs: %i", TotalPhysicalCPUs) << std::endl;
      if (TotalLogicalCPUs > -1)   out << IString(" Total logical CPUs: %i", TotalLogicalCPUs) << std::endl;
      if (TotalSlots > -1)         out << IString(" Total slots: %i", TotalSlots) << std::endl;
      if (!Homogeneous) out << IString(" Non-homogeneous resource") << std::endl;
      return;
    }
    
    if (!ProductName.empty())    out << IString(" Resource manager: %s", ProductName) << std::endl;
    if (!ProductVersion.empty()) out << IString(" Resource manager version: %s", ProductVersion) << std::endl;
    if (Reservation)             out << IString(" Supports advance reservations") << std::endl;
    else                         out << IString(" Doesn't support advance reservations") << std::endl;
    if (BulkSubmission)          out << IString(" Supports bulk submission") << std::endl;
    else                         out << IString(" Doesn't support bulk Submission") << std::endl;
    if (TotalPhysicalCPUs > -1)  out << IString(" Total physical CPUs: %i", TotalPhysicalCPUs) << std::endl;
    if (TotalLogicalCPUs > -1)   out << IString(" Total logical CPUs: %i", TotalLogicalCPUs) << std::endl;
    if (TotalSlots > -1)         out << IString(" Total slots: %i", TotalSlots) << std::endl;
    if (Homogeneous)             out << IString(" Homogeneous resource") << std::endl;
    else                         out << IString(" Non-homogeneous resource") << std::endl;
    if (!NetworkInfo.empty()) {
                                    out << IString(" Network information:") << std::endl;
      for (std::list<std::string>::const_iterator it = NetworkInfo.begin();
           it != NetworkInfo.end(); it++)
        out << "  " << *it << std::endl;
    }
    if (WorkingAreaShared)        out << IString(" Working area is shared among jobs") << std::endl;
    else                          out << IString(" Working area is not shared among jobs") << std::endl;
    if (WorkingAreaTotal > -1)    out << IString(" Working area total size: %i GB", WorkingAreaTotal) << std::endl;
    if (WorkingAreaFree > -1)     out << IString(" Working area free size: %i GB", WorkingAreaFree) << std::endl;
    if (WorkingAreaLifeTime > -1) out << IString(" Working area life time: %s", WorkingAreaLifeTime.istr()) << std::endl;
    if (CacheTotal > -1)          out << IString(" Cache area total size: %i GB", CacheTotal) << std::endl;
    if (CacheFree > -1)           out << IString(" Cache area free size: %i GB", CacheFree) << std::endl;
  }
  
  std::ostream& operator<<(std::ostream& out, const ExecutionEnvironmentAttributes& ee) {
                                                  out << IString("Execution environment information:") << std::endl;
    if (!ee.Platform.empty())                     out << IString(" Platform: %s", ee.Platform) << std::endl;
    if (ee.ConnectivityIn)                        out << IString(" Execution environment supports inbound connections") << std::endl;
    else                                          out << IString(" Execution environment does not support inbound connections") << std::endl;
    if (ee.ConnectivityOut)                       out << IString(" Execution environment supports outbound connections") << std::endl;
    else                                          out << IString(" Execution environment does not support outbound connections") << std::endl;
    if (ee.VirtualMachine)                        out << IString(" Execution environment is a virtual machine") << std::endl;
    else                                          out << IString(" Execution environment is a physical machine") << std::endl;
    if (!ee.CPUVendor.empty())                    out << IString(" CPU vendor: %s", ee.CPUVendor) << std::endl;
    if (!ee.CPUModel.empty())                     out << IString(" CPU model: %s", ee.CPUModel) << std::endl;
    if (!ee.CPUVersion.empty())                   out << IString(" CPU version: %s", ee.CPUVersion) << std::endl;
    if (ee.CPUClockSpeed > -1)                    out << IString(" CPU clock speed: %i", ee.CPUClockSpeed) << std::endl;
    if (ee.MainMemorySize > -1)                   out << IString(" Main memory size: %i", ee.MainMemorySize) << std::endl;
    if (!ee.OperatingSystem.getFamily().empty())  out << IString(" OS family: %s", ee.OperatingSystem.getFamily()) << std::endl;
    if (!ee.OperatingSystem.getName().empty())    out << IString(" OS name: %s", ee.OperatingSystem.getName()) << std::endl;
    if (!ee.OperatingSystem.getVersion().empty()) out << IString(" OS version: %s", ee.OperatingSystem.getVersion()) << std::endl;
    return out;
  }

  void ComputingServiceType::SaveToStream(std::ostream& out, bool alldetails) const {
    out << IString("Computing resource:") << std::endl;
    out << **this;
    if ((**this).Cluster) out << IString(" Information endpoint: %s", (**this).Cluster.plainstr()) << std::endl;
    out << std::endl;
    
    if (alldetails) {
      out << std::endl << *Location;
      out << std::endl << *AdminDomain;
    }
    //~ if (!ComputingEndpoint.empty()) {
      //~ out << IString("Endpoint information:");
      //~ for (std::map<int, ComputingEndpointType>::const_iterator it = ComputingEndpoint.begin();
           //~ it != ComputingEndpoint.end(); ++it) {
        //~ out << std::endl;
        //~ it->second->SaveToStream(out, alldetails);
      //~ }
      //~ out << std::endl;
    //~ }
    if (!ComputingManager.empty()) {
      out << IString("Batch system information:");
      for (std::map<int, ComputingManagerType>::const_iterator it = ComputingManager.begin();
           it != ComputingManager.end(); ++it) {
        out << std::endl;
        it->second->SaveToStream(out, alldetails);
        if (!it->second.ApplicationEnvironments->empty()) {
          out << IString(" Installed application environments:") << std::endl;
          for (std::list<ApplicationEnvironment>::const_iterator itAE = it->second.ApplicationEnvironments->begin();
               itAE != it->second.ApplicationEnvironments->end(); ++itAE) {
            out << "  " << *itAE << std::endl;
          }
        }
      }
      out << std::endl;
    }
    
    if (!ComputingShare.empty()) {
      out << IString("Queue information:");
      for (std::map<int, ComputingShareType>::const_iterator it = ComputingShare.begin();
           it != ComputingShare.end(); ++it) {
        out << std::endl;
        it->second->SaveToStream(out, alldetails);
      }
      out << std::endl;
    }
  }


  void ExecutionTarget::SaveToStream(std::ostream& out, bool longlist) const {
    out << IString("Execution Target on Computing Service: %s", (!ComputingService->Name.empty() ? ComputingService->Name : ComputingService->Cluster.Host())) << std::endl;
    if (ComputingService->Cluster) {
      std::string formattedURL = ComputingService->Cluster.str();
      formattedURL.erase(std::remove(formattedURL.begin(), formattedURL.end(), ' '), formattedURL.end()); // Remove spaces.
      std::string::size_type pos = formattedURL.find("?"); // Do not output characters after the '?' character.
      out << IString(" Local information system URL: %s", formattedURL.substr(0, pos)) << std::endl;
    }
    if (!ComputingEndpoint->URLString.empty())
      out << IString(" Computing endpoint URL: %s", ComputingEndpoint->URLString) << std::endl;
    if (!ComputingEndpoint->InterfaceName.empty())
      out << IString(" Submission interface name: %s", ComputingEndpoint->InterfaceName) << std::endl;
    if (!ComputingShare->Name.empty()) {
       out << IString(" Queue: %s", ComputingShare->Name) << std::endl;
    }
    if (!ComputingShare->MappingQueue.empty()) {
       out << IString(" Mapping queue: %s", ComputingShare->MappingQueue) << std::endl;
    }
    if (!ComputingEndpoint->HealthState.empty()){
      out << IString(" Health state: %s", ComputingEndpoint->HealthState) << std::endl;
    }
    
    if (longlist) {
      out << std::endl << *Location;
      out << std::endl << *AdminDomain << std::endl;
      out << IString("Service information:") << std::endl << *ComputingService;
      out << std::endl;
      ComputingEndpoint->SaveToStream(out, longlist);

      if (!ApplicationEnvironments->empty()) {
        out << IString(" Installed application environments:") << std::endl;
        for (std::list<ApplicationEnvironment>::const_iterator it = ApplicationEnvironments->begin();
             it != ApplicationEnvironments->end(); ++it) {
          out << "  " << *it << std::endl;
        }
      }

      out << IString("Batch system information:");
      ComputingManager->SaveToStream(out, longlist);
      
      out << IString("Queue information:");
      ComputingShare->SaveToStream(out, longlist);
      
      out << std::endl << *ExecutionEnvironment;
      
      // Benchmarks
      if (!Benchmarks->empty()) {
        out << IString(" Benchmark information:") << std::endl;
        for (std::map<std::string, double>::const_iterator it =
               Benchmarks->begin(); it != Benchmarks->end(); ++it)
          out << "  " << it->first << ": " << it->second << std::endl;
      }

    } // end if long

    out << std::endl;

  } // end print

} // namespace Arc
