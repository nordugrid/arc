// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/ArcConfig.h>
#include <arc/Logger.h>
#include <arc/XMLNode.h>
#include <arc/compute/Broker.h>
#include <arc/compute/ExecutionTarget.h>
#include <arc/compute/Submitter.h>
#include <arc/UserConfig.h>
#include <arc/StringConv.h>

namespace Arc {

  Logger ExecutionTarget::logger(Logger::getRootLogger(), "ExecutionTarget");
  Logger ComputingServiceType::logger(Logger::getRootLogger(), "ComputingServiceType");

  template<typename T>
  void ComputingServiceType::GetExecutionTargets(T& container) const {
    // TODO: Currently assuming only one ComputingManager and one ExecutionEnvironment.
    CountedPointer<ComputingManagerAttributes> computingManager(
      ComputingManager.empty()?
        new ComputingManagerAttributes:
        ComputingManager.begin()->second.Attributes);
    CountedPointer<ExecutionEnvironmentAttributes> executionEnvironment(
      (ComputingManager.empty() || ComputingManager.begin()->second.ExecutionEnvironment.empty())?
        new ExecutionEnvironmentAttributes:
        ComputingManager.begin()->second.ExecutionEnvironment.begin()->second.Attributes);
    CountedPointer< std::map<std::string, double> > benchmarks(
      ComputingManager.empty()?
        new std::map<std::string, double>:
        ComputingManager.begin()->second.Benchmarks);
    CountedPointer< std::list<ApplicationEnvironment> > applicationEnvironments(
      ComputingManager.empty()?
        new std::list<ApplicationEnvironment>:
        ComputingManager.begin()->second.ApplicationEnvironments);

    for (std::map<int, ComputingEndpointType>::const_iterator itCE = ComputingEndpoint.begin();
         itCE != ComputingEndpoint.end(); ++itCE) {
      if (!itCE->second->Capability.count(Endpoint::GetStringForCapability(Endpoint::JOBSUBMIT)) &&
          !itCE->second->Capability.count(Endpoint::GetStringForCapability(Endpoint::JOBCREATION))) {
        continue;
      }
      if (!Attributes->InformationOriginEndpoint.RequestedSubmissionInterfaceName.empty()) {
        // If this endpoint has a non-preferred job interface, we skip it
        if (itCE->second->InterfaceName != Attributes->InformationOriginEndpoint.RequestedSubmissionInterfaceName) {
          logger.msg(VERBOSE,
            "Skipping ComputingEndpoint '%s', because it has '%s' interface instead of the requested '%s'.",
            itCE->second->URLString, itCE->second->InterfaceName, Attributes->InformationOriginEndpoint.RequestedSubmissionInterfaceName);
          continue;
        }
      }

      // Create list of other endpoints.
      std::list< CountedPointer<ComputingEndpointAttributes> > OtherEndpoints;
      for (std::map<int, ComputingEndpointType>::const_iterator itOE = ComputingEndpoint.begin();
           itOE != ComputingEndpoint.end(); ++itOE) {
        if (itOE == itCE) { // Dont include the current endpoint in the list of other endpoints.
          continue;
        }
        OtherEndpoints.push_back(itOE->second.Attributes);
      }

      if (!itCE->second.ComputingShareIDs.empty()) {
        for (std::set<int>::const_iterator itCSIDs = itCE->second.ComputingShareIDs.begin();
             itCSIDs != itCE->second.ComputingShareIDs.end(); ++itCSIDs) {
          std::map<int, ComputingShareType>::const_iterator itCS = ComputingShare.find(*itCSIDs);
          if (itCS != ComputingShare.end()) {
            // Create list of mapping policies
            std::list< CountedPointer<MappingPolicyAttributes> > MappingPolicies;
            for (std::map<int, MappingPolicyType>::const_iterator itMP = itCS->second.MappingPolicy.begin();
                 itMP != itCS->second.MappingPolicy.end(); ++itMP) {
              MappingPolicies.push_back(itMP->second.Attributes);
            }

            AddExecutionTarget<T>(container, ExecutionTarget(Location.Attributes, AdminDomain.Attributes,
                                                               Attributes, itCE->second.Attributes,
                                                               OtherEndpoints, itCS->second.Attributes,
                                                               MappingPolicies,
                                                               computingManager, executionEnvironment,
                                                               benchmarks, applicationEnvironments));
          }
        }
      }
      else if (!ComputingShare.empty()) {
        for (std::map<int, ComputingShareType>::const_iterator itCS = ComputingShare.begin();
             itCS != ComputingShare.end(); ++itCS) {
          // Create list of mapping policies
          std::list< CountedPointer<MappingPolicyAttributes> > MappingPolicies;
          for (std::map<int, MappingPolicyType>::const_iterator itMP = itCS->second.MappingPolicy.begin();
               itMP != itCS->second.MappingPolicy.end(); ++itMP) {
            MappingPolicies.push_back(itMP->second.Attributes);
          }
          AddExecutionTarget<T>(container, ExecutionTarget(Location.Attributes, AdminDomain.Attributes,
                                                             Attributes, itCE->second.Attributes,
                                                             OtherEndpoints, itCS->second.Attributes,
                                                             MappingPolicies,
                                                             computingManager, executionEnvironment,
                                                             benchmarks, applicationEnvironments));
        }
      } else {
        // No ComputingShares and no associations. Either it is not computing service
        // or it does not bother to specify its share or does not split resources
        // by shares.
        // Check if it is computing endpoint at all
        for (std::set<std::string>::const_iterator itCap = itCE->second.Attributes->Capability.begin();
                    itCap != itCE->second.Attributes->Capability.end(); ++itCap) {
          if((*itCap == "executionmanagement.jobcreation") ||
             (*itCap == "executionmanagement.jobexecution")) {
            // Creating generic target
            CountedPointer<ComputingShareAttributes> computingShare(new ComputingShareAttributes);
            std::list< CountedPointer<MappingPolicyAttributes> > MappingPolicies;
            AddExecutionTarget<T>(container,
                                  ExecutionTarget(Location.Attributes, AdminDomain.Attributes,
                                                  Attributes, itCE->second.Attributes,
                                                  OtherEndpoints,
                                                  computingShare,
                                                  MappingPolicies,
                                                  computingManager,
                                                  executionEnvironment,
                                                  benchmarks,
                                                  applicationEnvironments));
            break;
          }
        }


      }
      logger.msg(INFO, "Computing endpoint %s (type %s) added to the list for submission brokering",
                 itCE->second->URLString, itCE->second->InterfaceName);
    }
  }

