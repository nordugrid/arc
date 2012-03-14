// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/Logger.h>
#include <arc/StringConv.h>
#include <arc/UserConfig.h>
#include <arc/URL.h>
#include <arc/client/ExecutionTarget.h>
#include <arc/client/EndpointQueryingStatus.h>
#include <arc/data/DataBuffer.h>
#include <arc/data/DataHandle.h>

#include "TargetInformationRetrieverPluginLDAPGLUE1.h"

namespace Arc {

  Logger TargetInformationRetrieverPluginLDAPGLUE1::logger(Logger::getRootLogger(), "TargetInformationRetrieverPlugin.LDAPGLUE1");

  bool TargetInformationRetrieverPluginLDAPGLUE1::isEndpointNotSupported(const Endpoint& endpoint) const {
    const std::string::size_type pos = endpoint.URLString.find("://");
    return pos != std::string::npos && lower(endpoint.URLString.substr(0, pos)) != "ldap";
  }

  static URL CreateURL(std::string service) {
    std::string::size_type pos1 = service.find("://");
    if (pos1 == std::string::npos) {
      service = "ldap://" + service;
      pos1 = 4;
    } else {
      if(lower(service.substr(0,pos1)) != "ldap") return URL();
    }
    std::string::size_type pos2 = service.find(":", pos1 + 3);
    std::string::size_type pos3 = service.find("/", pos1 + 3);
    if (pos3 == std::string::npos) {
      if (pos2 == std::string::npos)
        service += ":2170";
      // Is this a good default path?
      service += "/o=Grid";
    }
    else if (pos2 == std::string::npos || pos2 > pos3)
      service.insert(pos3, ":2170");

    return service;
  }

