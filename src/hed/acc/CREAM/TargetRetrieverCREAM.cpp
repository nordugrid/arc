// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/ArcConfig.h>
#include <arc/Logger.h>
#include <arc/StringConv.h>
#include <arc/Thread.h>
#include <arc/URL.h>
#include <arc/XMLNode.h>
#include <arc/client/ExecutionTarget.h>
#include <arc/client/TargetGenerator.h>
#include <arc/data/DataBuffer.h>
#include <arc/data/DataHandle.h>

#include "TargetRetrieverCREAM.h"

namespace Arc {

  class ThreadArgCREAM {
  public:
    TargetGenerator *mom;
    const UserConfig *usercfg;
    URL url;
    bool isExecutionTarget;
  };

  ThreadArgCREAM* TargetRetrieverCREAM::CreateThreadArg(TargetGenerator& mom,
                                                   bool isExecutionTarget) {
    ThreadArgCREAM *arg = new ThreadArgCREAM;
    arg->mom = &mom;
    arg->usercfg = &usercfg;
    arg->url = url;
    arg->isExecutionTarget = isExecutionTarget;
    return arg;
  }

  Logger TargetRetrieverCREAM::logger(Logger::getRootLogger(),
                                      "TargetRetriever.CREAM");

  static URL CreateURL(std::string service, ServiceType /* st */) {
    std::string::size_type pos1 = service.find("://");
    if (pos1 == std::string::npos) {
      service = "ldap://" + service;
      pos1 = 4;
    }
    std::string::size_type pos2 = service.find(":", pos1 + 3);
    std::string::size_type pos3 = service.find("/", pos1 + 3);
    if (pos3 == std::string::npos) {
      if (pos2 == std::string::npos)
        service += ":2170";
      // Is this a good default path?
      // Different for computing and index?
      service += "/o=Grid";
    }
    else if (pos2 == std::string::npos || pos2 > pos3)
      service.insert(pos3, ":2170");
    return service;
  }

  TargetRetrieverCREAM::TargetRetrieverCREAM(const UserConfig& usercfg,
                                             const std::string& service,
                                             ServiceType st)
    : TargetRetriever(usercfg, CreateURL(service, st), st, "CREAM") {}

  TargetRetrieverCREAM::~TargetRetrieverCREAM() {}

  Plugin* TargetRetrieverCREAM::Instance(PluginArgument *arg) {
    TargetRetrieverPluginArgument *trarg =
      dynamic_cast<TargetRetrieverPluginArgument*>(arg);
    if (!trarg)
      return NULL;
    return new TargetRetrieverCREAM(*trarg, *trarg, *trarg);
  }

  void TargetRetrieverCREAM::GetExecutionTargets(TargetGenerator& mom) {
    logger.msg(VERBOSE, "TargetRetriver%s initialized with %s service url: %s",
               flavour, tostring(serviceType), url.str());

    for (std::list<std::string>::const_iterator it =
           usercfg.GetRejectedServices(serviceType).begin();
         it != usercfg.GetRejectedServices(serviceType).end(); it++) {
      std::string::size_type pos = it->find(":");
      if (pos != std::string::npos) {
        std::string flav = it->substr(0, pos);
        if (flav == flavour || flav == "*" || flav.empty())
          if (url == CreateURL(it->substr(pos + 1), serviceType)) {
            logger.msg(INFO, "Rejecting service: %s", url.str());
            return;
          }
      }
    }

    if (serviceType == COMPUTING && mom.AddService(flavour, url) ||
        serviceType == INDEX     && mom.AddIndexServer(flavour, url)) {
      ThreadArgCREAM *arg = CreateThreadArg(mom, true);
      if (!CreateThreadFunction((serviceType == COMPUTING ?
                                 &InterrogateTarget : &QueryIndex),
                                arg, &(mom.ServiceCounter()))) {
        delete arg;
      }
    }
  }

  void TargetRetrieverCREAM::GetJobs(TargetGenerator& mom) {
    logger.msg(VERBOSE, "TargetRetriver%s initialized with %s service url: %s",
               flavour, tostring(serviceType), url.str());

    for (std::list<std::string>::const_iterator it =
           usercfg.GetRejectedServices(serviceType).begin();
         it != usercfg.GetRejectedServices(serviceType).end(); it++) {
      std::string::size_type pos = it->find(":");
      if (pos != std::string::npos) {
        std::string flav = it->substr(0, pos);
        if (flav == flavour || flav == "*" || flav.empty())
          if (url == CreateURL(it->substr(pos + 1), serviceType)) {
            logger.msg(INFO, "Rejecting service: %s", url.str());
            return;
          }
      }
    }

    if (serviceType == COMPUTING && mom.AddService(flavour, url) ||
        serviceType == INDEX     && mom.AddIndexServer(flavour, url)) {
      ThreadArgCREAM *arg = CreateThreadArg(mom, false);
      if (!CreateThreadFunction((serviceType == COMPUTING ?
                                 &InterrogateTarget : &QueryIndex),
                                arg, &(mom.ServiceCounter()))) {
        delete arg;
      }
    }
  }

