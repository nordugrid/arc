// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/Logger.h>
#include <arc/StringConv.h>
#include <arc/URL.h>
#include <arc/UserConfig.h>
#include <arc/XMLNode.h>
#include <arc/client/EndpointQueryingStatus.h>
#include <arc/client/ExecutionTarget.h>
#include <arc/message/MCC.h>

#include "JobStateBES.h"
#include "JobStateARC1.h"
#include "AREXClient.h"
#include "TargetInformationRetrieverPluginWSRFGLUE2.h"

namespace Arc {

  Logger TargetInformationRetrieverPluginWSRFGLUE2::logger(Logger::getRootLogger(), "TargetInformationRetrieverPlugin.WSRFGLUE2");

  bool TargetInformationRetrieverPluginWSRFGLUE2::isEndpointNotSupported(const Endpoint& endpoint) const {
    const std::string::size_type pos = endpoint.URLString.find("://");
    if (pos != std::string::npos) {
      const std::string proto = lower(endpoint.URLString.substr(0, pos));
      return ((proto != "http") && (proto != "https"));
    }

    return false;
  }

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

  EndpointQueryingStatus TargetInformationRetrieverPluginWSRFGLUE2::Query(const UserConfig& uc, const Endpoint& cie, std::list<ComputingServiceType>& csList, const EndpointQueryOptions<ComputingServiceType>&) const {
    EndpointQueryingStatus s(EndpointQueryingStatus::FAILED);

    logger.msg(DEBUG, "Querying WSRF GLUE2 computing info endpoint.");

    URL url(CreateURL(cie.URLString));
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

    ExtractTargets(url, servicesQueryResponse, csList);

    if (!csList.empty()) s = EndpointQueryingStatus::SUCCESSFUL;
    return s;
  }

