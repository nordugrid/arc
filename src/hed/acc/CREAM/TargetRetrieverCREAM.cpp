#include "TargetRetrieverCREAM.h"
#include <arc/client/ExecutionTarget.h>
#include <arc/data/DataHandle.h>
#include <arc/data/DataBufferPar.h>
#include <arc/StringConv.h>
#include <arc/XMLNode.h>
#include <arc/URL.h>

namespace Arc {

  TargetRetrieverCREAM::TargetRetrieverCREAM(Config *cfg)
    : TargetRetriever(cfg) {}

  TargetRetrieverCREAM::~TargetRetrieverCREAM() {}

  ACC *TargetRetrieverCREAM::Instance(Config *cfg, ChainContext *) {
    return new TargetRetrieverCREAM(cfg);
  }

  void TargetRetrieverCREAM::GetTargets(TargetGenerator& mom, int TargetType,
					int DetailLevel) {

    if (mom.DoIAlreadyExist(m_url))
      return;


    if (ServiceType == "computing") {
      //Add Service to TG list

      bool AddedService(mom.AddService(m_url));

      std::cout << "TargetRetriverCREAM initialized with computing service url" << std::endl;

      //If added, interrogate service
      //Lines below this point depend on the usage of TargetGenerator
      //i.e. if it is used to find Targets for execution or storage,
      //and/or if the entire information is requested or only endpoints
      if (AddedService)
	InterrogateTarget(mom, m_url, TargetType, DetailLevel);
    }
    else if (ServiceType == "storage") {}
    else if (ServiceType == "index") {

      std::cout << "TargetRetriverCREAM initialized with index service url" << std::endl;

      DataHandle handler(m_url + "??sub?(|(GlueServiceType=bdii_site)(GlueServiceType=bdii_top))");
      DataBufferPar buffer;

      if (!handler->StartReading(buffer))
	return;

      int handle;
      unsigned int length;
      unsigned long long int offset;
      std::string result;

      while (buffer.for_write() || !buffer.eof_read())
	if (buffer.for_write(handle, length, offset, true)) {
	  result.append(buffer[handle], length);
	  buffer.is_written(handle);
	}

      if (!handler->StopReading())
	return;

      XMLNode XMLresult(result);

      std::list<XMLNode> topBDIIs =
	XMLresult.XPathLookup("//*[GlueServiceType='bdii_top']", NS());

      std::list<XMLNode>::iterator iter;

      for (iter = topBDIIs.begin(); iter != topBDIIs.end(); ++iter) {

	if ((std::string)(*iter)["GlueServiceStatus"] != "OK")
	  continue;

	std::string url = (std::string)(*iter)["GlueServiceEndpoint"];

	NS ns;
	Arc::Config cfg(ns);
	Arc::XMLNode URLXML = cfg.NewChild("URL") = url;
	URLXML.NewAttribute("ServiceType") = "index";

	TargetRetrieverCREAM thisBDII(&cfg);

	thisBDII.GetTargets(mom, TargetType, DetailLevel);

      } //end topBDIIs

      std::list<XMLNode> siteBDIIs =
	XMLresult.XPathLookup("//*[GlueServiceType='bdii_site']", NS());

      for (iter = siteBDIIs.begin(); iter != siteBDIIs.end(); ++iter) {
	if ((std::string)(*iter)["GlueServiceStatus"] != "OK")
	  continue;
	std::string url = (std::string)(*iter)["GlueServiceEndpoint"];

	//Should filter here on allowed VOs, not yet implemented

	//Add Service to TG list
	bool AddedService(mom.AddService(url));

	//If added, interrogate service
	//Lines below this point depend on the usage of TargetGenerator
	//i.e. if it is used to find Targets for execution or storage,
	//and/or if the entire information is requested or only endpoints
	if (AddedService)
	  InterrogateTarget(mom, url, TargetType, DetailLevel);
      }
    } //end if index type
  }

