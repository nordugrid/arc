// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/StringConv.h>

#include "GLUE2.h"

namespace Arc {

  Logger GLUE2::logger(Logger::getRootLogger(), "GLUE2");

  void GLUE2::ParseExecutionTargets(XMLNode glue2tree, std::list<ExecutionTarget>& targets,const std::string& interface) {

    XMLNode GLUEService = glue2tree;
    if(GLUEService.Name() != "ComputingService") {
      GLUEService = glue2tree["ComputingService"];
    }

    for (; GLUEService; ++GLUEService) {

      XMLNode xmlCENode = GLUEService["ComputingEndpoint"];
      if(!interface.empty()) {
        for(;(bool)xmlCENode;++xmlCENode) {
          if((xmlCENode["InterfaceName"] == interface) ||
             (xmlCENode["Interface"] == interface)) break;
        }
        if(!xmlCENode) {
          continue;
        }
      }

      targets.push_back(ExecutionTarget());

      logger.msg(VERBOSE, "Generating computing target: %s", targets.back().Cluster.str());

      if (xmlCENode["HealthState"]) {
        targets.back().ComputingEndpoint.HealthState = (std::string)xmlCENode["HealthState"];
      } else {
        logger.msg(VERBOSE, "The Service advertises no Health State.");
      }
      if (xmlCENode["HealthStateInfo"]) {
        // TODO: Map to HealthStateInfo instead?
        targets.back().ComputingEndpoint.HealthState = (std::string)xmlCENode["HealthStateInfo"];
      }
      if (GLUEService["Name"]) {
        targets.back().ComputingService.Name = (std::string)GLUEService["Name"];
      }
      if (xmlCENode["Capability"]) {
        for (XMLNode n = xmlCENode["Capability"]; n; ++n) {
          targets.back().ComputingEndpoint.Capability.push_back((std::string)n);
        }
      } else if (GLUEService["Capability"]) {
        for (XMLNode n = GLUEService["Capability"]; n; ++n) {
          targets.back().ComputingEndpoint.Capability.push_back((std::string)n);
        }
      }
      if (GLUEService["Type"]) {
        targets.back().ComputingService.Type = (std::string)GLUEService["Type"];
      } else {
        logger.msg(VERBOSE, "The Service doesn't advertise its Type.");
      }
      if (xmlCENode["QualityLevel"]) {
        targets.back().ComputingEndpoint.QualityLevel = (std::string)xmlCENode["QualityLevel"];
      } else if (GLUEService["QualityLevel"]) {
        targets.back().ComputingEndpoint.QualityLevel = (std::string)GLUEService["QualityLevel"];
      } else {
        logger.msg(VERBOSE, "The Service doesn't advertise its Quality Level.");
      }

      if (xmlCENode["Technology"]) {
        targets.back().ComputingEndpoint.Technology = (std::string)xmlCENode["Technology"];
      }
      if (xmlCENode["InterfaceName"]) {
        targets.back().ComputingEndpoint.InterfaceName = (std::string)xmlCENode["InterfaceName"];
      } else if (xmlCENode["Interface"]) {
        targets.back().ComputingEndpoint.InterfaceName = (std::string)xmlCENode["Interface"];
      } else {
        logger.msg(VERBOSE, "The Service doesn't advertise its Interface.");
      }
      if (xmlCENode["InterfaceVersion"]) {
        targets.back().ComputingEndpoint.InterfaceName = (std::string)xmlCENode["InterfaceVersion"];
      }
      if (xmlCENode["InterfaceExtension"]) {
        for (XMLNode n = xmlCENode["InterfaceExtension"]; n; ++n) {
          targets.back().ComputingEndpoint.InterfaceExtension.push_back((std::string)n);
        }
      }
      if (xmlCENode["SupportedProfile"]) {
        for (XMLNode n = xmlCENode["SupportedProfile"]; n; ++n) {
          targets.back().ComputingEndpoint.SupportedProfile.push_back((std::string)n);
        }
      }
      if (xmlCENode["Implementor"]) {
        targets.back().ComputingEndpoint.Implementor = (std::string)xmlCENode["Implementor"];
      }
      if (xmlCENode["ImplementationName"]) {
        if (xmlCENode["ImplementationVersion"]) {
          targets.back().ComputingEndpoint.Implementation =
            Software((std::string)xmlCENode["ImplementationName"],
                     (std::string)xmlCENode["ImplementationVersion"]);
        } else {
          targets.back().ComputingEndpoint.Implementation = Software((std::string)xmlCENode["ImplementationName"]);
        }
      }
      if (xmlCENode["ServingState"]) {
        targets.back().ComputingEndpoint.ServingState = (std::string)xmlCENode["ServingState"];
      } else {
        logger.msg(VERBOSE, "The Service doesn't advertise its Serving State.");
      }
      if (xmlCENode["IssuerCA"]) {
        targets.back().ComputingEndpoint.IssuerCA = (std::string)xmlCENode["IssuerCA"];
      }
      if (xmlCENode["TrustedCA"]) {
        XMLNode n = xmlCENode["TrustedCA"];
        while (n) {
          targets.back().ComputingEndpoint.TrustedCA.push_back((std::string)n);
          ++n; //The increment operator works in an unusual manner (returns void)
        }
      }
      if (xmlCENode["DowntimeStart"]) {
        targets.back().ComputingEndpoint.DowntimeStarts = (std::string)xmlCENode["DowntimeStart"];
      }
      if (xmlCENode["DowntimeEnd"]) {
        targets.back().ComputingEndpoint.DowntimeEnds = (std::string)xmlCENode["DowntimeEnd"];
      }
      if (xmlCENode["Staging"]) {
        targets.back().ComputingEndpoint.Staging = (std::string)xmlCENode["Staging"];
      }
      if (xmlCENode["JobDescription"]) {
        for (XMLNode n = xmlCENode["JobDescription"]; n; ++n) {
          targets.back().ComputingEndpoint.JobDescriptions.push_back((std::string)n);
        }
      }

      //Attributes below should possibly consider elements in different places (Service/Endpoint/Share etc).
      if (xmlCENode["TotalJobs"]) {
        targets.back().ComputingShare.TotalJobs = stringtoi((std::string)xmlCENode["TotalJobs"]);
      } else if (GLUEService["TotalJobs"]) {
        targets.back().ComputingShare.TotalJobs = stringtoi((std::string)GLUEService["TotalJobs"]);
      }
      if (xmlCENode["RunningJobs"]) {
        targets.back().ComputingShare.RunningJobs = stringtoi((std::string)xmlCENode["RunningJobs"]);
      } else if (GLUEService["RunningJobs"]) {
        targets.back().ComputingShare.RunningJobs = stringtoi((std::string)GLUEService["RunningJobs"]);
      }
      if (xmlCENode["WaitingJobs"]) {
        targets.back().ComputingShare.WaitingJobs = stringtoi((std::string)xmlCENode["WaitingJobs"]);
      } else if (GLUEService["WaitingJobs"]) {
        targets.back().ComputingShare.WaitingJobs = stringtoi((std::string)GLUEService["WaitingJobs"]);
      }
      if (xmlCENode["StagingJobs"]) {
        targets.back().ComputingShare.StagingJobs = stringtoi((std::string)xmlCENode["StagingJobs"]);
      } else if (GLUEService["StagingJobs"]) {
        targets.back().ComputingShare.StagingJobs = stringtoi((std::string)GLUEService["StagingJobs"]);
      }
      if (xmlCENode["SuspendedJobs"]) {
        targets.back().ComputingShare.SuspendedJobs = stringtoi((std::string)xmlCENode["SuspendedJobs"]);
      } else if (GLUEService["SuspendedJobs"]) {
        targets.back().ComputingShare.SuspendedJobs = stringtoi((std::string)GLUEService["SuspendedJobs"]);
      }
      if (xmlCENode["PreLRMSWaitingJobs"]) {
        targets.back().ComputingShare.PreLRMSWaitingJobs = stringtoi((std::string)xmlCENode["PreLRMSWaitingJobs"]);
      } else if (GLUEService["PreLRMSWaitingJobs"]) {
        targets.back().ComputingShare.PreLRMSWaitingJobs = stringtoi((std::string)GLUEService["PreLRMSWaitingJobs"]);
      }
      if (xmlCENode["LocalRunningJobs"]) {
        targets.back().ComputingShare.LocalRunningJobs = stringtoi((std::string)xmlCENode["LocalRunningJobs"]);
      } else if (GLUEService["LocalRunningJobs"]) {
        targets.back().ComputingShare.LocalRunningJobs = stringtoi((std::string)GLUEService["LocalRunningJobs"]);
      }
      if (xmlCENode["LocalWaitingJobs"]) {
        targets.back().ComputingShare.LocalWaitingJobs = stringtoi((std::string)xmlCENode["LocalWaitingJobs"]);
      } else if (GLUEService["LocalWaitingJobs"]) {
        targets.back().ComputingShare.LocalWaitingJobs = stringtoi((std::string)GLUEService["LocalWaitingJobs"]);
      }
      if (xmlCENode["LocalSuspendedJobs"]) {
        targets.back().ComputingShare.LocalSuspendedJobs = stringtoi((std::string)xmlCENode["LocalSuspendedJobs"]);
      } else if (GLUEService["LocalSuspendedJobs"]) {
        targets.back().ComputingShare.LocalWaitingJobs = stringtoi((std::string)GLUEService["LocalSuspendedJobs"]);
      }

      XMLNode ComputingManager = GLUEService["ComputingManager"];
      if (ComputingManager["ProductName"]) {
        targets.back().ComputingManager.ProductName = (std::string)ComputingManager["ProductName"];
      } else if (ComputingManager["Type"]) { // is this non-standard fallback needed?
        targets.back().ComputingManager.ProductName = (std::string)ComputingManager["Type"];
      }
      if (ComputingManager["ProductVersion"]) {
        targets.back().ComputingManager.ProductVersion = (std::string)ComputingManager["ProductVersion"];
      }
      if (ComputingManager["Reservation"]) {
        targets.back().ComputingManager.Reservation = ((std::string)ComputingManager["Reservation"] == "true") ? true : false;
      }
      if (ComputingManager["BulkSubmission"]) {
        targets.back().ComputingManager.BulkSubmission = ((std::string)ComputingManager["BulkSubmission"] == "true") ? true : false;
      }
      if (ComputingManager["TotalPhysicalCPUs"]) {
        targets.back().ComputingManager.TotalPhysicalCPUs = stringtoi((std::string)ComputingManager["TotalPhysicalCPUs"]);
      }
      if (ComputingManager["TotalLogicalCPUs"]) {
        targets.back().ComputingManager.TotalLogicalCPUs = stringtoi((std::string)ComputingManager["TotalLogicalCPUs"]);
      }
      if (ComputingManager["TotalSlots"]) {
        targets.back().ComputingManager.TotalSlots = stringtoi((std::string)ComputingManager["TotalSlots"]);
      }
      if (ComputingManager["Homogeneous"]) {
        targets.back().ComputingManager.Homogeneous = ((std::string)ComputingManager["Homogeneous"] == "true") ? true : false;
      }
      if (ComputingManager["NetworkInfo"]) {
        for (XMLNode n = ComputingManager["NetworkInfo"]; n; ++n) {
          targets.back().ComputingManager.NetworkInfo.push_back((std::string)n);
        }
      }
      if (ComputingManager["WorkingAreaShared"]) {
        targets.back().ComputingManager.WorkingAreaShared = ((std::string)ComputingManager["WorkingAreaShared"] == "true") ? true : false;
      }
      if (ComputingManager["WorkingAreaFree"]) {
        targets.back().ComputingManager.WorkingAreaFree = stringtoi((std::string)ComputingManager["WorkingAreaFree"]);
      }
      if (ComputingManager["WorkingAreaTotal"]) {
        targets.back().ComputingManager.WorkingAreaTotal = stringtoi((std::string)ComputingManager["WorkingAreaTotal"]);
      }
      if (ComputingManager["WorkingAreaLifeTime"]) {
        targets.back().ComputingManager.WorkingAreaLifeTime = (std::string)ComputingManager["WorkingAreaLifeTime"];
      }
      if (ComputingManager["CacheFree"]) {
        targets.back().ComputingManager.CacheFree = stringtoi((std::string)ComputingManager["CacheFree"]);
      }
      if (ComputingManager["CacheTotal"]) {
        targets.back().ComputingManager.CacheTotal = stringtoi((std::string)ComputingManager["CacheTotal"]);
      }
      for (XMLNode n = ComputingManager["Benchmark"]; n; ++n) {
        double value;
        if (n["Type"] && n["Value"] &&
            stringto((std::string)n["Value"], value)) {
          targets.back().Benchmarks[(std::string)n["Type"]] = value;
        } else {
          logger.msg(VERBOSE, "Couldn't parse benchmark XML:\n%s", (std::string)n);
          continue;
        }
      }
      for (XMLNode n = ComputingManager["ApplicationEnvironments"]["ApplicationEnvironment"]; n; ++n) {
        ApplicationEnvironment ae((std::string)n["AppName"], (std::string)n["AppVersion"]);
        ae.State = (std::string)n["State"];
        if (n["FreeSlots"]) {
          ae.FreeSlots = stringtoi((std::string)n["FreeSlots"]);
        } else {
          ae.FreeSlots = targets.back().ComputingShare.FreeSlots;
        }
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
        targets.back().ApplicationEnvironments.push_back(ae);
      }

      XMLNode ComputingShare = GLUEService["ComputingShare"];
      for (int i = 0; ComputingShare[i]; i++) {
        ExecutionTarget& currentTarget = targets.back();
        if (ComputingShare[i+1]) {
          targets.push_back(ExecutionTarget(targets.back()));
        }

        if (ComputingShare[i]["FreeSlots"]) {
          currentTarget.ComputingShare.FreeSlots = stringtoi((std::string)ComputingShare[i]["FreeSlots"]);
        }
        if (ComputingShare[i]["FreeSlotsWithDuration"]) {
          // Format: ns[:t] [ns[:t]]..., where ns is number of slots and t is the duration.
          currentTarget.ComputingShare.FreeSlotsWithDuration.clear();

          const std::string fswdValue = (std::string)ComputingShare[i]["FreeSlotsWithDuration"];
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

            currentTarget.ComputingShare.FreeSlotsWithDuration[Period(duration)] = freeSlots;
          }
        }
        if (ComputingShare[i]["UsedSlots"]) {
          currentTarget.ComputingShare.UsedSlots = stringtoi((std::string)ComputingShare[i]["UsedSlots"]);
        }
        if (ComputingShare[i]["RequestedSlots"]) {
          currentTarget.ComputingShare.RequestedSlots = stringtoi((std::string)ComputingShare[i]["RequestedSlots"]);
        }
        if (ComputingShare[i]["Name"]) {
          currentTarget.ComputingShare.Name = (std::string)ComputingShare[i]["Name"];
        }
        if (ComputingShare[i]["MaxWallTime"]) {
          currentTarget.ComputingShare.MaxWallTime = (std::string)ComputingShare[i]["MaxWallTime"];
        }
        if (ComputingShare[i]["MaxTotalWallTime"]) {
          currentTarget.ComputingShare.MaxTotalWallTime = (std::string)ComputingShare[i]["MaxTotalWallTime"];
        }
        if (ComputingShare[i]["MinWallTime"]) {
          currentTarget.ComputingShare.MinWallTime = (std::string)ComputingShare[i]["MinWallTime"];
        }
        if (ComputingShare[i]["DefaultWallTime"]) {
          currentTarget.ComputingShare.DefaultWallTime = (std::string)ComputingShare[i]["DefaultWallTime"];
        }
        if (ComputingShare[i]["MaxCPUTime"]) {
          currentTarget.ComputingShare.MaxCPUTime = (std::string)ComputingShare[i]["MaxCPUTime"];
        }
        if (ComputingShare[i]["MaxTotalCPUTime"]) {
          currentTarget.ComputingShare.MaxTotalCPUTime = (std::string)ComputingShare[i]["MaxTotalCPUTime"];
        }
        if (ComputingShare[i]["MinCPUTime"]) {
          currentTarget.ComputingShare.MinCPUTime = (std::string)ComputingShare[i]["MinCPUTime"];
        }
        if (ComputingShare[i]["DefaultCPUTime"]) {
          currentTarget.ComputingShare.DefaultCPUTime = (std::string)ComputingShare[i]["DefaultCPUTime"];
        }
        if (ComputingShare[i]["MaxTotalJobs"]) {
          currentTarget.ComputingShare.MaxTotalJobs = stringtoi((std::string)ComputingShare[i]["MaxTotalJobs"]);
        }
        if (ComputingShare[i]["MaxRunningJobs"]) {
          currentTarget.ComputingShare.MaxRunningJobs = stringtoi((std::string)ComputingShare[i]["MaxRunningJobs"]);
        }
        if (ComputingShare[i]["MaxWaitingJobs"]) {
          currentTarget.ComputingShare.MaxWaitingJobs = stringtoi((std::string)ComputingShare[i]["MaxWaitingJobs"]);
        }
        if (ComputingShare[i]["MaxPreLRMSWaitingJobs"]) {
          currentTarget.ComputingShare.MaxPreLRMSWaitingJobs = stringtoi((std::string)ComputingShare[i]["MaxPreLRMSWaitingJobs"]);
        }
        if (ComputingShare[i]["MaxUserRunningJobs"]) {
          currentTarget.ComputingShare.MaxUserRunningJobs = stringtoi((std::string)ComputingShare[i]["MaxUserRunningJobs"]);
        }
        if (ComputingShare[i]["MaxSlotsPerJob"]) {
          currentTarget.ComputingShare.MaxSlotsPerJob = stringtoi((std::string)ComputingShare[i]["MaxSlotsPerJob"]);
        }
        if (ComputingShare[i]["MaxStageInStreams"]) {
          currentTarget.ComputingShare.MaxStageInStreams = stringtoi((std::string)ComputingShare[i]["MaxStageInStreams"]);
        }
        if (ComputingShare[i]["MaxStageOutStreams"]) {
          currentTarget.ComputingShare.MaxStageOutStreams = stringtoi((std::string)ComputingShare[i]["MaxStageOutStreams"]);
        }
        if (ComputingShare[i]["SchedulingPolicy"]) {
          currentTarget.ComputingShare.SchedulingPolicy = (std::string)ComputingShare[i]["SchedulingPolicy"];
        }
        if (ComputingShare[i]["MaxMainMemory"]) {
          currentTarget.ComputingShare.MaxMainMemory = stringtoi((std::string)ComputingShare[i]["MaxMainMemory"]);
        }
        if (ComputingShare[i]["MaxVirtualMemory"]) {
          currentTarget.ComputingShare.MaxVirtualMemory = stringtoi((std::string)ComputingShare[i]["MaxVirtualMemory"]);
        }
        if (ComputingShare[i]["MaxDiskSpace"]) {
          currentTarget.ComputingShare.MaxDiskSpace = stringtoi((std::string)ComputingShare[i]["MaxDiskSpace"]);
        }
        if (ComputingShare[i]["DefaultStorageService"]) {
          currentTarget.ComputingShare.DefaultStorageService = (std::string)ComputingShare[i]["DefaultStorageService"];
        }
        if (ComputingShare[i]["Preemption"]) {
          currentTarget.ComputingShare.Preemption = ((std::string)ComputingShare[i]["Preemption"] == "true") ? true : false;
        }
        if (ComputingShare[i]["EstimatedAverageWaitingTime"]) {
          currentTarget.ComputingShare.EstimatedAverageWaitingTime = (std::string)ComputingShare[i]["EstimatedAverageWaitingTime"];
        }
        if (ComputingShare[i]["EstimatedWorstWaitingTime"]) {
          currentTarget.ComputingShare.EstimatedWorstWaitingTime = stringtoi((std::string)ComputingShare[i]["EstimatedWorstWaitingTime"]);
        }
        if (ComputingShare[i]["ReservationPolicy"]) {
          currentTarget.ComputingShare.ReservationPolicy = stringtoi((std::string)ComputingShare[i]["ReservationPolicy"]);
        }

        /*
         * A ComputingShare is linked to multiple ExecutionEnvironments.
         * Due to bug 2101 multiple ExecutionEnvironments per ComputingShare
         * will be ignored. The ExecutionEnvironment information will only be
         * stored if there is one ExecutionEnvironment associated with a
         * ComputingShare.
         */
        if (ComputingShare[i]["Associations"]["ExecutionEnvironmentID"][1]) { // Check if there are multiple ExecutionEnvironments associated with this ComputingShare.
          logger.msg(WARNING, "Multiple execution environments per queue specified for target. Execution environment information will be ignored.");
        }
        else {
          if (ComputingShare[i]["Associations"]["ExecutionEnvironmentID"]) {
            logger.msg(DEBUG, "ComputingShare is associated with the ExecutionEnvironment \"%s\"", (std::string)ComputingShare[i]["Associations"]["ExecutionEnvironmentID"]);
            XMLNode ExecutionEnvironment = ComputingManager["ExecutionEnvironments"]["ExecutionEnvironment"];
            for (int j = 0; ExecutionEnvironment[j]; j++) {
              if (ExecutionEnvironment[j]["Name"] &&
                  (std::string)ExecutionEnvironment[j]["Name"] == (std::string)ComputingShare[i]["Associations"]["ExecutionEnvironmentID"]) {
                ExecutionEnvironment = ExecutionEnvironment[j];
              }
            }

            if (ExecutionEnvironment) {
              logger.msg(DEBUG, "ExecutionEnvironment \"%s\" located", (std::string)ComputingShare[i]["Associations"]["ExecutionEnvironmentID"]);

              if (ExecutionEnvironment["Platform"]) {
                currentTarget.Platform = (std::string)ExecutionEnvironment["Platform"];
              }

              if (ExecutionEnvironment["MainMemorySize"]) {
                currentTarget.MainMemorySize = stringtoi((std::string)ExecutionEnvironment["MainMemorySize"]);
              }

              if (ExecutionEnvironment["OSName"]) {
                if (ExecutionEnvironment["OSVersion"]) {
                  if (ExecutionEnvironment["OSFamily"]) {
                    currentTarget.OperatingSystem = Software((std::string)ExecutionEnvironment["OSFamily"],
                                                             (std::string)ExecutionEnvironment["OSName"],
                                                             (std::string)ExecutionEnvironment["OSVersion"]);
                  }
                  else {
                    currentTarget.OperatingSystem = Software((std::string)ExecutionEnvironment["OSName"],
                                                             (std::string)ExecutionEnvironment["OSVersion"]);
                  }
                }
                else {
                  currentTarget.OperatingSystem = Software((std::string)ExecutionEnvironment["OSName"]);
                }
              }

              if (ExecutionEnvironment["ConnectivityIn"]) {
                currentTarget.ConnectivityIn = (lower((std::string)ExecutionEnvironment["ConnectivityIn"]) == "true");
              }

              if (ExecutionEnvironment["ConnectivityOut"]) {
                currentTarget.ConnectivityOut = (lower((std::string)ExecutionEnvironment["ConnectivityOut"]) == "true");
              }
            }
          }
        }
      }
    }
  }

} // namespace Arc