  void TargetInformationRetrieverPluginWSRFGLUE2::ExtractTargets(const URL& url, XMLNode response, std::list<ComputingServiceType>& csList) {
    /*
     * A-REX will not return multiple ComputingService elements, but if the
     * response comes from an index server then there might be multiple.
     */
    for (XMLNode GLUEService = response["ComputingService"]; GLUEService; ++GLUEService) {
      ComputingServiceType cs;
      AdminDomainType& AdminDomain = cs.AdminDomain;

      cs->Cluster = url;
      AdminDomain->Name = url.Host();

      if (GLUEService["Capability"]) {
        for (XMLNode n = GLUEService["Capability"]; n; ++n) {
          cs->Capability.push_back((std::string)n);
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
        logger.msg(VERBOSE, "The Service doesn't advertise its Quality Level.");
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
      // The GLUE2 specification does not have attribute ComputingService.LocalWaitingJobs
      //if (GLUEService["LocalSuspendedJobs"]) {
      //  cs->LocalWaitingJobs = stringtoi((std::string)GLUEService["LocalSuspendedJobs"]);
      //}


      logger.msg(VERBOSE, "Generating A-REX target: %s", cs->Cluster.str());

      int endpointID = 0;
      for(XMLNode xmlCENode = GLUEService["ComputingEndpoint"]; (bool)xmlCENode; ++xmlCENode) {
        if ((xmlCENode["InterfaceName"] == "XBES") ||
            (xmlCENode["InterfaceName"] == "BES") ||
            (xmlCENode["Interface"] == "XBES") ||
            (xmlCENode["Interface"] == "BES") ||
            (xmlCENode["InterfaceName"] == "org.ogf.bes") ||
            (xmlCENode["InterfaceName"] == "ogf.bes")) {
          // support for previous A-REX version, and fixing the InterfaceName
          xmlCENode["InterfaceName"] = "org.ogf.bes";
        }
        for (XMLNode n = xmlCENode["InterfaceExtension"]; n; ++n) {
          if ((std::string)n == "http://www.nordugrid.org/schemas/a-rex") {
            // support for previous A-REX version, and fixing the InterfaceExtension
            n = "urn:org.nordugrid.xbes";
          }
        }

        ComputingEndpointType ComputingEndpoint;

        if (xmlCENode["HealthState"]) {
          ComputingEndpoint->HealthState = (std::string)xmlCENode["HealthState"];
        } else {
          logger.msg(VERBOSE, "The Service advertises no Health State.");
        }
        if (xmlCENode["HealthStateInfo"]) {
          ComputingEndpoint->HealthStateInfo = (std::string)xmlCENode["HealthStateInfo"];
        }
        if (GLUEService["Name"]) {
          cs->Name = (std::string)GLUEService["Name"];
        }
        if (xmlCENode["Capability"]) {
          for (XMLNode n = xmlCENode["Capability"]; n; ++n) {
            ComputingEndpoint->Capability.push_back((std::string)n);
          }
        }
        if (xmlCENode["QualityLevel"]) {
          ComputingEndpoint->QualityLevel = (std::string)xmlCENode["QualityLevel"];
        }
        if (xmlCENode["Technology"]) {
          ComputingEndpoint->Technology = (std::string)xmlCENode["Technology"];
        }
        if (xmlCENode["InterfaceName"]) {
          ComputingEndpoint->InterfaceName = (std::string)xmlCENode["InterfaceName"];
        } else if (xmlCENode["Interface"]) { // No such attribute according to GLUE2 document. Legacy/backward compatibility?
          ComputingEndpoint->InterfaceName = (std::string)xmlCENode["Interface"];
        } else {
          logger.msg(VERBOSE, "The Service doesn't advertise its Interface.");
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
          logger.msg(VERBOSE, "The Service doesn't advertise its Serving State.");
        }
        if (xmlCENode["IssuerCA"]) {
          ComputingEndpoint->IssuerCA = (std::string)xmlCENode["IssuerCA"];
        }
        if (xmlCENode["TrustedCA"]) {
          XMLNode n = xmlCENode["TrustedCA"];
          while (n) {
            ComputingEndpoint->TrustedCA.push_back((std::string)n);
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

      int shareID = 0;
      for (XMLNode xmlCSNode = GLUEService["ComputingShare"]; (bool)xmlCSNode; ++xmlCSNode) {
        ComputingShareType ComputingShare;

        if (xmlCSNode["FreeSlots"]) {
          ComputingShare->FreeSlots = stringtoi((std::string)xmlCSNode["FreeSlots"]);
        }
        if (xmlCSNode["FreeSlotsWithDuration"]) {
          // Format: ns[:t] [ns[:t]]..., where ns is number of slots and t is the duration.
          ComputingShare->FreeSlotsWithDuration.clear();

          const std::string fswdValue = (std::string)xmlCSNode["FreeSlotsWithDuration"];
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

            ComputingShare->FreeSlotsWithDuration[Period(duration)] = freeSlots;
          }
        }
        if (xmlCSNode["UsedSlots"]) {
          ComputingShare->UsedSlots = stringtoi((std::string)xmlCSNode["UsedSlots"]);
        }
        if (xmlCSNode["RequestedSlots"]) {
          ComputingShare->RequestedSlots = stringtoi((std::string)xmlCSNode["RequestedSlots"]);
        }
        if (xmlCSNode["Name"]) {
          ComputingShare->Name = (std::string)xmlCSNode["Name"];
        }
        if (xmlCSNode["MaxWallTime"]) {
          ComputingShare->MaxWallTime = (std::string)xmlCSNode["MaxWallTime"];
        }
        if (xmlCSNode["MaxTotalWallTime"]) {
          ComputingShare->MaxTotalWallTime = (std::string)xmlCSNode["MaxTotalWallTime"];
        }
        if (xmlCSNode["MinWallTime"]) {
          ComputingShare->MinWallTime = (std::string)xmlCSNode["MinWallTime"];
        }
        if (xmlCSNode["DefaultWallTime"]) {
          ComputingShare->DefaultWallTime = (std::string)xmlCSNode["DefaultWallTime"];
        }
        if (xmlCSNode["MaxCPUTime"]) {
          ComputingShare->MaxCPUTime = (std::string)xmlCSNode["MaxCPUTime"];
        }
        if (xmlCSNode["MaxTotalCPUTime"]) {
          ComputingShare->MaxTotalCPUTime = (std::string)xmlCSNode["MaxTotalCPUTime"];
        }
        if (xmlCSNode["MinCPUTime"]) {
          ComputingShare->MinCPUTime = (std::string)xmlCSNode["MinCPUTime"];
        }
        if (xmlCSNode["DefaultCPUTime"]) {
          ComputingShare->DefaultCPUTime = (std::string)xmlCSNode["DefaultCPUTime"];
        }
        if (xmlCSNode["MaxTotalJobs"]) {
          ComputingShare->MaxTotalJobs = stringtoi((std::string)xmlCSNode["MaxTotalJobs"]);
        }
        if (xmlCSNode["MaxRunningJobs"]) {
          ComputingShare->MaxRunningJobs = stringtoi((std::string)xmlCSNode["MaxRunningJobs"]);
        }
        if (xmlCSNode["MaxWaitingJobs"]) {
          ComputingShare->MaxWaitingJobs = stringtoi((std::string)xmlCSNode["MaxWaitingJobs"]);
        }
        if (xmlCSNode["MaxPreLRMSWaitingJobs"]) {
          ComputingShare->MaxPreLRMSWaitingJobs = stringtoi((std::string)xmlCSNode["MaxPreLRMSWaitingJobs"]);
        }
        if (xmlCSNode["MaxUserRunningJobs"]) {
          ComputingShare->MaxUserRunningJobs = stringtoi((std::string)xmlCSNode["MaxUserRunningJobs"]);
        }
        if (xmlCSNode["MaxSlotsPerJob"]) {
          ComputingShare->MaxSlotsPerJob = stringtoi((std::string)xmlCSNode["MaxSlotsPerJob"]);
        }
        if (xmlCSNode["MaxStageInStreams"]) {
          ComputingShare->MaxStageInStreams = stringtoi((std::string)xmlCSNode["MaxStageInStreams"]);
        }
        if (xmlCSNode["MaxStageOutStreams"]) {
          ComputingShare->MaxStageOutStreams = stringtoi((std::string)xmlCSNode["MaxStageOutStreams"]);
        }
        if (xmlCSNode["SchedulingPolicy"]) {
          ComputingShare->SchedulingPolicy = (std::string)xmlCSNode["SchedulingPolicy"];
        }
        if (xmlCSNode["MaxMainMemory"]) {
          ComputingShare->MaxMainMemory = stringtoi((std::string)xmlCSNode["MaxMainMemory"]);
        }
        if (xmlCSNode["MaxVirtualMemory"]) {
          ComputingShare->MaxVirtualMemory = stringtoi((std::string)xmlCSNode["MaxVirtualMemory"]);
        }
        if (xmlCSNode["MaxDiskSpace"]) {
          ComputingShare->MaxDiskSpace = stringtoi((std::string)xmlCSNode["MaxDiskSpace"]);
        }
        if (xmlCSNode["DefaultStorageService"]) {
          ComputingShare->DefaultStorageService = (std::string)xmlCSNode["DefaultStorageService"];
        }
        if (xmlCSNode["Preemption"]) {
          ComputingShare->Preemption = ((std::string)xmlCSNode["Preemption"] == "true") ? true : false;
        }
        if (xmlCSNode["EstimatedAverageWaitingTime"]) {
          ComputingShare->EstimatedAverageWaitingTime = (std::string)xmlCSNode["EstimatedAverageWaitingTime"];
        }
        if (xmlCSNode["EstimatedWorstWaitingTime"]) {
          ComputingShare->EstimatedWorstWaitingTime = stringtoi((std::string)xmlCSNode["EstimatedWorstWaitingTime"]);
        }
        if (xmlCSNode["ReservationPolicy"]) {
          ComputingShare->ReservationPolicy = stringtoi((std::string)xmlCSNode["ReservationPolicy"]);
        }

        cs.ComputingShare.insert(std::pair<int, ComputingShareType>(shareID++, ComputingShare));
      }

      int managerID = 0;
      for (XMLNode xmlCMNode = GLUEService["ComputingManager"]; (bool)xmlCMNode; ++xmlCMNode) {
        ComputingManagerType ComputingManager;

        if (xmlCMNode["ProductName"]) {
          ComputingManager->ProductName = (std::string)xmlCMNode["ProductName"];
        } else if (xmlCMNode["Type"]) { // is this non-standard fallback needed?
          ComputingManager->ProductName = (std::string)xmlCMNode["Type"];
        }
        if (xmlCMNode["ProductVersion"]) {
          ComputingManager->ProductVersion = (std::string)xmlCMNode["ProductVersion"];
        }
        if (xmlCMNode["Reservation"]) {
          ComputingManager->Reservation = ((std::string)xmlCMNode["Reservation"] == "true") ? true : false;
        }
        if (xmlCMNode["BulkSubmission"]) {
          ComputingManager->BulkSubmission = ((std::string)xmlCMNode["BulkSubmission"] == "true") ? true : false;
        }
        if (xmlCMNode["TotalPhysicalCPUs"]) {
          ComputingManager->TotalPhysicalCPUs = stringtoi((std::string)xmlCMNode["TotalPhysicalCPUs"]);
        }
        if (xmlCMNode["TotalLogicalCPUs"]) {
          ComputingManager->TotalLogicalCPUs = stringtoi((std::string)xmlCMNode["TotalLogicalCPUs"]);
        }
        if (xmlCMNode["TotalSlots"]) {
          ComputingManager->TotalSlots = stringtoi((std::string)xmlCMNode["TotalSlots"]);
        }
        if (xmlCMNode["Homogeneous"]) {
          ComputingManager->Homogeneous = ((std::string)xmlCMNode["Homogeneous"] == "true") ? true : false;
        }
        if (xmlCMNode["NetworkInfo"]) {
          for (XMLNode n = xmlCMNode["NetworkInfo"]; n; ++n) {
            ComputingManager->NetworkInfo.push_back((std::string)n);
          }
        }
        if (xmlCMNode["WorkingAreaShared"]) {
          ComputingManager->WorkingAreaShared = ((std::string)xmlCMNode["WorkingAreaShared"] == "true") ? true : false;
        }
        if (xmlCMNode["WorkingAreaFree"]) {
          ComputingManager->WorkingAreaFree = stringtoi((std::string)xmlCMNode["WorkingAreaFree"]);
        }
        if (xmlCMNode["WorkingAreaTotal"]) {
          ComputingManager->WorkingAreaTotal = stringtoi((std::string)xmlCMNode["WorkingAreaTotal"]);
        }
        if (xmlCMNode["WorkingAreaLifeTime"]) {
          ComputingManager->WorkingAreaLifeTime = (std::string)xmlCMNode["WorkingAreaLifeTime"];
        }
        if (xmlCMNode["CacheFree"]) {
          ComputingManager->CacheFree = stringtoi((std::string)xmlCMNode["CacheFree"]);
        }
        if (xmlCMNode["CacheTotal"]) {
          ComputingManager->CacheTotal = stringtoi((std::string)xmlCMNode["CacheTotal"]);
        }
        for (XMLNode n = xmlCMNode["Benchmark"]; n; ++n) {
          double value;
          if (n["Type"] && n["Value"] &&
              stringto((std::string)n["Value"], value)) {
            (*ComputingManager.Benchmarks)[(std::string)n["Type"]] = value;
          } else {
            logger.msg(VERBOSE, "Couldn't parse benchmark XML:\n%s", (std::string)n);
            continue;
          }
        }
        for (XMLNode n = xmlCMNode["ApplicationEnvironments"]["ApplicationEnvironment"]; n; ++n) {
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

        /*
         * A ComputingShare is linked to multiple ExecutionEnvironments.
         * Due to bug 2101 multiple ExecutionEnvironments per ComputingShare
         * will be ignored. The ExecutionEnvironment information will only be
         * stored if there is one ExecutionEnvironment associated with a
         * ComputingShare.
         */
        int eeID = 0;
        for (XMLNode xmlEENode = xmlCMNode["ExecutionEnvironments"]["ExecutionEnvironment"]; (bool)xmlEENode; ++xmlEENode) {
          ExecutionEnvironmentType ExecutionEnvironment;
          if (xmlEENode["Platform"]) {
            ExecutionEnvironment->Platform = (std::string)xmlEENode["Platform"];
          }

          if (xmlEENode["MainMemorySize"]) {
            ExecutionEnvironment->MainMemorySize = stringtoi((std::string)xmlEENode["MainMemorySize"]);
          }

          if (xmlEENode["OSName"]) {
            if (xmlEENode["OSVersion"]) {
              if (xmlEENode["OSFamily"]) {
                ExecutionEnvironment->OperatingSystem = Software((std::string)xmlEENode["OSFamily"],
                                                                              (std::string)xmlEENode["OSName"],
                                                                              (std::string)xmlEENode["OSVersion"]);
              }
              else {
                ExecutionEnvironment->OperatingSystem = Software((std::string)xmlEENode["OSName"],
                                                                              (std::string)xmlEENode["OSVersion"]);
              }
            }
            else {
              ExecutionEnvironment->OperatingSystem = Software((std::string)xmlEENode["OSName"]);
            }
          }

          if (xmlEENode["ConnectivityIn"]) {
            ExecutionEnvironment->ConnectivityIn = (lower((std::string)xmlEENode["ConnectivityIn"]) == "true");
          }

          if (xmlEENode["ConnectivityOut"]) {
            ExecutionEnvironment->ConnectivityOut = (lower((std::string)xmlEENode["ConnectivityOut"]) == "true");
          }

          ComputingManager.ExecutionEnvironment.insert(std::pair<int, ExecutionEnvironmentType>(eeID++, ExecutionEnvironment));
        }

        cs.ComputingManager.insert(std::pair<int, ComputingManagerType>(managerID++, ComputingManager));
      }

      csList.push_back(cs);
    }
  }

} // namespace Arc
