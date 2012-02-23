// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/Logger.h>
#include <arc/URL.h>
#include <arc/UserConfig.h>
#include <arc/XMLNode.h>
#include <arc/client/EndpointQueryingStatus.h>
#include <arc/client/ExecutionTarget.h>
#include <arc/message/MCC.h>

// Temporary solution
#include "../ARC1/JobStateBES.cpp"
#include "../ARC1/JobStateARC1.cpp"
#include "../ARC1/AREXClient.cpp"

#include "TargetInformationRetrieverPluginWSRFGLUE2.h"

namespace Arc {

  Logger TargetInformationRetrieverPluginWSRFGLUE2::logger(Logger::getRootLogger(), "TargetInformationRetrieverPlugin.WSRFGLUE2");

  /*
  static URL CreateURL(std::string service) {
    std::string::size_type pos1 = service.find("://");
    if (pos1 == std::string::npos) {
      service = "https://" + service;
    } else {
      std::string proto = lower(service.substr(0,pos1));
      if((proto != "http") && (proto != "https")) return URL();
    }
    // Default port other than 443?
    // Default path?
    return service;
  }
  */

  EndpointQueryingStatus TargetInformationRetrieverPluginWSRFGLUE2::Query(const UserConfig& uc, const ComputingInfoEndpoint& cie, std::list<ExecutionTarget>& etList, const EndpointFilter<ExecutionTarget>&) const {
    EndpointQueryingStatus s(EndpointQueryingStatus::FAILED);

    logger.msg(DEBUG, "Querying WSRF GLUE2 computing info endpoint.");

    URL url(cie.Endpoint);
    if (!url) {
      return s;
    }

    MCCConfig cfg;
    uc.ApplyToConfig(cfg);
    AREXClient ac(url, cfg, uc.Timeout(), true); // Temporary
    //AREXClient ac(url, cfg, uc.Timeout(), /* thrarg->flavour == "ARC1" */); // TIR equivalent
    XMLNode servicesQueryResponse;
    if (!ac.sstat(servicesQueryResponse)) {
      return s;
    }

    ExtractTargets(url, servicesQueryResponse, etList);

    s = EndpointQueryingStatus::SUCCESSFUL;
    return s;
  }