  EndpointQueryingStatus TargetInformationRetrieverPluginLDAPGLUE1::Query(const UserConfig& uc, const Endpoint& cie, std::list<ComputingServiceType>& csList, const EndpointQueryOptions<ComputingServiceType>&) const {
    EndpointQueryingStatus s(EndpointQueryingStatus::FAILED);

    URL url(CreateURL(cie.URLString));
    url.ChangeLDAPScope(URL::subtree);

    if (!url) {
      return s;
    }

    DataHandle handler(url, uc);
    DataBuffer buffer;

    if (!handler) {
      logger.msg(INFO, "Can't create information handle - "
                 "is the ARC ldap DMC plugin available?");
      return s;
    }

    if (!handler->StartReading(buffer)) {
      return s;
    }

    int handle;
    unsigned int length;
    unsigned long long int offset;
    std::string result;

    while (buffer.for_write() || !buffer.eof_read())
      if (buffer.for_write(handle, length, offset, true)) {
        result.append(buffer[handle], length);
        buffer.is_written(handle);
      }

    if (!handler->StopReading()) {
      return s;
    }

    XMLNode XMLresult(result);

    // Create one ExecutionTarget per VOView record.
    // Consider using XMLNode::XPath for increased performance
    std::list<XMLNode> VOViews = XMLresult.XPathLookup("//*[objectClass='GlueVOView']", NS());
    for (std::list<XMLNode>::iterator it = VOViews.begin();
         it != VOViews.end(); it++) {
      XMLNode VOView(*it);

      ComputingServiceType cs;
      AdminDomainType& AdminDomain = cs.AdminDomain;
      LocationType& Location = cs.Location;

      cs->Cluster = url;

      std::string key;
      std::string::size_type pos = std::string::npos;

      for (XMLNode node = VOView["GlueChunkKey"]; node; ++node) {
        key = (std::string)node;
        pos = key.find('=');
        if (key.substr(0, pos) == "GlueCEUniqueID")
          break;
      }

      XMLNode CE =
        *XMLresult.XPathLookup("//*[GlueCEUniqueID='" +
                               key.substr(pos + 1) + "']", NS()).begin();

      for (XMLNode node = CE["GlueForeignKey"]; node; ++node) {
        key = (std::string)node;
        pos = key.find('=');
        if (key.substr(0, pos) == "GlueClusterUniqueID")
          break;
      }

      // Consider using XMLNode::XPath for increased performance
      XMLNode Cluster = *XMLresult.XPathLookup("//*[objectClass='GlueCluster']"
                                               "[GlueClusterUniqueID='" +
                                               key.substr(pos + 1) + "']", NS()).begin();

      // Consider using XMLNode::XPath for increased performance
      // TODO: What to do if a cluster has more than one subcluster??? Map into multiple ExecutionEnvironment objects
      XMLNode SubCluster =
        *XMLresult.XPathLookup("//*[objectClass='GlueSubCluster']"
                               "[GlueChunkKey='" + key + "']", NS()).begin();

      for (XMLNode node = Cluster["GlueForeignKey"]; node; ++node) {
        key = (std::string)node;
        pos = key.find('=');
        if (key.substr(0, pos) == "GlueSiteUniqueID")
          break;
      }

      // Consider using XMLNode::XPath for increased performance
      XMLNode Site =
        *XMLresult.XPathLookup("//*[objectClass='GlueSite']"
                               "[GlueSiteUniqueID='" +
                               key.substr(pos + 1) + "']", NS()).begin();

      /* These are the available attributes:

         VOView["GlueVOViewLocalID"];
         VOView["GlueCEAccessControlBaseRule"]; // multi
         VOView["GlueCEStateRunningJobs"];
         VOView["GlueCEStateWaitingJobs"];
         VOView["GlueCEStateTotalJobs"];
         VOView["GlueCEStateFreeJobSlots"];
         VOView["GlueCEStateEstimatedResponseTime"];
         VOView["GlueCEStateWorstResponseTime"];
         VOView["GlueCEInfoDefaultSE"];
         VOView["GlueCEInfoApplicationDir"];
         VOView["GlueCEInfoDataDir"];
         VOView["GlueChunkKey"];
         VOView["GlueSchemaVersionMajor"];
         VOView["GlueSchemaVersionMinor"];

         CE["GlueCEHostingCluster"];
         CE["GlueCEName"];
         CE["GlueCEUniqueID"];
         CE["GlueCEImplementationName"];
         CE["GlueCEImplementationVersion"];
         CE["GlueCECapability"]; // multi
         CE["GlueCEInfoGatekeeperPort"];
         CE["GlueCEInfoHostName"];
         CE["GlueCEInfoLRMSType"];
         CE["GlueCEInfoLRMSVersion"];
         CE["GlueCEInfoJobManager"];
         CE["GlueCEInfoContactString"]; // multi
         CE["GlueCEInfoApplicationDir"];
         CE["GlueCEInfoDataDir"];
         CE["GlueCEInfoDefaultSE"];
         CE["GlueCEInfoTotalCPUs"];
         CE["GlueCEStateEstimatedResponseTime"];
         CE["GlueCEStateRunningJobs"];
         CE["GlueCEStateStatus"];
         CE["GlueCEStateTotalJobs"];
         CE["GlueCEStateWaitingJobs"];
         CE["GlueCEStateWorstResponseTime"];
         CE["GlueCEStateFreeJobSlots"];
         CE["GlueCEStateFreeCPUs"];
         CE["GlueCEPolicyMaxCPUTime"];
         CE["GlueCEPolicyMaxObtainableCPUTime"];
         CE["GlueCEPolicyMaxRunningJobs"];
         CE["GlueCEPolicyMaxWaitingJobs"];
         CE["GlueCEPolicyMaxTotalJobs"];
         CE["GlueCEPolicyMaxWallClockTime"];
         CE["GlueCEPolicyMaxObtainableWallClockTime"];
         CE["GlueCEPolicyPriority"];
         CE["GlueCEPolicyAssignedJobSlots"];
         CE["GlueCEPolicyMaxSlotsPerJob"];
         CE["GlueCEPolicyPreemption"];
         CE["GlueCEAccessControlBaseRule"]; // multi
         CE["GlueForeignKey"];
         CE["GlueInformationServiceURL"];
         CE["GlueSchemaVersionMajor"];
         CE["GlueSchemaVersionMinor"];

         Cluster["GlueClusterName"];
         Cluster["GlueClusterService"];
         Cluster["GlueClusterUniqueID"];
         Cluster["GlueForeignKey"];
         Cluster["GlueInformationServiceURL"];
         Cluster["GlueSchemaVersionMajor"];
         Cluster["GlueSchemaVersionMinor"];

         SubCluster["GlueChunkKey"];
         SubCluster["GlueHostApplicationSoftwareRunTimeEnvironment"]; // multi
         SubCluster["GlueHostArchitectureSMPSize"];
         SubCluster["GlueHostArchitecturePlatformType"];
         SubCluster["GlueHostBenchmarkSF00"];
         SubCluster["GlueHostBenchmarkSI00"];
         SubCluster["GlueHostMainMemoryRAMSize"];
         SubCluster["GlueHostMainMemoryVirtualSize"];
         SubCluster["GlueHostNetworkAdapterInboundIP"];
         SubCluster["GlueHostNetworkAdapterOutboundIP"];
         SubCluster["GlueHostOperatingSystemName"];
         SubCluster["GlueHostOperatingSystemRelease"];
         SubCluster["GlueHostOperatingSystemVersion"];
         SubCluster["GlueHostProcessorClockSpeed"];
         SubCluster["GlueHostProcessorModel"];
         SubCluster["GlueHostProcessorVendor"];
         SubCluster["GlueSubClusterName"];
         SubCluster["GlueSubClusterUniqueID"];
         SubCluster["GlueSubClusterPhysicalCPUs"];
         SubCluster["GlueSubClusterLogicalCPUs"];
         SubCluster["GlueSubClusterTmpDir"];
         SubCluster["GlueSubClusterWNTmpDir"];
         SubCluster["GlueInformationServiceURL"];
         SubCluster["GlueSchemaVersionMajor"];
         SubCluster["GlueSchemaVersionMinor"];

         Site["GlueSiteUniqueID"];
         Site["GlueSiteName"];
         Site["GlueSiteDescription"];
         Site["GlueSiteEmailContact"];
         Site["GlueSiteUserSupportContact"];
         Site["GlueSiteSysAdminContact"];
         Site["GlueSiteSecurityContact"];
         Site["GlueSiteLocation"];
         Site["GlueSiteLatitude"];
         Site["GlueSiteLongitude"];
         Site["GlueSiteWeb"];
         Site["GlueSiteSponsor"];
         Site["GlueSiteOtherInfo"];
         Site["GlueSiteOtherInfo"];
         Site["GlueForeignKey"];
         Site["GlueSchemaVersionMajor"];
         Site["GlueSchemaVersionMinor"];

         ... now do the mapping */

      // TODO: Mapping to new GLUE2 class structure need to be done.
      /*

      // TODO: we need to somehow query the HealthState
      ComputingEndpoint->HealthState = "ok";

      if (CE["GlueCEName"])
        ComputingShare->Name = (std::string)CE["GlueCEName"];
      if (CE["GlueCEInfoLRMSType"])
        ComputingManager->ProductName = (std::string)CE["GlueCEInfoLRMSType"];
      if (CE["GlueCEInfoLRMSVersion"])
        ComputingManager->ProductVersion = (std::string)CE["GlueCEInfoLRMSVersion"];
      if (CE["GlueCEInfoJobManager"])
        ComputingShare->MappingQueue = (std::string)CE["GlueCEInfoJobManager"];
      if (Cluster["GlueClusterName"]) {
        cs->Name = (std::string)Cluster["GlueClusterName"];
      }
      if (Site["GlueSiteName"])
        AdminDomain->Name = (std::string)Site["GlueSiteName"];
      if (Site["GlueSiteLocation"])
        Location->Place = (std::string)Site["GlueSiteLocation"];
      if (Site["GlueSiteLatitude"])
        Location->Latitude = stringtof(Site["GlueSiteLatitude"]);
      if (Site["GlueSiteLongitude"])
        Location->Longitude = stringtof(Site["GlueSiteLongitude"]);
      if (CE["GlueCEInfoContactString"])
        ComputingEndpoint->URLString = (std::string)CE["GlueCEInfoContactString"];
      if (CE["GlueCEImplementationName"]) {
        if (CE["GlueCEImplementationVersion"])
          ComputingEndpoint->Implementation =
            Software((std::string)CE["GlueCEImplementationName"],
                     (std::string)CE["GlueCEImplementationVersion"]);
        else
          ComputingEndpoint->Implementation =
            (std::string)CE["GlueCEImplementationName"];
      }
      if (VOView["GlueCEStateTotalJobs"])
        ComputingShare->TotalJobs = stringtoi(VOView["GlueCEStateTotalJobs"]);
      else if (CE["GlueCEStateTotalJobs"])
        ComputingShare->TotalJobs = stringtoi(CE["GlueCEStateTotalJobs"]);
      if (VOView["GlueCEStateRunningJobs"])
        ComputingShare->RunningJobs = stringtoi(VOView["GlueCEStateRunningJobs"]);
      else if (CE["GlueCEStateRunningJobs"])
        ComputingShare->RunningJobs = stringtoi(CE["GlueCEStateRunningJobs"]);
      if (VOView["GlueCEStateWaitingJobs"])
        ComputingShare->WaitingJobs = stringtoi(VOView["GlueCEStateWaitingJobs"]);
      else if (CE["GlueCEStateWaitingJobs"])
        ComputingShare->WaitingJobs = stringtoi(CE["GlueCEStateWaitingJobs"]);

      // StagingJobs           - not available in schema
      // SuspendedJobs         - not available in schema
      // PreLRMSWaitingJobs    - not available in schema
      // ComputingShareName          - not available in schema

      if (VOView["GlueCEPolicyMaxWallClockTime"])
        ComputingShare->MaxWallTime = stringtoi(VOView["GlueCEPolicyMaxWallClockTime"]);
      else if (CE["GlueCEPolicyMaxWallClockTime"])
        ComputingShare->MaxWallTime = stringtoi(CE["GlueCEPolicyMaxWallClockTime"]);

      // MinWallTime           - not available in schema
      // DefaultWallTime       - not available in schema

      if (VOView["GlueCEPolicyMaxCPUTime"])
        ComputingShare->MaxCPUTime = stringtoi(VOView["GlueCEPolicyMaxCPUTime"]);
      else if (CE["GlueCEPolicyMaxCPUTime"])
        ComputingShare->MaxCPUTime = stringtoi(CE["GlueCEPolicyMaxCPUTime"]);

      // MinCPUTime            - not available in schema
      // DefaultCPUTime        - not available in schema

      if (VOView["GlueCEPolicyMaxTotalJobs"])
        ComputingShare->MaxTotalJobs = stringtoi(VOView["GlueCEPolicyMaxTotalJobs"]);
      else if (CE["GlueCEPolicyMaxTotalJobs"])
        ComputingShare->MaxTotalJobs = stringtoi(CE["GlueCEPolicyMaxTotalJobs"]);
      if (VOView["GlueCEPolicyMaxRunningJobs"])
        ComputingShare->MaxRunningJobs =
          stringtoi(VOView["GlueCEPolicyMaxRunningJobs"]);
      else if (CE["GlueCEPolicyMaxRunningJobs"])
        ComputingShare->MaxRunningJobs = stringtoi(CE["GlueCEPolicyMaxRunningJobs"]);
      if (VOView["GlueCEPolicyMaxWaitingJobs"])
        ComputingShare->MaxWaitingJobs =
          stringtoi(VOView["GlueCEPolicyMaxWaitingJobs"]);
      else if (CE["GlueCEPolicyMaxWaitingJobs"])
        ComputingShare->MaxWaitingJobs = stringtoi(CE["GlueCEPolicyMaxWaitingJobs"]);
      if (SubCluster["GlueHostMainMemoryRAMSize"])
        ComputingShare->MaxMainMemory = stringtoi(SubCluster["GlueHostMainMemoryRAMSize"]);

      // MaxPreLRMSWaitingJobs - not available in schema

      // is this correct ???
      if (VOView["GlueCEPolicyAssignedJobSlots"])
        ComputingShare->MaxUserRunningJobs =
          stringtoi(VOView["GlueCEPolicyAssignedJobSlots"]);
      else if (CE["GlueCEPolicyAssignedJobSlots"])
        ComputingShare->MaxUserRunningJobs =
          stringtoi(CE["GlueCEPolicyAssignedJobSlots"]);
      if (VOView["GlueCEPolicyMaxSlotsPerJob"])
        ComputingShare->MaxSlotsPerJob =
          stringtoi(VOView["GlueCEPolicyMaxSlotsPerJob"]);
      else if (CE["GlueCEPolicyMaxSlotsPerJob"])
        ComputingShare->MaxSlotsPerJob =
          stringtoi(CE["GlueCEPolicyMaxSlotsPerJob"]);

      // MaxStageInStreams     - not available in schema
      // MaxStageOutStreams    - not available in schema
      // SchedulingPolicy      - not available in schema

      if (SubCluster["GlueHostMainMemoryVirtualSize"])
        ComputingShare->MaxMainMemory =
          stringtoi(SubCluster["GlueHostMainMemoryVirtualSize"]);

      // MaxDiskSpace          - not available in schema

      if (VOView["GlueCEInfoDefaultSE"])
        ComputingShare->DefaultStorageService =
          (std::string)VOView["GlueCEInfoDefaultSE"];
      else if (CE["GlueCEInfoDefaultSE"])
        ComputingShare->DefaultStorageService = (std::string)CE["GlueCEInfoDefaultSE"];
      if (VOView["GlueCEPolicyPreemption"])
        ComputingShare->Preemption = stringtoi(VOView["GlueCEPolicyPreemption"]);
      else if (CE["GlueCEPolicyPreemption"])
        ComputingShare->Preemption = stringtoi(CE["GlueCEPolicyPreemption"]);
      if (VOView["GlueCEStateStatus"])
        ComputingEndpoint->ServingState = (std::string)VOView["GlueCEStateStatus"];
      else if (CE["GlueCEStateStatus"])
        ComputingEndpoint->ServingState = (std::string)CE["GlueCEStateStatus"];
      if (VOView["GlueCEStateEstimatedResponseTime"])
        ComputingShare->EstimatedAverageWaitingTime =
          stringtoi(VOView["GlueCEStateEstimatedResponseTime"]);
      else if (CE["GlueCEStateEstimatedResponseTime"])
        ComputingShare->EstimatedAverageWaitingTime =
          stringtoi(CE["GlueCEStateEstimatedResponseTime"]);
      if (VOView["GlueCEStateWorstResponseTime"])
        ComputingShare->EstimatedWorstWaitingTime =
          stringtoi(VOView["GlueCEStateWorstResponseTime"]);
      else if (CE["GlueCEStateWorstResponseTime"])
        ComputingShare->EstimatedWorstWaitingTime =
          stringtoi(CE["GlueCEStateWorstResponseTime"]);
      if (VOView["GlueCEStateFreeJobSlots"])
        ComputingShare->FreeSlots = stringtoi(VOView["GlueCEStateFreeJobSlots"]);
      else if (VOView["GlueCEStateFreeCPUs"])
        ComputingShare->FreeSlots = stringtoi(VOView["GlueCEStateFreeCPUs"]);
      else if (CE["GlueCEStateFreeJobSlots"])
        ComputingShare->FreeSlots = stringtoi(CE["GlueCEStateFreeJobSlots"]);
      else if (CE["GlueCEStateFreeCPUs"])
        ComputingShare->FreeSlots = stringtoi(CE["GlueCEStateFreeCPUs"]);

      // UsedSlots;
      // RequestedSlots;
      // ReservationPolicy;

      for (XMLNode node =
             SubCluster["GlueHostApplicationSoftwareRunTimeEnvironment"];
           node; ++node) {
        ApplicationEnvironment ae((std::string)node);
        ae.State = "UNDEFINEDVALUE";
        ae.FreeSlots = -1;
        ae.FreeUserSeats = -1;
        ae.FreeJobs = -1;
        ComputingManager.ApplicationEnvironments->push_back(ae);
      }

      csList.push_back(cs);
      */
    }

    if (!csList.empty()) s = EndpointQueryingStatus::SUCCESSFUL;
    return s;
  }

} // namespace Arc
