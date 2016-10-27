// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <utility>
#include <vector>

#include <arc/Logger.h>
#include <arc/StringConv.h>
#include <arc/UserConfig.h>
#include <arc/URL.h>
#include <arc/compute/ExecutionTarget.h>
#include <arc/compute/EndpointQueryingStatus.h>
#include <arc/credential/VOMSUtil.h>
#include <arc/data/DataBuffer.h>
#include <arc/data/DataHandle.h>

#include "Extractor.h"
#include "TargetInformationRetrieverPluginLDAPGLUE1.h"

namespace Arc {

  Logger TargetInformationRetrieverPluginLDAPGLUE1::logger(Logger::getRootLogger(), "TargetInformationRetrieverPlugin.LDAPGLUE1");

  bool TargetInformationRetrieverPluginLDAPGLUE1::isEndpointNotSupported(const Endpoint& endpoint) const {
    const std::string::size_type pos = endpoint.URLString.find("://");
    return pos != std::string::npos && lower(endpoint.URLString.substr(0, pos)) != "ldap";
  }

  EndpointQueryingStatus TargetInformationRetrieverPluginLDAPGLUE1::Query(const UserConfig& uc, const Endpoint& cie, std::list<ComputingServiceType>& csList, const EndpointQueryOptions<ComputingServiceType>&) const {
    EndpointQueryingStatus s(EndpointQueryingStatus::FAILED);

    if (isEndpointNotSupported(cie)) {
      return s;
    }
    URL url((cie.URLString.find("://") == std::string::npos ? "ldap://" : "") + cie.URLString, false, 2170, "/Mds-Vo-name=resource,o=grid");
    url.ChangeLDAPScope(URL::subtree);

    if (!url) {
      return s;
    }

    DataBuffer buffer;
    DataHandle handler(url, uc);

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

    XMLNode xmlResult(result);
    XMLNodeList glueServices = xmlResult.Path("o/Mds-Vo-name/GlueServiceUniqueID");
    for (XMLNodeList::iterator itS = glueServices.begin(); itS != glueServices.end(); ++itS) {
      // Currently only consider CREAM services.
      if (lower((std::string)(*itS)["GlueServiceType"]) != "org.glite.ce.cream") continue;
      
      // Only consider the first 'GlueClusterUniqueID' entry - possibly there is only one.
      XMLNode glueCluster = xmlResult["o"]["Mds-Vo-name"]["GlueClusterUniqueID"];
      if (!glueCluster) continue;
      
      XMLNode glueCE;
      // Find associated CE node.
      for (XMLNode n = glueCluster["GlueForeignKey"]; n; ++n) {
        std::string glueForeignKey = (std::string)n;
        if (glueForeignKey.substr(0, 14) == "GlueCEUniqueID") {
          for (XMLNode m = xmlResult["o"]["Mds-Vo-name"]["GlueCEUniqueID"]; m; ++m) {
            if ((std::string)m["GlueCEUniqueID"] == glueForeignKey.substr(15)) {
              glueCE = m;
              break;
            }
          }
        }
      }
      
      if (!glueCE) continue;
      
      Extractor site(xmlResult["o"]["Mds-Vo-name"]["GlueSiteUniqueID"], "Site", "Glue", &logger);
      Extractor service(*itS, "Service", "Glue", &logger);
      Extractor cluster(glueCluster, "Cluster", "Glue", &logger);
      Extractor ce(glueCE, "CE", "Glue", &logger);
      
      // If credentials contains a VO, then try to find matching VOView.
      Extractor vo(XMLNode(), "CE", "Glue", &logger);
      VOMSTrustList vomsTrustDN;
      vomsTrustDN.AddRegex(".*");
      std::vector<VOMSACInfo> vomsAttributes;
      Credential cred(uc);
      if (parseVOMSAC(cred, uc.CACertificatesDirectory(), "", "", vomsTrustDN, vomsAttributes)) {
        for (std::vector<VOMSACInfo>::const_iterator itAC = vomsAttributes.begin();
             itAC != vomsAttributes.end(); ++itAC) {
          for (XMLNode n = ce.node["GlueVOViewLocalID"]; n; ++n) {
            if ((std::string)n["GlueVOViewLocalID"] == itAC->voname) {
              vo.node = n;
              break;
            }
          }
          if (vo.node) break;
        }
      }
      
      ComputingServiceType cs;
      ComputingEndpointType ComputingEndpoint;
      ComputingManagerType ComputingManager;
      ComputingShareType ComputingShare;
      
      service.set("Status", ComputingEndpoint->HealthState);
      service.set("Type", ComputingEndpoint->InterfaceName);
      ComputingEndpoint->InterfaceName = lower(ComputingEndpoint->InterfaceName); // CREAM -> cream.
      ComputingEndpoint->Technology = "webservice"; // CREAM is a webservice
      ComputingEndpoint->Capability.insert("information.lookup.job");
      ComputingEndpoint->Capability.insert("executionmanagement.jobcreation");
      ComputingEndpoint->Capability.insert("executionmanagement.jobdescription");
      ComputingEndpoint->Capability.insert("executionmanagement.jobmanager");

      ce.set("Name", ComputingShare->Name);
      ce.set("Name", ComputingShare->MappingQueue);
      //ce.set("InfoJobManager", ComputingShare->MappingQueue);

      ce.set("InfoLRMSType", ComputingManager->ProductName);
      ce.set("InfoLRMSVersion", ComputingManager->ProductVersion);
      ce.set("PolicyAssignedJobSlots", ComputingManager->TotalSlots);
      
      cluster.set("Name", cs->Name);
      
      if (site) {
        site.set("Name", cs.AdminDomain->Name);
        site.set("Sponsor", cs.AdminDomain->Owner, "none");
        site.set("Location", cs.Location->Place);
        site.set("Latitude", cs.Location->Latitude);
        site.set("Longitude", cs.Location->Longitude);
      }

      ce.set("InfoContactString", ComputingEndpoint->URLString);
      
      if (!ce.get("ImplementationName").empty()) {
        if (!ce.get("ImplementationVersion").empty()) {
          ComputingEndpoint->Implementation = Software(ce.get("ImplementationName"), ce.get("ImplementationVersion"));
        }
        else ComputingEndpoint->Implementation = ce.get("ImplementationName");
      }
      
      if (!vo.set("StateTotalJobs",   ComputingShare->TotalJobs))   ce.set("StateTotalJobs", ComputingShare->TotalJobs);
      if (!vo.set("StateRunningJobs", ComputingShare->RunningJobs)) ce.set("StateRunningJobs", ComputingShare->RunningJobs);
      if (!vo.set("StateWaitingJobs", ComputingShare->WaitingJobs, 444444)) ce.set("StateWaitingJobs", ComputingShare->WaitingJobs, 444444);

      if (!vo.set("PolicyMaxWallClockTime", ComputingShare->MaxWallTime)) ce.set("PolicyMaxWallClockTime", ComputingShare->MaxWallTime);

      if (!vo.set("PolicyMaxCPUTime", ComputingShare->MaxCPUTime)) ce.set("PolicyMaxCPUTime", ComputingShare->MaxCPUTime);

      if (!vo.set("PolicyMaxTotalJobs", ComputingShare->MaxTotalJobs, 999999999)) ce.set("PolicyMaxTotalJobs", ComputingShare->MaxTotalJobs, 999999999);
      if (!vo.set("PolicyMaxRunningJobs", ComputingShare->MaxRunningJobs, 999999999)) ce.set("PolicyMaxRunningJobs", ComputingShare->MaxRunningJobs, 999999999);
      if (!vo.set("PolicyMaxWaitingJobs", ComputingShare->MaxWaitingJobs, 999999999)) ce.set("PolicyMaxWaitingJobs", ComputingShare->MaxWaitingJobs, 999999999);
      
      if (!vo.set("PolicyAssignedJobSlots", ComputingShare->MaxUserRunningJobs, 999999999)) ce.set("PolicyAssignedJobSlots", ComputingShare->MaxUserRunningJobs, 999999999);
      if (!vo.set("PolicyMaxSlotsPerJob", ComputingShare->MaxSlotsPerJob, 999999999)) ce.set("PolicyMaxSlotsPerJob", ComputingShare->MaxSlotsPerJob, 999999999);

      // Only consider first SubCluster.
      Extractor subcluster(cluster.node["GlueSubClusterUniqueID"], "", "Glue", &logger);
      subcluster.set("HostMainMemoryRAMSize", ComputingShare->MaxMainMemory);
      subcluster.set("HostMainMemoryVirtualSize", ComputingShare->MaxVirtualMemory);

      ExecutionEnvironmentType ee;
      subcluster.set("HostNetworkAdapterInboundIP",  ee->ConnectivityIn);
      subcluster.set("HostNetworkAdapterOutboundIP", ee->ConnectivityOut);
      subcluster.set("HostProcessorClockSpeed", ee->CPUClockSpeed);
      subcluster.set("HostProcessorModel", ee->CPUModel);
      subcluster.set("HostArchitecturePlatformType", ee->Platform);
      subcluster.set("HostProcessorVendor", ee->CPUVendor);
      subcluster.set("HostMainMemoryRAMSize", ee->MainMemorySize);
      //subcluster.set("HostMainMemoryVirtualSize", ee->VirtualMemorySize); // 'VirtualMemorySize': No such member in ExecutionEnvironment.
      //subcluster.set("SubClusterPhysicalCPUs", ee->PhysicalCPUs); // 'PhysicalCPUs': No such member in ExecutionEnvironment.
      //subcluster.set("SubClusterLogicalCPUs", ee->LogicalCPUs); // 'LogicalCPUs': No such member in ExecutionEnvironment.
      ee->OperatingSystem = Software(subcluster["HostOperatingSystemName"], subcluster["HostOperatingSystemRelease"]);

      std::string defaultSE = "";
      if (!vo.set("InfoDefaultSE", defaultSE)) ce.set("InfoDefaultSE", defaultSE);
      if (!defaultSE.empty()) ComputingShare->DefaultStorageService = "gsiftp://" + defaultSE;
      
      if (!vo.set("PolicyPreemption", ComputingShare->Preemption)) ce.set("PolicyPreemption", ComputingShare->Preemption);
      if (!vo.set("StateStatus", ComputingEndpoint->ServingState)) ce.set("StateStatus", ComputingEndpoint->ServingState);
      if (!vo.set("StateEstimatedResponseTime", ComputingShare->EstimatedAverageWaitingTime, "2146660842")) {
        ce.set("StateEstimatedResponseTime", ComputingShare->EstimatedAverageWaitingTime, "2146660842");
      }
      if (!vo.set("StateWorstResponseTime", ComputingShare->EstimatedWorstWaitingTime, "2146660842")) {
        ce.set("StateWorstResponseTime", ComputingShare->EstimatedWorstWaitingTime, "2146660842");
      }

      vo.set("StateFreeJobSlots", ComputingShare->FreeSlots) ||
      vo.set("StateFreeCPUs", ComputingShare->FreeSlots) ||
      ce.set("StateFreeJobSlots", ComputingShare->FreeSlots) ||
      ce.set("StateFreeJobCPUs", ComputingShare->FreeSlots);

      for (XMLNode node = subcluster.node["GlueHostApplicationSoftwareRunTimeEnvironment"]; node; ++node) {
        ApplicationEnvironment ae((std::string)node);
        ae.State = "UNDEFINEDVALUE";
        ae.FreeSlots = -1;
        ae.FreeUserSeats = -1;
        ae.FreeJobs = -1;
        ComputingManager.ApplicationEnvironments->push_back(ae);
      }

      cs.ComputingEndpoint.insert(std::make_pair(0, ComputingEndpoint));
      
      // Create information endpoint.
      ComputingEndpointType infoEndpoint;
      if (!((std::string)ce.node["GlueInformationServiceURL"]).empty()) {
        infoEndpoint->URLString = (std::string)ce.node["GlueInformationServiceURL"];
        infoEndpoint->InterfaceName = "org.nordugrid.ldapglue1";
        infoEndpoint->Capability.insert("information.discovery.resource");
        infoEndpoint->Technology = "ldap";
        cs.ComputingEndpoint.insert(std::make_pair(1, infoEndpoint));
      }
      
      ComputingManager.ExecutionEnvironment.insert(std::make_pair(0, ee));
      cs.ComputingManager.insert(std::make_pair(0, ComputingManager));
      cs.ComputingShare.insert(std::make_pair(0, ComputingShare));
      csList.push_back(cs);
    }

    if (!csList.empty()) s = EndpointQueryingStatus::SUCCESSFUL;
    return s;
  }

} // namespace Arc
