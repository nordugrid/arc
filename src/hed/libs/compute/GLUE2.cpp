// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/StringConv.h>

#include "GLUE2.h"

namespace Arc {

  Logger GLUE2::logger(Logger::getRootLogger(), "GLUE2");

  static bool CheckConformingDN(const std::string& dn) {
    // /name=value/name/...
    // Implementing very simplified check
    std::string::size_type pos = 0;
    if((dn[pos] != '/') && (dn.find(',') == std::string::npos)) return false;
    //while(pos < dn.length()) {
    //  std::string::size_type ppos = dn.find('/',pos+1);
    //  if(ppos == std::string::npos) ppos = dn.length();
    //  std::string item = dn.substr(pso+1,ppos-pos-1);
    //  std::string::size_type epos = item.find('=');
    //}
    return true;
  }

  void GLUE2::ParseExecutionTargets(XMLNode glue2tree, std::list<ComputingServiceType>& targets) {

    XMLNode GLUEService = glue2tree;
    if(GLUEService.Name() != "ComputingService") {
      GLUEService = glue2tree["ComputingService"];
    }

    for (; GLUEService; ++GLUEService) {
      ComputingServiceType cs;

      if (GLUEService["ID"]) {
        cs->ID = (std::string)GLUEService["ID"];
      }
      if (GLUEService["Name"]) {
        cs->Name = (std::string)GLUEService["Name"];
      }
      if (GLUEService["Capability"]) {
        for (XMLNode n = GLUEService["Capability"]; n; ++n) {
          cs->Capability.insert((std::string)n);
        }
      }
      if (GLUEService["Type"]) {
        cs->Type = (std::string)GLUEService["Type"];
      } else {
        logger.msg(VERBOSE, "The Service doesn't advertise its Type.");
      }
      if (GLUEService["QualityLevel"]) {
        cs->QualityLevel = (std::string)GLUEService["QualityLevel"];
      } else {
        logger.msg(VERBOSE, "The ComputingService doesn't advertise its Quality Level.");
      }
      if (GLUEService["TotalJobs"]) {
        cs->TotalJobs = stringtoi((std::string)GLUEService["TotalJobs"]);
      }
      if (GLUEService["RunningJobs"]) {
        cs->RunningJobs = stringtoi((std::string)GLUEService["RunningJobs"]);
      }
      if (GLUEService["WaitingJobs"]) {
        cs->WaitingJobs = stringtoi((std::string)GLUEService["WaitingJobs"]);
      }
      if (GLUEService["StagingJobs"]) {
        cs->StagingJobs = stringtoi((std::string)GLUEService["StagingJobs"]);
      }
      if (GLUEService["SuspendedJobs"]) {
        cs->SuspendedJobs = stringtoi((std::string)GLUEService["SuspendedJobs"]);
      }
      if (GLUEService["PreLRMSWaitingJobs"]) {
        cs->PreLRMSWaitingJobs = stringtoi((std::string)GLUEService["PreLRMSWaitingJobs"]);
      }
      // The GLUE2 specification does not have attribute ComputingService.LocalRunningJobs
      //if (GLUEService["LocalRunningJobs"]) {
      //  cs->LocalRunningJobs = stringtoi((std::string)GLUEService["LocalRunningJobs"]);
      //}
      // The GLUE2 specification does not have attribute ComputingService.LocalWaitingJobs
      //if (GLUEService["LocalWaitingJobs"]) {
      //  cs->LocalWaitingJobs = stringtoi((std::string)GLUEService["LocalWaitingJobs"]);
      //}
      // The GLUE2 specification does not have attribute ComputingService.LocalSuspendedJobs
      //if (GLUEService["LocalSuspendedJobs"]) {
      //  cs->LocalWaitingJobs = stringtoi((std::string)GLUEService["LocalSuspendedJobs"]);
      //}


      XMLNode xmlCENode = GLUEService["ComputingEndpoint"];
      int endpointID = 0;
      for(;(bool)xmlCENode;++xmlCENode) {
        ComputingEndpointType ComputingEndpoint;
        if (xmlCENode["URL"]) {
          ComputingEndpoint->URLString = (std::string)xmlCENode["URL"];
        } else {
          logger.msg(VERBOSE, "The ComputingEndpoint has no URL.");
        }
        if (xmlCENode["HealthState"]) {
          ComputingEndpoint->HealthState = (std::string)xmlCENode["HealthState"];
        } else {
          logger.msg(VERBOSE, "The Service advertises no Health State.");
        }
        if (xmlCENode["HealthStateInfo"]) {
          ComputingEndpoint->HealthStateInfo = (std::string)xmlCENode["HealthStateInfo"];
        }
        if (xmlCENode["Capability"]) {
          for (XMLNode n = xmlCENode["Capability"]; n; ++n) {
            ComputingEndpoint->Capability.insert((std::string)n);
          }
        }
        if (xmlCENode["QualityLevel"]) {
          ComputingEndpoint->QualityLevel = (std::string)xmlCENode["QualityLevel"];
        } else {
          logger.msg(VERBOSE, "The ComputingEndpoint doesn't advertise its Quality Level.");
        }

        if (xmlCENode["Technology"]) {
          ComputingEndpoint->Technology = (std::string)xmlCENode["Technology"];
        }
        if (xmlCENode["InterfaceName"]) {
          ComputingEndpoint->InterfaceName = lower((std::string)xmlCENode["InterfaceName"]);
        } else if (xmlCENode["Interface"]) { // No such attribute according to GLUE2 document. Legacy/backward compatibility?
          ComputingEndpoint->InterfaceName = lower((std::string)xmlCENode["Interface"]);
        } else {
          logger.msg(VERBOSE, "The ComputingService doesn't advertise its Interface.");
        }
        if (xmlCENode["InterfaceVersion"]) {
          for (XMLNode n = xmlCENode["InterfaceVersion"]; n; ++n) {
            ComputingEndpoint->InterfaceVersion.push_back((std::string)n);
          }
        }
        if (xmlCENode["InterfaceExtension"]) {
          for (XMLNode n = xmlCENode["InterfaceExtension"]; n; ++n) {
            ComputingEndpoint->InterfaceExtension.push_back((std::string)n);
          }
        }
        if (xmlCENode["SupportedProfile"]) {
          for (XMLNode n = xmlCENode["SupportedProfile"]; n; ++n) {
            ComputingEndpoint->SupportedProfile.push_back((std::string)n);
          }
        }
        if (xmlCENode["Implementor"]) {
          ComputingEndpoint->Implementor = (std::string)xmlCENode["Implementor"];
        }
        if (xmlCENode["ImplementationName"]) {
          if (xmlCENode["ImplementationVersion"]) {
            ComputingEndpoint->Implementation =
              Software((std::string)xmlCENode["ImplementationName"],
                       (std::string)xmlCENode["ImplementationVersion"]);
          } else {
            ComputingEndpoint->Implementation = Software((std::string)xmlCENode["ImplementationName"]);
          }
        }
        if (xmlCENode["ServingState"]) {
          ComputingEndpoint->ServingState = (std::string)xmlCENode["ServingState"];
        } else {
          logger.msg(VERBOSE, "The ComputingEndpoint doesn't advertise its Serving State.");
        }
        if (xmlCENode["IssuerCA"]) {
          ComputingEndpoint->IssuerCA = (std::string)xmlCENode["IssuerCA"];
        }
        if (xmlCENode["TrustedCA"]) {
          XMLNode n = xmlCENode["TrustedCA"];
          while (n) {
            // Workaround to drop non-conforming records generated by EGI services
            std::string subject = (std::string)n;
            if(CheckConformingDN(subject)) {
              ComputingEndpoint->TrustedCA.push_back(subject);
            }
            ++n; //The increment operator works in an unusual manner (returns void)
          }
        }
        if (xmlCENode["DowntimeStart"]) {
          ComputingEndpoint->DowntimeStarts = (std::string)xmlCENode["DowntimeStart"];
        }
        if (xmlCENode["DowntimeEnd"]) {
          ComputingEndpoint->DowntimeEnds = (std::string)xmlCENode["DowntimeEnd"];
        }
        if (xmlCENode["Staging"]) {
          ComputingEndpoint->Staging = (std::string)xmlCENode["Staging"];
        }
        if (xmlCENode["JobDescription"]) {
          for (XMLNode n = xmlCENode["JobDescription"]; n; ++n) {
            ComputingEndpoint->JobDescriptions.push_back((std::string)n);
          }
        }

        if (xmlCENode["TotalJobs"]) {
          ComputingEndpoint->TotalJobs = stringtoi((std::string)xmlCENode["TotalJobs"]);
        }
        if (xmlCENode["RunningJobs"]) {
          ComputingEndpoint->RunningJobs = stringtoi((std::string)xmlCENode["RunningJobs"]);
        }
        if (xmlCENode["WaitingJobs"]) {
          ComputingEndpoint->WaitingJobs = stringtoi((std::string)xmlCENode["WaitingJobs"]);
        }
        if (xmlCENode["StagingJobs"]) {
          ComputingEndpoint->StagingJobs = stringtoi((std::string)xmlCENode["StagingJobs"]);
        }
        if (xmlCENode["SuspendedJobs"]) {
          ComputingEndpoint->SuspendedJobs = stringtoi((std::string)xmlCENode["SuspendedJobs"]);
        }
        if (xmlCENode["PreLRMSWaitingJobs"]) {
          ComputingEndpoint->PreLRMSWaitingJobs = stringtoi((std::string)xmlCENode["PreLRMSWaitingJobs"]);
        }
        // The GLUE2 specification does not have attribute ComputingEndpoint.LocalRunningJobs
        //if (xmlCENode["LocalRunningJobs"]) {
        //  ComputingEndpoint->LocalRunningJobs = stringtoi((std::string)xmlCENode["LocalRunningJobs"]);
        //}
        // The GLUE2 specification does not have attribute ComputingEndpoint.LocalWaitingJobs
        //if (xmlCENode["LocalWaitingJobs"]) {
        //  ComputingEndpoint->LocalWaitingJobs = stringtoi((std::string)xmlCENode["LocalWaitingJobs"]);
        //}
        // The GLUE2 specification does not have attribute ComputingEndpoint.LocalSuspendedJobs
        //if (xmlCENode["LocalSuspendedJobs"]) {
        //  ComputingEndpoint->LocalSuspendedJobs = stringtoi((std::string)xmlCENode["LocalSuspendedJobs"]);
        //}

        cs.ComputingEndpoint.insert(std::pair<int, ComputingEndpointType>(endpointID++, ComputingEndpoint));
      }

      XMLNode xComputingShare = GLUEService["ComputingShare"];
      int shareID = 0;
      for (;(bool)xComputingShare;++xComputingShare) {
        ComputingShareType ComputingShare;

        if (xComputingShare["FreeSlots"]) {
          ComputingShare->FreeSlots = stringtoi((std::string)xComputingShare["FreeSlots"]);
        }
        if (xComputingShare["FreeSlotsWithDuration"]) {
          // Format: ns[:t] [ns[:t]]..., where ns is number of slots and t is the duration.
          ComputingShare->FreeSlotsWithDuration.clear();

          const std::string fswdValue = (std::string)xComputingShare["FreeSlotsWithDuration"];
          std::list<std::string> fswdList;
          tokenize(fswdValue, fswdList);
          for (std::list<std::string>::iterator it = fswdList.begin();
               it != fswdList.end(); it++) {
            std::list<std::string> fswdPair;
            tokenize(*it, fswdPair, ":");
            long duration = LONG_MAX;
            int freeSlots = 0;
            if (fswdPair.size() > 2 || !stringto(fswdPair.front(), freeSlots) || (fswdPair.size() == 2 && !stringto(fswdPair.back(), duration)) ) {
              logger.msg(VERBOSE, "The \"FreeSlotsWithDuration\" attribute published by \"%s\" is wrongly formatted. Ignoring it.");
              logger.msg(DEBUG, "Wrong format of the \"FreeSlotsWithDuration\" = \"%s\" (\"%s\")", fswdValue, *it);
              continue;
            }

            ComputingShare->FreeSlotsWithDuration[Period(duration)] = freeSlots;
          }
        }
        if (xComputingShare["UsedSlots"]) {
          ComputingShare->UsedSlots = stringtoi((std::string)xComputingShare["UsedSlots"]);
        }
        if (xComputingShare["RequestedSlots"]) {
          ComputingShare->RequestedSlots = stringtoi((std::string)xComputingShare["RequestedSlots"]);
        }
        if (xComputingShare["Name"]) {
          ComputingShare->Name = (std::string)xComputingShare["Name"];
        }
        if (xComputingShare["MappingQueue"]) {
          ComputingShare->MappingQueue = (std::string)xComputingShare["MappingQueue"];
        }
        if (xComputingShare["MaxWallTime"]) {
          ComputingShare->MaxWallTime = (std::string)xComputingShare["MaxWallTime"];
        }
        if (xComputingShare["MaxTotalWallTime"]) {
          ComputingShare->MaxTotalWallTime = (std::string)xComputingShare["MaxTotalWallTime"];
        }
        if (xComputingShare["MinWallTime"]) {
          ComputingShare->MinWallTime = (std::string)xComputingShare["MinWallTime"];
        }
        if (xComputingShare["DefaultWallTime"]) {
          ComputingShare->DefaultWallTime = (std::string)xComputingShare["DefaultWallTime"];
        }
        if (xComputingShare["MaxCPUTime"]) {
          ComputingShare->MaxCPUTime = (std::string)xComputingShare["MaxCPUTime"];
        }
        if (xComputingShare["MaxTotalCPUTime"]) {
          ComputingShare->MaxTotalCPUTime = (std::string)xComputingShare["MaxTotalCPUTime"];
        }
        if (xComputingShare["MinCPUTime"]) {
          ComputingShare->MinCPUTime = (std::string)xComputingShare["MinCPUTime"];
        }
        if (xComputingShare["DefaultCPUTime"]) {
          ComputingShare->DefaultCPUTime = (std::string)xComputingShare["DefaultCPUTime"];
        }
        if (xComputingShare["MaxTotalJobs"]) {
          ComputingShare->MaxTotalJobs = stringtoi((std::string)xComputingShare["MaxTotalJobs"]);
        }
        if (xComputingShare["MaxRunningJobs"]) {
          ComputingShare->MaxRunningJobs = stringtoi((std::string)xComputingShare["MaxRunningJobs"]);
        }
        if (xComputingShare["MaxWaitingJobs"]) {
          ComputingShare->MaxWaitingJobs = stringtoi((std::string)xComputingShare["MaxWaitingJobs"]);
        }
        if (xComputingShare["MaxPreLRMSWaitingJobs"]) {
          ComputingShare->MaxPreLRMSWaitingJobs = stringtoi((std::string)xComputingShare["MaxPreLRMSWaitingJobs"]);
        }
        if (xComputingShare["MaxUserRunningJobs"]) {
          ComputingShare->MaxUserRunningJobs = stringtoi((std::string)xComputingShare["MaxUserRunningJobs"]);
        }
        if (xComputingShare["MaxSlotsPerJob"]) {
          ComputingShare->MaxSlotsPerJob = stringtoi((std::string)xComputingShare["MaxSlotsPerJob"]);
        }
        if (xComputingShare["MaxStageInStreams"]) {
          ComputingShare->MaxStageInStreams = stringtoi((std::string)xComputingShare["MaxStageInStreams"]);
        }
        if (xComputingShare["MaxStageOutStreams"]) {
          ComputingShare->MaxStageOutStreams = stringtoi((std::string)xComputingShare["MaxStageOutStreams"]);
        }
        if (xComputingShare["SchedulingPolicy"]) {
          ComputingShare->SchedulingPolicy = (std::string)xComputingShare["SchedulingPolicy"];
        }
        if (xComputingShare["MaxMainMemory"]) {
          ComputingShare->MaxMainMemory = stringtoi((std::string)xComputingShare["MaxMainMemory"]);
        }
        if (xComputingShare["MaxVirtualMemory"]) {
          ComputingShare->MaxVirtualMemory = stringtoi((std::string)xComputingShare["MaxVirtualMemory"]);
        }
        if (xComputingShare["MaxDiskSpace"]) {
          ComputingShare->MaxDiskSpace = stringtoi((std::string)xComputingShare["MaxDiskSpace"]);
        }
        if (xComputingShare["DefaultStorageService"]) {
          ComputingShare->DefaultStorageService = (std::string)xComputingShare["DefaultStorageService"];
        }
        if (xComputingShare["Preemption"]) {
          ComputingShare->Preemption = ((std::string)xComputingShare["Preemption"] == "true") ? true : false;
        }
        if (xComputingShare["EstimatedAverageWaitingTime"]) {
          ComputingShare->EstimatedAverageWaitingTime = (std::string)xComputingShare["EstimatedAverageWaitingTime"];
        }
        if (xComputingShare["EstimatedWorstWaitingTime"]) {
          ComputingShare->EstimatedWorstWaitingTime = stringtoi((std::string)xComputingShare["EstimatedWorstWaitingTime"]);
        }
        if (xComputingShare["ReservationPolicy"]) {
          ComputingShare->ReservationPolicy = stringtoi((std::string)xComputingShare["ReservationPolicy"]);
        }

        cs.ComputingShare.insert(std::pair<int, ComputingShareType>(shareID++, ComputingShare));
      }

      /*
       * A ComputingShare is linked to multiple ExecutionEnvironments.
       * Due to bug 2101 multiple ExecutionEnvironments per ComputingShare
       * will be ignored. The ExecutionEnvironment information will only be
       * stored if there is one ExecutionEnvironment associated with a
       * ComputingShare.
       */
      /*
       * TODO: Store ExecutionEnvironment information in the list of
       * ExecutionEnvironmentType objects and issue a warning when the
       * resources published in multiple ExecutionEnvironment are
       * requested in a job description document.
       */

      int managerID = 0;
      for (XMLNode xComputingManager = GLUEService["ComputingManager"]; (bool)xComputingManager; ++xComputingManager) {
        ComputingManagerType ComputingManager;
        if (xComputingManager["ProductName"]) {
          ComputingManager->ProductName = (std::string)xComputingManager["ProductName"];
        }
        // The GlUE2 specification does not have attribute ComputingManager.Type
        //if (xComputingManager["Type"]) {
        //  ComputingManager->Type = (std::string)xComputingManager["Type"];
        //}
        if (xComputingManager["ProductVersion"]) {
          ComputingManager->ProductVersion = (std::string)xComputingManager["ProductVersion"];
        }
        if (xComputingManager["Reservation"]) {
          ComputingManager->Reservation = ((std::string)xComputingManager["Reservation"] == "true");
        }
        if (xComputingManager["BulkSubmission"]) {
          ComputingManager->BulkSubmission = ((std::string)xComputingManager["BulkSubmission"] == "true");
        }
        if (xComputingManager["TotalPhysicalCPUs"]) {
          ComputingManager->TotalPhysicalCPUs = stringtoi((std::string)xComputingManager["TotalPhysicalCPUs"]);
        }
        if (xComputingManager["TotalLogicalCPUs"]) {
          ComputingManager->TotalLogicalCPUs = stringtoi((std::string)xComputingManager["TotalLogicalCPUs"]);
        }
        if (xComputingManager["TotalSlots"]) {
          ComputingManager->TotalSlots = stringtoi((std::string)xComputingManager["TotalSlots"]);
        }
        if (xComputingManager["Homogeneous"]) {
          ComputingManager->Homogeneous = ((std::string)xComputingManager["Homogeneous"] == "true");
        }
        if (xComputingManager["NetworkInfo"]) {
          for (XMLNode n = xComputingManager["NetworkInfo"]; n; ++n) {
            ComputingManager->NetworkInfo.push_back((std::string)n);
          }
        }
        if (xComputingManager["WorkingAreaShared"]) {
          ComputingManager->WorkingAreaShared = ((std::string)xComputingManager["WorkingAreaShared"] == "true");
        }
        if (xComputingManager["WorkingAreaFree"]) {
          ComputingManager->WorkingAreaFree = stringtoi((std::string)xComputingManager["WorkingAreaFree"]);
        }
        if (xComputingManager["WorkingAreaTotal"]) {
          ComputingManager->WorkingAreaTotal = stringtoi((std::string)xComputingManager["WorkingAreaTotal"]);
        }
        if (xComputingManager["WorkingAreaLifeTime"]) {
          ComputingManager->WorkingAreaLifeTime = (std::string)xComputingManager["WorkingAreaLifeTime"];
        }
        if (xComputingManager["CacheFree"]) {
          ComputingManager->CacheFree = stringtoi((std::string)xComputingManager["CacheFree"]);
        }
        if (xComputingManager["CacheTotal"]) {
          ComputingManager->CacheTotal = stringtoi((std::string)xComputingManager["CacheTotal"]);
        }
        for (XMLNode n = xComputingManager["Benchmark"]; n; ++n) {
          double value;
          if (n["Type"] && n["Value"] &&
              stringto((std::string)n["Value"], value)) {
            (*ComputingManager.Benchmarks)[(std::string)n["Type"]] = value;
          } else {
            logger.msg(VERBOSE, "Couldn't parse benchmark XML:\n%s", (std::string)n);
            continue;
          }
        }
        for (XMLNode n = xComputingManager["ApplicationEnvironments"]["ApplicationEnvironment"]; n; ++n) {
          ApplicationEnvironment ae((std::string)n["AppName"], (std::string)n["AppVersion"]);
          ae.State = (std::string)n["State"];
          if (n["FreeSlots"]) {
            ae.FreeSlots = stringtoi((std::string)n["FreeSlots"]);
          }
          //else {
          //  ae.FreeSlots = ComputingShare->FreeSlots; // Non compatible??, i.e. a ComputingShare is unrelated to the ApplicationEnvironment.
          //}
          if (n["FreeJobs"]) {
            ae.FreeJobs = stringtoi((std::string)n["FreeJobs"]);
          } else {
            ae.FreeJobs = -1;
          }
          if (n["FreeUserSeats"]) {
            ae.FreeUserSeats = stringtoi((std::string)n["FreeUserSeats"]);
          } else {
            ae.FreeUserSeats = -1;
          }
          ComputingManager.ApplicationEnvironments->push_back(ae);
        }

        int eeID = 0;
        for (XMLNode xExecutionEnvironment = xComputingManager["ExecutionEnvironments"]["ExecutionEnvironment"]; (bool)xExecutionEnvironment; ++xExecutionEnvironment) {
          ExecutionEnvironmentType ExecutionEnvironment;
          if (xExecutionEnvironment["Platform"]) {
            ExecutionEnvironment->Platform = (std::string)xExecutionEnvironment["Platform"];
          }

          if (xExecutionEnvironment["MainMemorySize"]) {
            ExecutionEnvironment->MainMemorySize = stringtoi((std::string)xExecutionEnvironment["MainMemorySize"]);
          }

          if (xExecutionEnvironment["OSName"]) {
            if (xExecutionEnvironment["OSVersion"]) {
              if (xExecutionEnvironment["OSFamily"]) {
                ExecutionEnvironment->OperatingSystem = Software((std::string)xExecutionEnvironment["OSFamily"],
                                                                 (std::string)xExecutionEnvironment["OSName"],
                                                                 (std::string)xExecutionEnvironment["OSVersion"]);
              }
              else {
                ExecutionEnvironment->OperatingSystem = Software((std::string)xExecutionEnvironment["OSName"],
                                                                 (std::string)xExecutionEnvironment["OSVersion"]);
              }
            }
            else {
              ExecutionEnvironment->OperatingSystem = Software((std::string)xExecutionEnvironment["OSName"]);
            }
          }

          if (xExecutionEnvironment["ConnectivityIn"]) {
            ExecutionEnvironment->ConnectivityIn = (lower((std::string)xExecutionEnvironment["ConnectivityIn"]) == "true");
          }

          if (xExecutionEnvironment["ConnectivityOut"]) {
            ExecutionEnvironment->ConnectivityOut = (lower((std::string)xExecutionEnvironment["ConnectivityOut"]) == "true");
          }
          ComputingManager.ExecutionEnvironment.insert(std::pair<int, ExecutionEnvironmentType>(eeID++, ExecutionEnvironment));
        }

        cs.ComputingManager.insert(std::pair<int, ComputingManagerType>(managerID++, ComputingManager));
      }

      targets.push_back(cs);
    }
  }

} // namespace Arc