  void TargetRetrieverCREAM::InterrogateTarget(TargetGenerator& mom,
					       std::string url, int TargetType,
					       int DetailLevel) {

    DataHandle handler(url + "??sub");
    DataBufferPar buffer;

    if (!handler->StartReading(buffer))
      return;

    int handle;
    unsigned int length;
    unsigned long long int offset;
    std::string result;

    while (buffer.for_write() || !buffer.eof_read())
      if (buffer.for_write(handle, length, offset, true)) {
	result.append(buffer[handle], length);
	buffer.is_written(handle);
      }

    if (!handler->StopReading())
      return;

    XMLNode XMLresult(result);

    XMLresult.SaveToStream(std::cout);

    // Create one ExecutionTarget per VOView record.

    std::list<XMLNode> VOViews =
      XMLresult.XPathLookup("//*[objectClass='GlueVOView']", NS());

    for (std::list<XMLNode>::iterator it = VOViews.begin();
	 it != VOViews.end(); it++) {

      XMLNode VOView(*it);

      ExecutionTarget target;

      target.GridFlavour = "CREAM";

      std::string key;
      std::string::size_type pos;

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

      if (Site["GlueSiteName"])
	target.Name = (std::string)Site["GlueSiteName"];

      if (Site["GlueSiteLocation"])
	target.Place = (std::string)Site["GlueSiteLocation"];

      if (Site["GlueSiteLatitude"])
	target.Latitude = stringtof(Site["GlueSiteLatitude"]);

      if (Site["GlueSiteLongitude"])
	target.Longitude = stringtof(Site["GlueSiteLongitude"]);

      if (CE["GlueCEImplementationName"])
	target.ImplementationName =
	  (std::string)CE["GlueCEImplementationName"];

      if (CE["GlueCEImplementationVersion"])
	target.ImplementationVersion =
	  (std::string)CE["GlueCEImplementationVersion"];

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
      // target.MappingQueue          - not available in schema

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
	target.MaxRunningJobs = stringtoi(VOView["GlueCEPolicyMaxRunningJobs"]);
      else if (CE["GlueCEPolicyMaxRunningJobs"])
	target.MaxRunningJobs = stringtoi(CE["GlueCEPolicyMaxRunningJobs"]);

      if (VOView["GlueCEPolicyMaxWaitingJobs"])
	target.MaxWaitingJobs = stringtoi(VOView["GlueCEPolicyMaxWaitingJobs"]);
      else if (CE["GlueCEPolicyMaxWaitingJobs"])
	target.MaxWaitingJobs = stringtoi(CE["GlueCEPolicyMaxWaitingJobs"]);

      if (SubCluster["GlueHostMainMemoryRAMSize"])
	target.NodeMemory = stringtoi(SubCluster["GlueHostMainMemoryRAMSize"]);

      // target.MaxPreLRMSWaitingJobs - not available in schema

      // is this correct ???
      if (VOView["GlueCEStateFreeJobSlots"])
	target.MaxUserRunningJobs = stringtoi(VOView["GlueCEPolicyAssignedJobSlots"]);
      else if (CE["GlueCEStateFreeJobSlots"])
	target.MaxUserRunningJobs = stringtoi(CE["GlueCEPolicyAssignedJobSlots"]);

      if (VOView["MaxSlotsPerJob"])
	target.MaxWaitingJobs = stringtoi(VOView["MaxSlotsPerJob"]);
      else if (CE["MaxSlotsPerJob"])
	target.MaxWaitingJobs = stringtoi(CE["MaxSlotsPerJob"]);

      // target.MaxStageInStreams     - not available in schema
      // target.MaxStageOutStreams    - not available in schema
      // target.SchedulingPolicy      - not available in schema

      if (SubCluster["GlueHostMainMemoryVirtualSize"])
	target.MaxMemory = stringtoi(SubCluster["GlueHostMainMemoryVirtualSize"]);

      // target.MaxDiskSpace          - not available in schema

      if (VOView["GlueCEInfoDefaultSE"])
	target.DefaultStorageService = (std::string)VOView["GlueCEInfoDefaultSE"];
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

      /*
         for (XMLNode node =
             SubCluster["GlueHostApplicationSoftwareRunTimeEnvironment"];
           node; ++node) {
         Glue2::ApplicationEnvironment_t env;
         env.Name = (std::string) node;
         target.ApplicationEnvironments.push_back(env);
         }
       */

      //Register target in TargetGenerator list
      mom.AddTarget(target);
    }
  }

} // namespace Arc


acc_descriptors ARC_ACC_LOADER = {
  {"TargetRetrieverCREAM", 0, &Arc::TargetRetrieverCREAM::Instance},
  {NULL, 0, NULL}
};
