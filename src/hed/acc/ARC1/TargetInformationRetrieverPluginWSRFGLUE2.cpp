// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/Logger.h>
#include <arc/StringConv.h>
#include <arc/URL.h>
#include <arc/UserConfig.h>
#include <arc/XMLNode.h>
#include <arc/compute/EndpointQueryingStatus.h>
#include <arc/compute/ExecutionTarget.h>
#include <arc/message/MCC.h>

#include "JobStateBES.h"
#include "JobStateARC1.h"
#include "AREXClient.h"
#include "TargetInformationRetrieverPluginWSRFGLUE2.h"

namespace Arc {

  Logger TargetInformationRetrieverPluginWSRFGLUE2::logger(Logger::getRootLogger(), "TargetInformationRetrieverPlugin.WSRFGLUE2");

  EndpointQueryingStatus TargetInformationRetrieverPluginWSRFGLUE2::Query(const UserConfig& uc, const Endpoint& cie, std::list<ComputingServiceType>& csList, const EndpointQueryOptions<ComputingServiceType>&) const {
    logger.msg(DEBUG, "Querying WSRF GLUE2 computing info endpoint.");

    URL url(CreateURL(cie.URLString));
    if (!url) {
      return EndpointQueryingStatus(EndpointQueryingStatus::FAILED,"URL "+cie.URLString+" can't be processed");
    }

    MCCConfig cfg;
    uc.ApplyToConfig(cfg);
    AREXClient ac(url, cfg, uc.Timeout(), true); // Temporary
    //AREXClient ac(url, cfg, uc.Timeout(), /* thrarg->flavour == "ARC1" */); // TIR equivalent
    XMLNode servicesQueryResponse;
    if (!ac.sstat(servicesQueryResponse)) {
      return EndpointQueryingStatus(EndpointQueryingStatus::FAILED,ac.failure());
    }

    ExtractTargets(url, servicesQueryResponse, csList);
    
    for (std::list<ComputingServiceType>::iterator it = csList.begin(); it != csList.end(); it++) {
      (*it)->InformationOriginEndpoint = cie;
    }

    if (!csList.empty()) return EndpointQueryingStatus(EndpointQueryingStatus::SUCCESSFUL);
    return EndpointQueryingStatus(EndpointQueryingStatus::FAILED,"Query returned no endpoints");
  }