  void TargetRetrieverCREAM::QueryIndex(void *arg) {
    ThreadArgCREAM *thrarg = (ThreadArgCREAM*)arg;

    if (!thrarg->isExecutionTarget) {
      delete thrarg;
      return;
    }

    TargetGenerator& mom = *thrarg->mom;
    const UserConfig& usercfg = *thrarg->usercfg;

    URL& url = thrarg->url;
    url.ChangeLDAPScope(URL::subtree);
    url.ChangeLDAPFilter("(|(GlueServiceType=bdii_site)"
                         "(GlueServiceType=bdii_top))");
    DataHandle handler(url, usercfg);
    DataBuffer buffer;

    if (!handler) {
      logger.msg(INFO, "Can't create information handle - "
                 "is the ARC ldap DMC plugin available?");
      delete thrarg;
      return;
    }

    if (!handler->StartReading(buffer)) {
      delete thrarg;
      return;
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
      delete thrarg;
      return;
    }

    XMLNode XMLresult(result);

    std::list<XMLNode> topBDIIs =
      XMLresult.XPathLookup("//*[GlueServiceType='bdii_top']", NS());

    for (std::list<XMLNode>::iterator iter = topBDIIs.begin();
         iter != topBDIIs.end(); ++iter) {

      if ((std::string)(*iter)["GlueServiceStatus"] != "OK")
        continue;

      TargetRetrieverCREAM retriever(usercfg, (std::string)(*iter)["GlueServiceEndpoint"], INDEX);
      if (thrarg->isExecutionTarget) {
        retriever.GetExecutionTargets(mom);
      }
      else {
        retriever.GetJobs(mom);
      }
    }

    std::list<XMLNode> siteBDIIs =
      XMLresult.XPathLookup("//*[GlueServiceType='bdii_site']", NS());

    for (std::list<XMLNode>::iterator iter = siteBDIIs.begin();
         iter != siteBDIIs.end(); ++iter) {

      if ((std::string)(*iter)["GlueServiceStatus"] != "OK")
        continue;

      //Should filter here on allowed VOs, not yet implemented

      TargetRetrieverCREAM retriever(usercfg, (std::string)(*iter)["GlueServiceEndpoint"], COMPUTING);
      if (thrarg->isExecutionTarget) {
        retriever.GetExecutionTargets(mom);
      }
      else {
        retriever.GetJobs(mom);
      }
    }

    delete thrarg;
  }