  void TargetInformationRetrieverPluginWSRFGLUE2::ExtractTargets(const URL& url, XMLNode response, std::list<ExecutionTarget>& targets) {
    /*
     * A-REX will not return multiple ComputingService elements, but if the
     * response comes from an index server then there might be multiple.
     */
    for (XMLNode GLUEService = response["ComputingService"];
         GLUEService; ++GLUEService) {
      ExecutionTarget t;

      //t.GridFlavour = "ARC1"; // TIR equivalent
      t.Cluster = url;
      t.url = url;
      t.InterfaceName = "BES";
      t.Implementor = "NorduGrid";

      t.DomainName = url.Host();

      logger.msg(VERBOSE, "Generating A-REX target: %s", t.Cluster.str());

      XMLNode ComputingEndpoint = GLUEService["ComputingEndpoint"];
      for(;(bool)ComputingEndpoint;++ComputingEndpoint) {
        if((ComputingEndpoint["InterfaceName"] == "XBES") ||
           (ComputingEndpoint["InterfaceName"] == "BES") ||
           (ComputingEndpoint["Interface"] == "XBES") ||
           (ComputingEndpoint["Interface"] == "BES") ||
           ((ComputingEndpoint["InterfaceName"] == "org.ogf.bes") &&
            (ComputingEndpoint["InterfaceExtension"] == "urn:org.nordugrid.xbes"))) {
          break;
        };
      }
      if (ComputingEndpoint["HealthState"]) {
        t.HealthState = (std::string)ComputingEndpoint["HealthState"];
      } else {
        logger.msg(VERBOSE, "The Service advertises no Health State.");
      }
      if (ComputingEndpoint["HealthStateInfo"]) {
        t.HealthState = (std::string)ComputingEndpoint["HealthStateInfo"];
      }
      if (GLUEService["Name"]) {
        t.ServiceName = (std::string)GLUEService["Name"];
      }
      if (ComputingEndpoint["Capability"]) {
        for (XMLNode n = ComputingEndpoint["Capability"]; n; ++n) {
          t.Capability.push_back((std::string)n);
        }
      } else if (GLUEService["Capability"]) {
        for (XMLNode n = GLUEService["Capability"]; n; ++n) {
          t.Capability.push_back((std::string)n);
        }
      }
      if (GLUEService["Type"]) {
        t.ServiceType = (std::string)GLUEService["Type"];
      } else {
        logger.msg(VERBOSE, "The Service doesn't advertise its Type.");
      }
      if (ComputingEndpoint["QualityLevel"]) {
        t.QualityLevel = (std::string)ComputingEndpoint["QualityLevel"];
      } else if (GLUEService["QualityLevel"]) {
        t.QualityLevel = (std::string)GLUEService["QualityLevel"];
      } else {
        logger.msg(VERBOSE, "The Service doesn't advertise its Quality Level.");
      }

      if (ComputingEndpoint["Technology"]) {
        t.Technology = (std::string)ComputingEndpoint["Technology"];
      }
      if (ComputingEndpoint["InterfaceName"]) {
        t.InterfaceName = (std::string)ComputingEndpoint["InterfaceName"];
      } else if (ComputingEndpoint["Interface"]) {
        t.InterfaceName = (std::string)ComputingEndpoint["Interface"];
      } else {
        logger.msg(VERBOSE, "The Service doesn't advertise its Interface.");
      }
      if (ComputingEndpoint["InterfaceVersion"]) {
        t.InterfaceName = (std::string)ComputingEndpoint["InterfaceVersion"];
      }
      if (ComputingEndpoint["InterfaceExtension"]) {
        for (XMLNode n = ComputingEndpoint["InterfaceExtension"]; n; ++n) {
          t.InterfaceExtension.push_back((std::string)n);
        }
      }
      if (ComputingEndpoint["SupportedProfile"]) {
        for (XMLNode n = ComputingEndpoint["SupportedProfile"]; n; ++n) {
          t.SupportedProfile.push_back((std::string)n);
        }
      }
      if (ComputingEndpoint["Implementor"]) {
        t.Implementor = (std::string)ComputingEndpoint["Implementor"];
      }
      if (ComputingEndpoint["ImplementationName"]) {
        if (ComputingEndpoint["ImplementationVersion"]) {
          t.Implementation =
            Software((std::string)ComputingEndpoint["ImplementationName"],
                     (std::string)ComputingEndpoint["ImplementationVersion"]);
        } else {
          t.Implementation = Software((std::string)ComputingEndpoint["ImplementationName"]);
        }
      }
      if (ComputingEndpoint["ServingState"]) {
        t.ServingState = (std::string)ComputingEndpoint["ServingState"];
      } else {
        logger.msg(VERBOSE, "The Service doesn't advertise its Serving State.");
      }
      if (ComputingEndpoint["IssuerCA"]) {
        t.IssuerCA = (std::string)ComputingEndpoint["IssuerCA"];
      }
      if (ComputingEndpoint["TrustedCA"]) {
        XMLNode n = ComputingEndpoint["TrustedCA"];
        while (n) {
          t.TrustedCA.push_back((std::string)n);
          ++n; //The increment operator works in an unusual manner (returns void)
        }
      }
      if (ComputingEndpoint["DowntimeStart"]) {
        t.DowntimeStarts = (std::string)ComputingEndpoint["DowntimeStart"];
      }
      if (ComputingEndpoint["DowntimeEnd"]) {
        t.DowntimeEnds = (std::string)ComputingEndpoint["DowntimeEnd"];
      }
      if (ComputingEndpoint["Staging"]) {
        t.Staging = (std::string)ComputingEndpoint["Staging"];
      }
      if (ComputingEndpoint["JobDescription"]) {
        for (XMLNode n = ComputingEndpoint["JobDescription"]; n; ++n) {
          t.JobDescriptions.push_back((std::string)n);
        }
      }

      //Attributes below should possibly consider elements in different places (Service/Endpoint/Share etc).
      if (ComputingEndpoint["TotalJobs"]) {
        t.TotalJobs = stringtoi((std::string)ComputingEndpoint["TotalJobs"]);
      } else if (GLUEService["TotalJobs"]) {
        t.TotalJobs = stringtoi((std::string)GLUEService["TotalJobs"]);
      }
      if (ComputingEndpoint["RunningJobs"]) {
        t.RunningJobs = stringtoi((std::string)ComputingEndpoint["RunningJobs"]);
      } else if (GLUEService["RunningJobs"]) {
        t.RunningJobs = stringtoi((std::string)GLUEService["RunningJobs"]);
      }
      if (ComputingEndpoint["WaitingJobs"]) {
        t.WaitingJobs = stringtoi((std::string)ComputingEndpoint["WaitingJobs"]);
      } else if (GLUEService["WaitingJobs"]) {
        t.WaitingJobs = stringtoi((std::string)GLUEService["WaitingJobs"]);
      }
      if (ComputingEndpoint["StagingJobs"]) {
        t.StagingJobs = stringtoi((std::string)ComputingEndpoint["StagingJobs"]);
      } else if (GLUEService["StagingJobs"]) {
        t.StagingJobs = stringtoi((std::string)GLUEService["StagingJobs"]);
      }
      if (ComputingEndpoint["SuspendedJobs"]) {
        t.SuspendedJobs = stringtoi((std::string)ComputingEndpoint["SuspendedJobs"]);
      } else if (GLUEService["SuspendedJobs"]) {
        t.SuspendedJobs = stringtoi((std::string)GLUEService["SuspendedJobs"]);
      }
      if (ComputingEndpoint["PreLRMSWaitingJobs"]) {
        t.PreLRMSWaitingJobs = stringtoi((std::string)ComputingEndpoint["PreLRMSWaitingJobs"]);
      } else if (GLUEService["PreLRMSWaitingJobs"]) {
        t.PreLRMSWaitingJobs = stringtoi((std::string)GLUEService["PreLRMSWaitingJobs"]);
      }
      if (ComputingEndpoint["LocalRunningJobs"]) {
        t.LocalRunningJobs = stringtoi((std::string)ComputingEndpoint["LocalRunningJobs"]);
      } else if (GLUEService["LocalRunningJobs"]) {
        t.LocalRunningJobs = stringtoi((std::string)GLUEService["LocalRunningJobs"]);
      }
      if (ComputingEndpoint["LocalWaitingJobs"]) {
        t.LocalWaitingJobs = stringtoi((std::string)ComputingEndpoint["LocalWaitingJobs"]);
      } else if (GLUEService["LocalWaitingJobs"]) {
        t.LocalWaitingJobs = stringtoi((std::string)GLUEService["LocalWaitingJobs"]);
      }
      if (ComputingEndpoint["LocalSuspendedJobs"]) {
        t.LocalSuspendedJobs = stringtoi((std::string)ComputingEndpoint["LocalSuspendedJobs"]);
      } else if (GLUEService["LocalSuspendedJobs"]) {
        t.LocalWaitingJobs = stringtoi((std::string)GLUEService["LocalSuspendedJobs"]);
      }

      XMLNode ComputingManager = GLUEService["ComputingManager"];
      if (ComputingManager["ProductName"]) {
        t.ManagerProductName = (std::string)ComputingManager["ProductName"];
      } else if (ComputingManager["Type"]) { // is this non-standard fallback needed?
        t.ManagerProductName = (std::string)ComputingManager["Type"];
      }
      if (ComputingManager["ProductVersion"]) {
        t.ManagerProductName = (std::string)ComputingManager["ProductVersion"];
      }
      if (ComputingManager["Reservation"]) {
        t.Reservation = ((std::string)ComputingManager["Reservation"] == "true") ? true : false;
      }
      if (ComputingManager["BulkSubmission"]) {
        t.BulkSubmission = ((std::string)ComputingManager["BulkSubmission"] == "true") ? true : false;
      }
      if (ComputingManager["TotalPhysicalCPUs"]) {
        t.TotalPhysicalCPUs = stringtoi((std::string)ComputingManager["TotalPhysicalCPUs"]);
      }
      if (ComputingManager["TotalLogicalCPUs"]) {
        t.TotalLogicalCPUs = stringtoi((std::string)ComputingManager["TotalLogicalCPUs"]);
      }
      if (ComputingManager["TotalSlots"]) {
        t.TotalSlots = stringtoi((std::string)ComputingManager["TotalSlots"]);
      }
      if (ComputingManager["Homogeneous"]) {
        t.Homogeneous = ((std::string)ComputingManager["Homogeneous"] == "true") ? true : false;
      }
      if (ComputingManager["NetworkInfo"]) {
        for (XMLNode n = ComputingManager["NetworkInfo"]; n; ++n) {
          t.NetworkInfo.push_back((std::string)n);
        }
      }
      if (ComputingManager["WorkingAreaShared"]) {
        t.WorkingAreaShared = ((std::string)ComputingManager["WorkingAreaShared"] == "true") ? true : false;
      }
      if (ComputingManager["WorkingAreaFree"]) {
        t.WorkingAreaFree = stringtoi((std::string)ComputingManager["WorkingAreaFree"]);
      }
      if (ComputingManager["WorkingAreaTotal"]) {
        t.WorkingAreaTotal = stringtoi((std::string)ComputingManager["WorkingAreaTotal"]);
      }
      if (ComputingManager["WorkingAreaLifeTime"]) {
        t.WorkingAreaLifeTime = (std::string)ComputingManager["WorkingAreaLifeTime"];
      }
      if (ComputingManager["CacheFree"]) {
        t.CacheFree = stringtoi((std::string)ComputingManager["CacheFree"]);
      }
      if (ComputingManager["CacheTotal"]) {
        t.CacheTotal = stringtoi((std::string)ComputingManager["CacheTotal"]);
      }
      for (XMLNode n = ComputingManager["Benchmark"]; n; ++n) {
        double value;
        if (n["Type"] && n["Value"] &&
            stringto((std::string)n["Value"], value)) {
          t.Benchmarks[(std::string)n["Type"]] = value;
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
          ae.FreeSlots = t.FreeSlots;
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
        t.ApplicationEnvironments.push_back(ae);
      }

      XMLNode ComputingShare = GLUEService["ComputingShare"];
      for (int i = 0; ComputingShare[i]; i++) {
        ExecutionTarget& currentTarget = t;
        if (ComputingShare[i+1]) {
          targets.push_back(ExecutionTarget(t));
        }

        if (ComputingShare[i]["FreeSlots"]) {
          currentTarget.FreeSlots = stringtoi((std::string)ComputingShare[i]["FreeSlots"]);
        }
        if (ComputingShare[i]["FreeSlotsWithDuration"]) {
          // Format: ns[:t] [ns[:t]]..., where ns is number of slots and t is the duration.
          currentTarget.FreeSlotsWithDuration.clear();

          const std::string fswdValue = (std::string)ComputingShare[i]["FreeSlotsWithDuration"];
          std::list<std::string> fswdList;
          tokenize(fswdValue, fswdList);
          for (std::list<std::string>::iterator it = fswdList.begin();
               it != fswdList.end(); it++) {
            std::list<std::string> fswdPair;
            tokenize(*it, fswdPair, ":");
            long duration = LONG_MAX;
            int freeSlots = 0;
            if (fswdPair.size() > 2 || !stringto(fswdPair.front(), freeSlots) || (fswdPair.size() == 2 && !stringto(fswdPair.back(), duration))) {
              logger.msg(VERBOSE, "The \"FreeSlotsWithDuration\" attribute published by \"%s\" is wrongly formatted. Ignoring it.");
              logger.msg(DEBUG, "Wrong format of the \"FreeSlotsWithDuration\" = \"%s\" (\"%s\")", fswdValue, *it);
              continue;
            }

            currentTarget.FreeSlotsWithDuration[Period(duration)] = freeSlots;
          }
        }
        if (ComputingShare[i]["UsedSlots"]) {
          currentTarget.UsedSlots = stringtoi((std::string)ComputingShare[i]["UsedSlots"]);
        }
        if (ComputingShare[i]["RequestedSlots"]) {
          currentTarget.RequestedSlots = stringtoi((std::string)ComputingShare[i]["RequestedSlots"]);
        }
        if (ComputingShare[i]["Name"]) {
          currentTarget.ComputingShareName = (std::string)ComputingShare[i]["Name"];
        }
        if (ComputingShare[i]["MaxWallTime"]) {
          currentTarget.MaxWallTime = (std::string)ComputingShare[i]["MaxWallTime"];
        }
        if (ComputingShare[i]["MaxTotalWallTime"]) {
          currentTarget.MaxTotalWallTime = (std::string)ComputingShare[i]["MaxTotalWallTime"];
        }
        if (ComputingShare[i]["MinWallTime"]) {
          currentTarget.MinWallTime = (std::string)ComputingShare[i]["MinWallTime"];
        }
        if (ComputingShare[i]["DefaultWallTime"]) {
          currentTarget.DefaultWallTime = (std::string)ComputingShare[i]["DefaultWallTime"];
        }
        if (ComputingShare[i]["MaxCPUTime"]) {
          currentTarget.MaxCPUTime = (std::string)ComputingShare[i]["MaxCPUTime"];
        }
        if (ComputingShare[i]["MaxTotalCPUTime"]) {
          currentTarget.MaxTotalCPUTime = (std::string)ComputingShare[i]["MaxTotalCPUTime"];
        }
        if (ComputingShare[i]["MinCPUTime"]) {
          currentTarget.MinCPUTime = (std::string)ComputingShare[i]["MinCPUTime"];
        }
        if (ComputingShare[i]["DefaultCPUTime"]) {
          currentTarget.DefaultCPUTime = (std::string)ComputingShare[i]["DefaultCPUTime"];
        }
        if (ComputingShare[i]["MaxTotalJobs"]) {
          currentTarget.MaxTotalJobs = stringtoi((std::string)ComputingShare[i]["MaxTotalJobs"]);
        }
        if (ComputingShare[i]["MaxRunningJobs"]) {
          currentTarget.MaxRunningJobs = stringtoi((std::string)ComputingShare[i]["MaxRunningJobs"]);
        }
        if (ComputingShare[i]["MaxWaitingJobs"]) {
          currentTarget.MaxWaitingJobs = stringtoi((std::string)ComputingShare[i]["MaxWaitingJobs"]);
        }
        if (ComputingShare[i]["MaxPreLRMSWaitingJobs"]) {
          currentTarget.MaxPreLRMSWaitingJobs = stringtoi((std::string)ComputingShare[i]["MaxPreLRMSWaitingJobs"]);
        }
        if (ComputingShare[i]["MaxUserRunningJobs"]) {
          currentTarget.MaxUserRunningJobs = stringtoi((std::string)ComputingShare[i]["MaxUserRunningJobs"]);
        }
        if (ComputingShare[i]["MaxSlotsPerJob"]) {
          currentTarget.MaxSlotsPerJob = stringtoi((std::string)ComputingShare[i]["MaxSlotsPerJob"]);
        }
        if (ComputingShare[i]["MaxStageInStreams"]) {
          currentTarget.MaxStageInStreams = stringtoi((std::string)ComputingShare[i]["MaxStageInStreams"]);
        }
        if (ComputingShare[i]["MaxStageOutStreams"]) {
          currentTarget.MaxStageOutStreams = stringtoi((std::string)ComputingShare[i]["MaxStageOutStreams"]);
        }
        if (ComputingShare[i]["SchedulingPolicy"]) {
          currentTarget.SchedulingPolicy = (std::string)ComputingShare[i]["SchedulingPolicy"];
        }
        if (ComputingShare[i]["MaxMainMemory"]) {
          currentTarget.MaxMainMemory = stringtoi((std::string)ComputingShare[i]["MaxMainMemory"]);
        }
        if (ComputingShare[i]["MaxVirtualMemory"]) {
          currentTarget.MaxVirtualMemory = stringtoi((std::string)ComputingShare[i]["MaxVirtualMemory"]);
        }
        if (ComputingShare[i]["MaxDiskSpace"]) {
          currentTarget.MaxDiskSpace = stringtoi((std::string)ComputingShare[i]["MaxDiskSpace"]);
        }
        if (ComputingShare[i]["DefaultStorageService"]) {
          currentTarget.DefaultStorageService = (std::string)ComputingShare[i]["DefaultStorageService"];
        }
        if (ComputingShare[i]["Preemption"]) {
          currentTarget.Preemption = ((std::string)ComputingShare[i]["Preemption"] == "true") ? true : false;
        }
        if (ComputingShare[i]["EstimatedAverageWaitingTime"]) {
          currentTarget.EstimatedAverageWaitingTime = (std::string)ComputingShare[i]["EstimatedAverageWaitingTime"];
        }
        if (ComputingShare[i]["EstimatedWorstWaitingTime"]) {
          currentTarget.EstimatedWorstWaitingTime = stringtoi((std::string)ComputingShare[i]["EstimatedWorstWaitingTime"]);
        }
        if (ComputingShare[i]["ReservationPolicy"]) {
          currentTarget.ReservationPolicy = stringtoi((std::string)ComputingShare[i]["ReservationPolicy"]);
        }

        /*
         * A ComputingShare is linked to multiple ExecutionEnvironments.
         * Due to bug 2101 multiple ExecutionEnvironments per ComputingShare
         * will be ignored. The ExecutionEnvironment information will only be
         * stored if there is one ExecutionEnvironment associated with a
         * ComputingShare.
         */
        if (ComputingShare[i]["Associations"]["ExecutionEnvironmentID"][1]) { // Check if there are multiple ExecutionEnvironments associated with this ComputingShare.
          logger.msg(WARNING, "Multiple execution environments per queue specified for target: \"%s\". Execution environment information will be ignored.", url.str());
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

      targets.push_back(t);
    }
  }

} // namespace Arc