  void TargetInformationRetrieverPluginWSRFGLUE2::ExtractTargets(const URL& url, XMLNode response, std::list<ComputingServiceType>& csList) {
    /*
     * A-REX will not return multiple ComputingService elements, but if the
     * response comes from an index server then there might be multiple.
     */
    for (XMLNode GLUEService = response["ComputingService"]; GLUEService; ++GLUEService) {
      ComputingServiceType cs;
      AdminDomainType& AdminDomain = cs.AdminDomain;

      AdminDomain->Name = url.Host();

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
        logger.msg(VERBOSE, "The Service doesn't advertise its Quality Level.");
      }
      EntryToInt(url, GLUEService["TotalJobs"], cs->TotalJobs);
      EntryToInt(url, GLUEService["RunningJobs"], cs->RunningJobs);
      EntryToInt(url, GLUEService["WaitingJobs"], cs->WaitingJobs);
      EntryToInt(url, GLUEService["PreLRMSWaitingJobs"], cs->PreLRMSWaitingJobs);
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


      logger.msg(VERBOSE, "Generating A-REX target: %s", AdminDomain->Name);

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
        if (GLUEService["ID"]) {
          cs->ID = (std::string)GLUEService["ID"];
        }
        if (GLUEService["Name"]) {
          cs->Name = (std::string)GLUEService["Name"];
        }
        if (xmlCENode["Capability"]) {
          for (XMLNode n = xmlCENode["Capability"]; n; ++n) {
            ComputingEndpoint->Capability.insert((std::string)n);
          }
        }
        if (xmlCENode["QualityLevel"]) {
          ComputingEndpoint->QualityLevel = (std::string)xmlCENode["QualityLevel"];
        }
        if (xmlCENode["Technology"]) {
          ComputingEndpoint->Technology = (std::string)xmlCENode["Technology"];
        }
        if (xmlCENode["InterfaceName"]) {
          ComputingEndpoint->InterfaceName = lower((std::string)xmlCENode["InterfaceName"]);
        } else if (xmlCENode["Interface"]) { // No such attribute according to GLUE2 document. Legacy/backward compatibility?
          ComputingEndpoint->InterfaceName = lower((std::string)xmlCENode["Interface"]);
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
        EntryToInt(url, xmlCENode["TotalJobs"], ComputingEndpoint->TotalJobs);
        EntryToInt(url, xmlCENode["RunningJobs"], ComputingEndpoint->RunningJobs);
        EntryToInt(url, xmlCENode["WaitingJobs"], ComputingEndpoint->WaitingJobs);
        EntryToInt(url, xmlCENode["StagingJobs"], ComputingEndpoint->StagingJobs);
        EntryToInt(url, xmlCENode["SuspendedJobs"], ComputingEndpoint->SuspendedJobs);
        EntryToInt(url, xmlCENode["PreLRMSWaitingJobs"], ComputingEndpoint->PreLRMSWaitingJobs);
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

        EntryToInt(url, xmlCSNode["FreeSlots"], ComputingShare->FreeSlots);
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
            if (fswdPair.size() > 2 || !Arc::stringto(fswdPair.front(), freeSlots) || (fswdPair.size() == 2 && !Arc::stringto(fswdPair.back(), duration))) {
              logger.msg(VERBOSE, "The \"FreeSlotsWithDuration\" attribute published by \"%s\" is wrongly formatted. Ignoring it.");
              logger.msg(DEBUG, "Wrong format of the \"FreeSlotsWithDuration\" = \"%s\" (\"%s\")", fswdValue, *it);
              continue;
            }

            ComputingShare->FreeSlotsWithDuration[Period(duration)] = freeSlots;
          }
        }
        EntryToInt(url, xmlCSNode["UsedSlots"], ComputingShare->UsedSlots);
        EntryToInt(url, xmlCSNode["RequestedSlots"], ComputingShare->RequestedSlots);
        if (xmlCSNode["Name"]) {
          ComputingShare->Name = (std::string)xmlCSNode["Name"];
        }
        if (xmlCSNode["MappingQueue"]) {
          ComputingShare->MappingQueue = (std::string)xmlCSNode["MappingQueue"];
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
        EntryToInt(url, xmlCSNode["MaxTotalJobs"], ComputingShare->MaxTotalJobs);
        EntryToInt(url, xmlCSNode["MaxRunningJobs"], ComputingShare->MaxRunningJobs);
        EntryToInt(url, xmlCSNode["MaxWaitingJobs"], ComputingShare->MaxWaitingJobs);
        EntryToInt(url, xmlCSNode["WaitingJobs"], ComputingShare->WaitingJobs);
        EntryToInt(url, xmlCSNode["MaxPreLRMSWaitingJobs"], ComputingShare->MaxPreLRMSWaitingJobs);
        EntryToInt(url, xmlCSNode["MaxUserRunningJobs"], ComputingShare->MaxUserRunningJobs);
        EntryToInt(url, xmlCSNode["MaxSlotsPerJob"], ComputingShare->MaxSlotsPerJob);
        EntryToInt(url, xmlCSNode["MaxStageInStreams"], ComputingShare->MaxStageInStreams);
        EntryToInt(url, xmlCSNode["MaxStageOutStreams"], ComputingShare->MaxStageOutStreams);
        if (xmlCSNode["SchedulingPolicy"]) {
          ComputingShare->SchedulingPolicy = (std::string)xmlCSNode["SchedulingPolicy"];
        }
        EntryToInt(url, xmlCSNode["MaxMainMemory"], ComputingShare->MaxMainMemory);
        EntryToInt(url, xmlCSNode["MaxVirtualMemory"], ComputingShare->MaxVirtualMemory);
        EntryToInt(url, xmlCSNode["MaxDiskSpace"], ComputingShare->MaxDiskSpace);
        if (xmlCSNode["DefaultStorageService"]) {
          ComputingShare->DefaultStorageService = (std::string)xmlCSNode["DefaultStorageService"];
        }
        if (xmlCSNode["Preemption"]) {
          ComputingShare->Preemption = ((std::string)xmlCSNode["Preemption"] == "true") ? true : false;
        }
        if (xmlCSNode["EstimatedAverageWaitingTime"]) {
          ComputingShare->EstimatedAverageWaitingTime = (std::string)xmlCSNode["EstimatedAverageWaitingTime"];
        }
        int EstimatedWorstWaitingTime;
        if (EntryToInt(url, xmlCSNode["EstimatedWorstWaitingTime"], EstimatedWorstWaitingTime)) {
          ComputingShare->EstimatedWorstWaitingTime = EstimatedWorstWaitingTime;
        }
        if (xmlCSNode["ReservationPolicy"]) {
          ComputingShare->ReservationPolicy = (std::string)xmlCSNode["ReservationPolicy"];
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
          ComputingManager->Reservation = ((std::string)xmlCMNode["Reservation"] == "true");
        }
        if (xmlCMNode["BulkSubmission"]) {
          ComputingManager->BulkSubmission = ((std::string)xmlCMNode["BulkSubmission"] == "true");
        }
        EntryToInt(url, xmlCMNode["TotalPhysicalCPUs"], ComputingManager->TotalPhysicalCPUs);
        EntryToInt(url, xmlCMNode["TotalLogicalCPUs"], ComputingManager->TotalLogicalCPUs);
        EntryToInt(url, xmlCMNode["TotalSlots"], ComputingManager->TotalSlots);
        if (xmlCMNode["Homogeneous"]) {
          ComputingManager->Homogeneous = ((std::string)xmlCMNode["Homogeneous"] == "true");
        }
        if (xmlCMNode["NetworkInfo"]) {
          for (XMLNode n = xmlCMNode["NetworkInfo"]; n; ++n) {
            ComputingManager->NetworkInfo.push_back((std::string)n);
          }
        }
        if (xmlCMNode["WorkingAreaShared"]) {
          ComputingManager->WorkingAreaShared = ((std::string)xmlCMNode["WorkingAreaShared"] == "true");
        }
        EntryToInt(url, xmlCMNode["WorkingAreaFree"], ComputingManager->WorkingAreaFree);
        EntryToInt(url, xmlCMNode["WorkingAreaTotal"], ComputingManager->WorkingAreaTotal);
        if (xmlCMNode["WorkingAreaLifeTime"]) {
          ComputingManager->WorkingAreaLifeTime = (std::string)xmlCMNode["WorkingAreaLifeTime"];
        }
        EntryToInt(url, xmlCMNode["CacheFree"], ComputingManager->CacheFree);
        EntryToInt(url, xmlCMNode["CacheTotal"], ComputingManager->CacheTotal);
        for (XMLNode n = xmlCMNode["Benchmark"]; n; ++n) {
          double value;
          if (n["Type"] && n["Value"] &&
              Arc::stringto((std::string)n["Value"], value)) {
            (*ComputingManager.Benchmarks)[(std::string)n["Type"]] = value;
          } else {
            logger.msg(VERBOSE, "Couldn't parse benchmark XML:\n%s", (std::string)n);
            continue;
          }
        }
        for (XMLNode n = xmlCMNode["ApplicationEnvironments"]["ApplicationEnvironment"]; n; ++n) {
          ApplicationEnvironment ae((std::string)n["AppName"], (std::string)n["AppVersion"]);
          ae.State = (std::string)n["State"];
          EntryToInt(url, n["FreeSlots"], ae.FreeSlots);
          //else {
          //  ae.FreeSlots = ComputingShare->FreeSlots; // Non compatible??, i.e. a ComputingShare is unrelated to the ApplicationEnvironment.
          //}
          EntryToInt(url, n["FreeJobs"], ae.FreeJobs);
          EntryToInt(url, n["FreeUserSeats"], ae.FreeUserSeats);
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

          EntryToInt(url, xmlEENode["MainMemorySize"], ExecutionEnvironment->MainMemorySize);

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

  bool TargetInformationRetrieverPluginWSRFGLUE2::EntryToInt(const URL& url, XMLNode entry, int& i) {
    if (entry && !stringto((std::string)entry, i)) {
      logger.msg(INFO, "Unable to parse the %s.%s value from execution service (%s).", entry.Parent().Name(), entry.Name(), url.fullstr());
      logger.msg(DEBUG, "Value of %s.%s is \"%s\"", entry.Parent().Name(), entry.Name(), (std::string)entry);
      return false;
    }
    return (bool)entry;
  }

} // namespace Arc