  void TargetRetrieverCREAM::InterrogateTarget(void *arg) {
    ThreadArgCREAM *thrarg = (ThreadArgCREAM*)arg;

    if (!thrarg->isExecutionTarget) {
      delete thrarg;
      return;
    }

    TargetGenerator& mom = *thrarg->mom;
    const UserConfig& usercfg = *thrarg->usercfg;

    URL url = thrarg->url;
    url.ChangeLDAPScope(URL::subtree);
    DataHandle handler(url, usercfg);
    DataBuffer buffer;

    if (!handler) {
      logger.msg(INFO, "Can't create information handle - "
                 "is the ARC ldap DMC plugin available?");
      delete thrarg;
      return;
    }

    if (!handler->StartReading(buffer)) {
      delete thrarg;
      return;
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
      delete thrarg;
      return;
    }

    XMLNode XMLresult(result);

    // Create one ExecutionTarget per VOView record.

    std::list<XMLNode> VOViews =
      XMLresult.XPathLookup("//*[objectClass='GlueVOView']", NS());

    for (std::list<XMLNode>::iterator it = VOViews.begin();
         it != VOViews.end(); it++) {

      XMLNode VOView(*it);

      ExecutionTarget target;

      target.GridFlavour = "CREAM";
      target.Cluster = thrarg->url;

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

      XMLNode Cluster =
        *XMLresult.XPathLookup("//*[objectClass='GlueCluster']"
                               "[GlueClusterUniqueID='" +
                               key.substr(pos + 1) + "']", NS()).begin();

      // What to do if a cluster has more than one subcluster???
      XMLNode SubCluster =
        *XMLresult.XPathLookup("//*[objectClass='GlueSubCluster']"
                               "[GlueChunkKey='" + key + "']", NS()).begin();

      for (XMLNode node = Cluster["GlueForeignKey"]; node; ++node) {
        key = (std::string)node;
        pos = key.find('=');
        if (key.substr(0, pos) == "GlueSiteUniqueID")
          break;
      }

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

      // TODO: we need to somehow query the HealthState

      target.HealthState = "ok";


      if (CE["GlueCEName"])
        target.ComputingShareName = (std::string)CE["GlueCEName"];
      if (CE["GlueCEInfoLRMSType"])
        target.ManagerProductName = (std::string)CE["GlueCEInfoLRMSType"];
      if (CE["GlueCEInfoLRMSVersion"])
        target.ManagerProductVersion = (std::string)CE["GlueCEInfoLRMSVersion"];
      if (Site["GlueSiteName"])
        target.DomainName = (std::string)Site["GlueSiteName"];
      if (Site["GlueSiteLocation"])
        target.Place = (std::string)Site["GlueSiteLocation"];
      if (Site["GlueSiteLatitude"])
        target.Latitude = stringtof(Site["GlueSiteLatitude"]);
      if (Site["GlueSiteLongitude"])
        target.Longitude = stringtof(Site["GlueSiteLongitude"]);
      if (CE["GlueCEInfoContactString"])
        target.url = (std::string)CE["GlueCEInfoContactString"];
      if (CE["GlueCEImplementationName"]) {
        if (CE["GlueCEImplementationVersion"])
          target.Implementation =
            Software((std::string)CE["GlueCEImplementationName"],
                     (std::string)CE["GlueCEImplementationVersion"]);
        else
          target.Implementation =
            (std::string)CE["GlueCEImplementationName"];
      }
      if (VOView["GlueCEStateTotalJobs"])
        target.TotalJobs = stringtoi(VOView["GlueCEStateTotalJobs"]);
      else if (CE["GlueCEStateTotalJobs"])
        target.TotalJobs = stringtoi(CE["GlueCEStateTotalJobs"]);
      if (VOView["GlueCEStateRunningJobs"])
        target.RunningJobs = stringtoi(VOView["GlueCEStateRunningJobs"]);
      else if (CE["GlueCEStateRunningJobs"])
        target.RunningJobs = stringtoi(CE["GlueCEStateRunningJobs"]);
      if (VOView["GlueCEStateWaitingJobs"])
        target.WaitingJobs = stringtoi(VOView["GlueCEStateWaitingJobs"]);
      else if (CE["GlueCEStateWaitingJobs"])
        target.WaitingJobs = stringtoi(CE["GlueCEStateWaitingJobs"]);

      // target.StagingJobs           - not available in schema
      // target.SuspendedJobs         - not available in schema
      // target.PreLRMSWaitingJobs    - not available in schema
      // target.ComputingShareName          - not available in schema

      if (VOView["GlueCEPolicyMaxWallClockTime"])
        target.MaxWallTime = stringtoi(VOView["GlueCEPolicyMaxWallClockTime"]);
      else if (CE["GlueCEPolicyMaxWallClockTime"])
        target.MaxWallTime = stringtoi(CE["GlueCEPolicyMaxWallClockTime"]);

      // target.MinWallTime           - not available in schema
      // target.DefaultWallTime       - not available in schema

      if (VOView["GlueCEPolicyMaxCPUTime"])
        target.MaxCPUTime = stringtoi(VOView["GlueCEPolicyMaxCPUTime"]);
      else if (CE["GlueCEPolicyMaxCPUTime"])
        target.MaxCPUTime = stringtoi(CE["GlueCEPolicyMaxCPUTime"]);

      // target.MinCPUTime            - not available in schema
      // target.DefaultCPUTime        - not available in schema

      if (VOView["GlueCEPolicyMaxTotalJobs"])
        target.MaxTotalJobs = stringtoi(VOView["GlueCEPolicyMaxTotalJobs"]);
      else if (CE["GlueCEPolicyMaxTotalJobs"])
        target.MaxTotalJobs = stringtoi(CE["GlueCEPolicyMaxTotalJobs"]);
      if (VOView["GlueCEPolicyMaxRunningJobs"])
        target.MaxRunningJobs =
          stringtoi(VOView["GlueCEPolicyMaxRunningJobs"]);
      else if (CE["GlueCEPolicyMaxRunningJobs"])
        target.MaxRunningJobs = stringtoi(CE["GlueCEPolicyMaxRunningJobs"]);
      if (VOView["GlueCEPolicyMaxWaitingJobs"])
        target.MaxWaitingJobs =
          stringtoi(VOView["GlueCEPolicyMaxWaitingJobs"]);
      else if (CE["GlueCEPolicyMaxWaitingJobs"])
        target.MaxWaitingJobs = stringtoi(CE["GlueCEPolicyMaxWaitingJobs"]);
      if (SubCluster["GlueHostMainMemoryRAMSize"])
        target.MaxMainMemory = stringtoi(SubCluster["GlueHostMainMemoryRAMSize"]);

      // target.MaxPreLRMSWaitingJobs - not available in schema

      // is this correct ???
      if (VOView["GlueCEPolicyAssignedJobSlots"])
        target.MaxUserRunningJobs =
          stringtoi(VOView["GlueCEPolicyAssignedJobSlots"]);
      else if (CE["GlueCEPolicyAssignedJobSlots"])
        target.MaxUserRunningJobs =
          stringtoi(CE["GlueCEPolicyAssignedJobSlots"]);
      if (VOView["GlueCEPolicyMaxSlotsPerJob"])
        target.MaxSlotsPerJob =
          stringtoi(VOView["GlueCEPolicyMaxSlotsPerJob"]);
      else if (CE["GlueCEPolicyMaxSlotsPerJob"])
        target.MaxSlotsPerJob =
          stringtoi(CE["GlueCEPolicyMaxSlotsPerJob"]);

      // target.MaxStageInStreams     - not available in schema
      // target.MaxStageOutStreams    - not available in schema
      // target.SchedulingPolicy      - not available in schema

      if (SubCluster["GlueHostMainMemoryVirtualSize"])
        target.MaxMainMemory =
          stringtoi(SubCluster["GlueHostMainMemoryVirtualSize"]);

      // target.MaxDiskSpace          - not available in schema

      if (VOView["GlueCEInfoDefaultSE"])
        target.DefaultStorageService =
          (std::string)VOView["GlueCEInfoDefaultSE"];
      else if (CE["GlueCEInfoDefaultSE"])
        target.DefaultStorageService = (std::string)CE["GlueCEInfoDefaultSE"];
      if (VOView["GlueCEPolicyPreemption"])
        target.Preemption = stringtoi(VOView["GlueCEPolicyPreemption"]);
      else if (CE["GlueCEPolicyPreemption"])
        target.Preemption = stringtoi(CE["GlueCEPolicyPreemption"]);
      if (VOView["GlueCEStateStatus"])
        target.ServingState = (std::string)VOView["GlueCEStateStatus"];
      else if (CE["GlueCEStateStatus"])
        target.ServingState = (std::string)CE["GlueCEStateStatus"];
      if (VOView["GlueCEStateEstimatedResponseTime"])
        target.EstimatedAverageWaitingTime =
          stringtoi(VOView["GlueCEStateEstimatedResponseTime"]);
      else if (CE["GlueCEStateEstimatedResponseTime"])
        target.EstimatedAverageWaitingTime =
          stringtoi(CE["GlueCEStateEstimatedResponseTime"]);
      if (VOView["GlueCEStateWorstResponseTime"])
        target.EstimatedWorstWaitingTime =
          stringtoi(VOView["GlueCEStateWorstResponseTime"]);
      else if (CE["GlueCEStateWorstResponseTime"])
        target.EstimatedWorstWaitingTime =
          stringtoi(CE["GlueCEStateWorstResponseTime"]);
      if (VOView["GlueCEStateFreeJobSlots"])
        target.FreeSlots = stringtoi(VOView["GlueCEStateFreeJobSlots"]);
      else if (VOView["GlueCEStateFreeCPUs"])
        target.FreeSlots = stringtoi(VOView["GlueCEStateFreeCPUs"]);
      else if (CE["GlueCEStateFreeJobSlots"])
        target.FreeSlots = stringtoi(CE["GlueCEStateFreeJobSlots"]);
      else if (CE["GlueCEStateFreeCPUs"])
        target.FreeSlots = stringtoi(CE["GlueCEStateFreeCPUs"]);

      // target.UsedSlots;
      // target.RequestedSlots;
      // target.ReservationPolicy;

      for (XMLNode node =
             SubCluster["GlueHostApplicationSoftwareRunTimeEnvironment"];
           node; ++node) {
        ApplicationEnvironment ae((std::string)node);
        ae.State = "UNDEFINEDVALUE";
        ae.FreeSlots = -1;
        ae.FreeUserSeats = -1;
        ae.FreeJobs = -1;
        target.ApplicationEnvironments.push_back(ae);
      }
      //Register target in TargetGenerator list
      mom.AddTarget(target);
    }

    delete thrarg;
  }

} // namespace Arc