  template void ComputingServiceType::GetExecutionTargets< std::list<ExecutionTarget> >(std::list<ExecutionTarget>&) const;

  template<typename T>
  void ComputingServiceType::AddExecutionTarget(T&, const ExecutionTarget&) const {}

  template<>
  void ComputingServiceType::AddExecutionTarget< std::list<ExecutionTarget> >(std::list<ExecutionTarget>& etList, const ExecutionTarget& et) const {
    etList.push_back(et);
  }

  SubmissionStatus ExecutionTarget::Submit(const UserConfig& ucfg, const JobDescription& jobdesc, Job& job) const {
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

  class Indenter : public std::streambuf {
  private:
    std::streambuf* dest;
    bool isAtStartOfLine;
    std::string indent;
    std::ostream* owner;

  protected:
    virtual int overflow(int ch) {
      if (isAtStartOfLine && ch != '\n') dest->sputn(indent.data(), indent.size());
      isAtStartOfLine = (ch == '\n');
      return dest->sputc(ch);
    }

  public:
    explicit Indenter(std::streambuf* dest, unsigned indentSize = 2)
      : dest(dest), isAtStartOfLine(true), indent(indentSize, ' '), owner(NULL) {}

    explicit Indenter(std::ostringstream& dest, unsigned indentSize = 2)
      : dest(dest.rdbuf()), isAtStartOfLine(true), indent(indentSize, ' '), owner(&dest) { owner->rdbuf(this); }

    explicit Indenter(std::ostream& dest, unsigned indentSize = 2)
      : dest(dest.rdbuf()), isAtStartOfLine(true), indent(indentSize, ' '), owner(&dest) { owner->rdbuf(this); }

    virtual ~Indenter() { if (owner != NULL) owner->rdbuf(dest); }
  };

  std::ostream& operator<<(std::ostream& out, const LocationAttributes& l) {
    if (!l.Address.empty())  out << IString("Address: %s", l.Address) << std::endl;
    if (!l.Place.empty())    out << IString("Place: %s", l.Place) << std::endl;
    if (!l.Country.empty())  out << IString("Country: %s", l.Country) << std::endl;
    if (!l.PostCode.empty()) out << IString("Postal code: %s", l.PostCode) << std::endl;
    if (l.Latitude > 0)      out << IString("Latitude: %f", l.Latitude) << std::endl;
    if (l.Longitude > 0)     out << IString("Longitude: %f", l.Longitude) << std::endl;
    return out;
  }

  std::ostream& operator<<(std::ostream& out, const AdminDomainAttributes& ad) {
    if (!ad.Owner.empty()) {
      out << IString("Owner: %s", ad.Owner) << std::endl;
    }
    return out;
  }

  std::ostream& operator<<(std::ostream& out, const ComputingServiceAttributes& cs) {
    if (!cs.Name.empty()) out << IString("Name: %s", cs.Name) << '\n';
    if (!cs.ID.empty())   out << IString("ID: %s", cs.ID) << '\n';
    if (!cs.Type.empty()) out << IString("Type: %s", cs.Type) << '\n';
    return out;
  }

  std::ostream& operator<<(std::ostream& out, const ComputingEndpointAttributes& ce) {
    if (!ce.URLString.empty())        out << IString("URL: %s", ce.URLString) << std::endl;
    if (!ce.InterfaceName.empty())    out << IString("Interface: %s", ce.InterfaceName) << std::endl;
    if (!ce.InterfaceVersion.empty()) {
                                      out << IString("Interface versions:") << std::endl;
      for (std::list<std::string>::const_iterator it = ce.InterfaceVersion.begin();
           it != ce.InterfaceVersion.end(); ++it) out << "  " << *it << std::endl;
    }
    if (!ce.InterfaceExtension.empty()) {
                                      out << IString("Interface extensions:") << std::endl;
      for (std::list<std::string>::const_iterator it = ce.InterfaceExtension.begin();
           it != ce.InterfaceExtension.end(); ++it) out << "  " << *it << std::endl;
    }
    if (!ce.Capability.empty()) {
                                      out << IString("Capabilities:") << std::endl;
      for (std::set<std::string>::const_iterator it = ce.Capability.begin();
           it != ce.Capability.end(); ++it) out << "  " << *it << std::endl;
    }
    if (!ce.Technology.empty())       out << IString("Technology: %s", ce.Technology) << std::endl;
    if (!ce.SupportedProfile.empty()) {
                                      out << IString("Supported Profiles:") << std::endl;
      for (std::list<std::string>::const_iterator it = ce.SupportedProfile.begin();
           it != ce.SupportedProfile.end(); ++it) out << "  " << *it << std::endl;
    }
    if (!ce.Implementor.empty())      out << IString("Implementor: %s", ce.Implementor) << std::endl;
    if (!ce.Implementation().empty()) out << IString("Implementation name: %s", (std::string)ce.Implementation) << std::endl;
    if (!ce.QualityLevel.empty())     out << IString("Quality level: %s", ce.QualityLevel) << std::endl;
    if (!ce.HealthState.empty())      out << IString("Health state: %s", ce.HealthState) << std::endl;
    if (!ce.HealthStateInfo.empty())  out << IString("Health state info: %s", ce.HealthStateInfo) << std::endl;
    if (!ce.ServingState.empty())     out << IString("Serving state: %s", ce.ServingState) << std::endl;
    if (!ce.IssuerCA.empty())         out << IString("Issuer CA: %s", ce.IssuerCA) << std::endl;
    if (!ce.TrustedCA.empty()) {
                                      out << IString("Trusted CAs:") << std::endl;
      for (std::list<std::string>::const_iterator it = ce.TrustedCA.begin();
           it != ce.TrustedCA.end(); ++it) out << "  " << *it << std::endl;
    }
    if (ce.DowntimeStarts > -1)       out << IString("Downtime starts: %s", ce.DowntimeStarts.str())<< std::endl;
    if (ce.DowntimeEnds > -1)         out << IString("Downtime ends: %s", ce.DowntimeEnds.str()) << std::endl;
    if (!ce.Staging.empty())          out << IString("Staging: %s", ce.Staging) << std::endl;
    if (!ce.JobDescriptions.empty()) {
                                      out << IString("Job descriptions:") << std::endl;
      for (std::list<std::string>::const_iterator it = ce.JobDescriptions.begin();
           it != ce.JobDescriptions.end(); ++it) out << "  " << *it << std::endl;
    }

    return out;
  }

  std::ostream& operator<<(std::ostream& out, const MappingPolicyAttributes& mp) {
    // ID not printed.

    std::string scheme = mp.Scheme.empty() ? "basic" : mp.Scheme;
    out << IString("Scheme: %s", scheme) << std::endl;
    for (std::list<std::string>::const_iterator itR = mp.Rule.begin();
         itR != mp.Rule.end(); ++itR) {
      out << IString("Rule: %s", *itR) << std::endl;
    }
    return out;
  }

  std::ostream& operator<<(std::ostream& out, const ComputingShareAttributes& cs) {
    // Following attributes are not printed:
    // Period MaxTotalCPUTime;
    // Period MaxTotalWallTime; // not in current Glue2 draft
    // std::string ID;

    if (!cs.Name.empty())                    out << IString("Name: %s", cs.Name) << std::endl;
    if (!cs.MappingQueue.empty())            out << IString("Mapping queue: %s", cs.MappingQueue) << std::endl;
    if (cs.MaxWallTime > -1)                 out << IString("Max wall-time: %s", cs.MaxWallTime.istr()) << std::endl;
    if (cs.MaxTotalWallTime > -1)            out << IString("Max total wall-time: %s", cs.MaxTotalWallTime.istr()) << std::endl;
    if (cs.MinWallTime > -1)                 out << IString("Min wall-time: %s", cs.MinWallTime.istr()) << std::endl;
    if (cs.DefaultWallTime > -1)             out << IString("Default wall-time: %s", cs.DefaultWallTime.istr()) << std::endl;
    if (cs.MaxCPUTime > -1)                  out << IString("Max CPU time: %s", cs.MaxCPUTime.istr()) << std::endl;
    if (cs.MinCPUTime > -1)                  out << IString("Min CPU time: %s", cs.MinCPUTime.istr()) << std::endl;
    if (cs.DefaultCPUTime > -1)              out << IString("Default CPU time: %s", cs.DefaultCPUTime.istr()) << std::endl;
    if (cs.MaxTotalJobs > -1)                out << IString("Max total jobs: %i", cs.MaxTotalJobs) << std::endl;
    if (cs.MaxRunningJobs > -1)              out << IString("Max running jobs: %i", cs.MaxRunningJobs) << std::endl;
    if (cs.MaxWaitingJobs > -1)              out << IString("Max waiting jobs: %i", cs.MaxWaitingJobs) << std::endl;
    if (cs.MaxPreLRMSWaitingJobs > -1)       out << IString("Max pre-LRMS waiting jobs: %i", cs.MaxPreLRMSWaitingJobs) << std::endl;
    if (cs.MaxUserRunningJobs > -1)          out << IString("Max user running jobs: %i", cs.MaxUserRunningJobs) << std::endl;
    if (cs.MaxSlotsPerJob > -1)              out << IString("Max slots per job: %i", cs.MaxSlotsPerJob) << std::endl;
    if (cs.MaxStageInStreams > -1)           out << IString("Max stage in streams: %i", cs.MaxStageInStreams) << std::endl;
    if (cs.MaxStageOutStreams > -1)          out << IString("Max stage out streams: %i", cs.MaxStageOutStreams) << std::endl;
    if (!cs.SchedulingPolicy.empty())        out << IString("Scheduling policy: %s", cs.SchedulingPolicy) << std::endl;
    if (cs.MaxMainMemory > -1)               out << IString("Max memory: %i", cs.MaxMainMemory) << std::endl;
    if (cs.MaxVirtualMemory > -1)            out << IString("Max virtual memory: %i", cs.MaxVirtualMemory) << std::endl;
    if (cs.MaxDiskSpace > -1)                out << IString("Max disk space: %i", cs.MaxDiskSpace) << std::endl;
    if (cs.DefaultStorageService)            out << IString("Default Storage Service: %s", cs.DefaultStorageService.str()) << std::endl;
    if (cs.Preemption)                       out << IString("Supports preemption") << std::endl;
    else                                     out << IString("Doesn't support preemption") << std::endl;
    if (cs.TotalJobs > -1)                   out << IString("Total jobs: %i", cs.TotalJobs) << std::endl;
    if (cs.RunningJobs > -1)                 out << IString("Running jobs: %i", cs.RunningJobs) << std::endl;
    if (cs.LocalRunningJobs > -1)            out << IString("Local running jobs: %i", cs.LocalRunningJobs) << std::endl;
    if (cs.WaitingJobs > -1)                 out << IString("Waiting jobs: %i", cs.WaitingJobs) << std::endl;
    if (cs.LocalWaitingJobs > -1)            out << IString("Local waiting jobs: %i", cs.LocalWaitingJobs) << std::endl;
    if (cs.SuspendedJobs > -1)               out << IString("Suspended jobs: %i", cs.SuspendedJobs) << std::endl;
    if (cs.LocalSuspendedJobs > -1)          out << IString("Local suspended jobs: %i", cs.LocalSuspendedJobs) << std::endl;
    if (cs.StagingJobs > -1)                 out << IString("Staging jobs: %i", cs.StagingJobs) << std::endl;
    if (cs.PreLRMSWaitingJobs > -1)          out << IString("Pre-LRMS waiting jobs: %i", cs.PreLRMSWaitingJobs) << std::endl;
    if (cs.EstimatedAverageWaitingTime > -1) out << IString("Estimated average waiting time: %s", cs.EstimatedAverageWaitingTime.istr()) << std::endl;
    if (cs.EstimatedWorstWaitingTime > -1)   out << IString("Estimated worst waiting time: %s", cs.EstimatedWorstWaitingTime.istr()) << std::endl;
    if (cs.FreeSlots > -1)                   out << IString("Free slots: %i", cs.FreeSlots) << std::endl;
    if (!cs.FreeSlotsWithDuration.empty()) {
                                             out << IString("Free slots grouped according to time limits (limit: free slots):") << std::endl;
      for (std::map<Period, int>::const_iterator it = cs.FreeSlotsWithDuration.begin();
           it != cs.FreeSlotsWithDuration.end(); ++it) {
        if (it->first != Period(LONG_MAX))   out << IString("  %s: %i", it->first.istr(), it->second) << std::endl;
        else                                 out << IString("  unspecified: %i", it->second) << std::endl;
      }
    }
    if (cs.UsedSlots > -1)                   out << IString("Used slots: %i", cs.UsedSlots) << std::endl;
    if (cs.RequestedSlots > -1)              out << IString("Requested slots: %i", cs.RequestedSlots) << std::endl;
    if (!cs.ReservationPolicy.empty())       out << IString("Reservation policy: %s", cs.ReservationPolicy) << std::endl;

    return out;
  }

  std::ostream& operator<<(std::ostream& out, const ComputingManagerAttributes& cm) {
    if (!cm.ProductName.empty()) {
                                      out << IString("Resource manager: %s", cm.ProductName);
      if (!cm.ProductVersion.empty()) {
                                      out << IString(" (%s)", cm.ProductVersion);
      }
                                      out << std::endl;
    }
    if (cm.TotalPhysicalCPUs > -1)    out << IString("Total physical CPUs: %i", cm.TotalPhysicalCPUs) << std::endl;
    if (cm.TotalLogicalCPUs > -1)     out << IString("Total logical CPUs: %i", cm.TotalLogicalCPUs) << std::endl;
    if (cm.TotalSlots > -1)           out << IString("Total slots: %i", cm.TotalSlots) << std::endl;
    if (cm.Reservation)               out << IString("Supports advance reservations") << std::endl;
    else                              out << IString("Doesn't support advance reservations") << std::endl;
    if (cm.BulkSubmission)            out << IString("Supports bulk submission") << std::endl;
    else                              out << IString("Doesn't support bulk Submission") << std::endl;
    if (cm.Homogeneous)               out << IString("Homogeneous resource") << std::endl;
    else                              out << IString("Non-homogeneous resource") << std::endl;
    if (!cm.NetworkInfo.empty()) {
                                      out << IString("Network information:") << std::endl;
      for (std::list<std::string>::const_iterator it = cm.NetworkInfo.begin();
           it != cm.NetworkInfo.end(); ++it)
        out << "  " << *it << std::endl;
    }
    if (cm.WorkingAreaShared)         out << IString("Working area is shared among jobs") << std::endl;
    else                              out << IString("Working area is not shared among jobs") << std::endl;
    if (cm.WorkingAreaTotal > -1)     out << IString("Working area total size: %i GB", cm.WorkingAreaTotal) << std::endl;
    if (cm.WorkingAreaFree > -1)      out << IString("Working area free size: %i GB", cm.WorkingAreaFree) << std::endl;
    if (cm.WorkingAreaLifeTime > -1)  out << IString("Working area life time: %s", cm.WorkingAreaLifeTime.istr()) << std::endl;
    if (cm.CacheTotal > -1)           out << IString("Cache area total size: %i GB", cm.CacheTotal) << std::endl;
    if (cm.CacheFree > -1)            out << IString("Cache area free size: %i GB", cm.CacheFree) << std::endl;

    return out;
  }

  std::ostream& operator<<(std::ostream& out, const ExecutionEnvironmentAttributes& ee) {
    if (!ee.Platform.empty())                     out << IString("Platform: %s", ee.Platform) << std::endl;
    if (ee.ConnectivityIn)                        out << IString("Execution environment supports inbound connections") << std::endl;
    else                                          out << IString("Execution environment does not support inbound connections") << std::endl;
    if (ee.ConnectivityOut)                       out << IString("Execution environment supports outbound connections") << std::endl;
    else                                          out << IString("Execution environment does not support outbound connections") << std::endl;
    if (ee.VirtualMachine)                        out << IString("Execution environment is a virtual machine") << std::endl;
    else                                          out << IString("Execution environment is a physical machine") << std::endl;
    if (!ee.CPUVendor.empty())                    out << IString("CPU vendor: %s", ee.CPUVendor) << std::endl;
    if (!ee.CPUModel.empty())                     out << IString("CPU model: %s", ee.CPUModel) << std::endl;
    if (!ee.CPUVersion.empty())                   out << IString("CPU version: %s", ee.CPUVersion) << std::endl;
    if (ee.CPUClockSpeed > -1)                    out << IString("CPU clock speed: %i", ee.CPUClockSpeed) << std::endl;
    if (ee.MainMemorySize > -1)                   out << IString("Main memory size: %i", ee.MainMemorySize) << std::endl;
    if (!ee.OperatingSystem.getFamily().empty())  out << IString("OS family: %s", ee.OperatingSystem.getFamily()) << std::endl;
    if (!ee.OperatingSystem.getName().empty())    out << IString("OS name: %s", ee.OperatingSystem.getName()) << std::endl;
    if (!ee.OperatingSystem.getVersion().empty()) out << IString("OS version: %s", ee.OperatingSystem.getVersion()) << std::endl;
    return out;
  }

  std::ostream& operator<<(std::ostream& out, const ComputingServiceType& cst) {
    std::ostringstream buffer;

    out << IString("Computing service:") << std::endl;

    Indenter iOut(out);
    buffer << *cst;
    if (buffer.tellp() > 0) {
      out << buffer.str();
    }
    buffer.str("");

    buffer << *cst.Location;
    if (buffer.tellp() > 0) {
      out << buffer.str();
    }
    buffer.str("");

    buffer << *cst.AdminDomain;
    if (buffer.tellp() > 0) {
      out << buffer.str();
    }
    buffer.str("");

    if (!cst.ComputingEndpoint.empty()) {
      out << std::endl;
      if (cst.ComputingEndpoint.size() > 1) {
        out << IString("%d Endpoints", cst.ComputingEndpoint.size()) << std::endl;
      }
      for (std::map<int, ComputingEndpointType>::const_iterator it = cst.ComputingEndpoint.begin();
           it != cst.ComputingEndpoint.end(); ++it) {
        if (it != cst.ComputingEndpoint.begin()) out << std::endl;
        out << IString("Endpoint Information:") << std::endl;

        std::ostringstream endpointBuffer;
        Indenter iEndpoint(endpointBuffer);
        endpointBuffer << (*it->second);
        out << endpointBuffer.str();
      }
    }

    if (!cst.ComputingManager.empty()) {
      out << std::endl;
      if (cst.ComputingManager.size() > 1) {
        out << IString("%d Batch Systems", cst.ComputingManager.size()) << std::endl;
      }
      for (std::map<int, ComputingManagerType>::const_iterator it = cst.ComputingManager.begin();
           it != cst.ComputingManager.end(); ++it) {
        if (it != cst.ComputingManager.begin()) out << std::endl;
        out << IString("Batch System Information:") << std::endl;

        std::ostringstream managerBuffer;
        Indenter iManager(managerBuffer);
        managerBuffer << (*it->second);
        if (!it->second.ApplicationEnvironments->empty()) {
          managerBuffer << IString("Installed application environments:") << std::endl;
          for (std::list<ApplicationEnvironment>::const_iterator itAE = it->second.ApplicationEnvironments->begin();
               itAE != it->second.ApplicationEnvironments->end(); ++itAE) {
            managerBuffer << "  " << *itAE << std::endl;
          }
        }
        out << managerBuffer.str();
      }
    }

    if (!cst.ComputingShare.empty()) {
      out << std::endl;
      if (cst.ComputingShare.size() > 1) {
        out << IString("%d Shares", cst.ComputingShare.size()) << std::endl;
      }
      for (std::map<int, ComputingShareType>::const_iterator itCH = cst.ComputingShare.begin();
           itCH != cst.ComputingShare.end(); ++itCH) {
        if (itCH != cst.ComputingShare.begin()) out << std::endl;
        out << IString("Share Information:") << std::endl;
        std::ostringstream queueBuffer;
        Indenter iQueue(queueBuffer);
        queueBuffer << (*itCH->second);

        if (itCH->second.MappingPolicy.size() > 1) {
          queueBuffer << IString("%d mapping policies", itCH->second.MappingPolicy.size());
        }
        for (std::map<int, MappingPolicyType>::const_iterator itMP = itCH->second.MappingPolicy.begin();
             itMP != itCH->second.MappingPolicy.end(); ++itMP) {
          queueBuffer << IString("Mapping policy:") << std::endl;
          std::ostringstream mpBuffer;
          Indenter indMP(mpBuffer);
          mpBuffer << (*itMP->second);
          queueBuffer << mpBuffer.str();
        }

        out << queueBuffer.str();
      }
    }

    return out;
  }


  std::ostream& operator<<(std::ostream& out, const ExecutionTarget& et) {
    out << IString("Execution Target on Computing Service: %s", et.ComputingService->Name) << std::endl;
    if (!et.ComputingEndpoint->URLString.empty())
      out << IString(" Computing endpoint URL: %s", et.ComputingEndpoint->URLString) << std::endl;
    if (!et.ComputingEndpoint->InterfaceName.empty())
      out << IString(" Computing endpoint interface name: %s", et.ComputingEndpoint->InterfaceName) << std::endl;
    if (!et.ComputingShare->Name.empty()) {
       out << IString(" Queue: %s", et.ComputingShare->Name) << std::endl;
    }
    if (!et.ComputingShare->MappingQueue.empty()) {
       out << IString(" Mapping queue: %s", et.ComputingShare->MappingQueue) << std::endl;
    }
    if (!et.ComputingEndpoint->HealthState.empty()){
      out << IString(" Health state: %s", et.ComputingEndpoint->HealthState) << std::endl;
    }

    out << std::endl << *et.Location;
    out << std::endl << *et.AdminDomain << std::endl;
    out << IString("Service information:") << std::endl << *et.ComputingService;
    out << std::endl;
    out << *et.ComputingEndpoint;

    if (!et.ApplicationEnvironments->empty()) {
      out << IString(" Installed application environments:") << std::endl;
      for (std::list<ApplicationEnvironment>::const_iterator it = et.ApplicationEnvironments->begin();
           it != et.ApplicationEnvironments->end(); ++it) {
        out << "  " << *it << std::endl;
      }
    }

    out << IString("Batch system information:");
    out << *et.ComputingManager;

    out << IString("Queue information:");
    out << *et.ComputingShare;

    out << std::endl << *et.ExecutionEnvironment;

    // Benchmarks
    if (!et.Benchmarks->empty()) {
      out << IString(" Benchmark information:") << std::endl;
      for (std::map<std::string, double>::const_iterator it = et.Benchmarks->begin();
           it != et.Benchmarks->end(); ++it)
        out << "  " << it->first << ": " << it->second << std::endl;
    }

    out << std::endl;

    return out;
  }

} // namespace Arc
