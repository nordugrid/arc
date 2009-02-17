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
#include <arc/message/MCC.h>
#include <glibmm/stringutils.h>

#include "AREXClient.h"
#include "TargetRetrieverARC1.h"

//Remove these after debugging
#include <iostream>
//End of debugging headers

namespace Arc {

  struct ThreadArg {
    TargetGenerator *mom;
    std::string proxyPath;
    std::string certificatePath;
    std::string keyPath;
    std::string caCertificatesDir;
    URL url;
    int targetType;
    int detailLevel;
  };

  const std::string alphanum("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789");

  ThreadArg* TargetRetrieverARC1::CreateThreadArg(TargetGenerator& mom,
						  int targetType,
						  int detailLevel) {
    ThreadArg *arg = new ThreadArg;
    arg->mom = &mom;
    arg->proxyPath = proxyPath;
    arg->certificatePath = certificatePath;
    arg->keyPath = keyPath;
    arg->caCertificatesDir = caCertificatesDir;
    arg->url = url;
    arg->targetType = targetType;
    arg->detailLevel = detailLevel;
    return arg;
  }

  Logger TargetRetrieverARC1::logger(TargetRetriever::logger, "ARC1");

  TargetRetrieverARC1::TargetRetrieverARC1(Config *cfg)
    : TargetRetriever(cfg, "ARC1") {}

  TargetRetrieverARC1::~TargetRetrieverARC1() {}

  Plugin* TargetRetrieverARC1::Instance(Arc::PluginArgument* arg) {
    ACCPluginArgument* accarg =
            arg?dynamic_cast<ACCPluginArgument*>(arg):NULL;
    if(!accarg) return NULL;
    return new TargetRetrieverARC1((Arc::Config*)(*accarg));
  }

  void TargetRetrieverARC1::GetTargets(TargetGenerator& mom, int targetType,
				       int detailLevel) {

    logger.msg(INFO, "TargetRetriverARC1 initialized with %s service url: %s",
	       serviceType, url.str());

    if (serviceType == "computing") {
      bool added = mom.AddService(url);
      if (added) {
	ThreadArg *arg = CreateThreadArg(mom, targetType, detailLevel);
	if (!CreateThreadFunction(&InterrogateTarget, arg)) {
	  delete arg;
	  mom.RetrieverDone();
	}
      }
    }
    else if (serviceType == "storage") {}
    else if (serviceType == "index") {
      bool added = mom.AddIndexServer(url);
      if (added) {
	ThreadArg *arg = CreateThreadArg(mom, targetType, detailLevel);
	if (!CreateThreadFunction(&QueryIndex, arg)) {
	  delete arg;
	  mom.RetrieverDone();
	}
      }
    }
    else
      logger.msg(ERROR,
		 "TargetRetrieverARC1 initialized with unknown url type");
  }

  void TargetRetrieverARC1::QueryIndex(void *arg) {
    ThreadArg *thrarg = (ThreadArg*)arg;
    TargetGenerator& mom = *thrarg->mom;
    // URL& url = thrarg->url;

    // TODO: ISIS

    delete thrarg;
    mom.RetrieverDone();
  }

  void TargetRetrieverARC1::InterrogateTarget(void *arg) {
    ThreadArg *thrarg = (ThreadArg*)arg;
    TargetGenerator& mom = *thrarg->mom;

    URL& url = thrarg->url;
    MCCConfig cfg;
    if (!thrarg->proxyPath.empty())
      cfg.AddProxy(thrarg->proxyPath);
    if (!thrarg->certificatePath.empty())
      cfg.AddCertificate(thrarg->certificatePath);
    if (!thrarg->keyPath.empty())
      cfg.AddPrivateKey(thrarg->keyPath);
    if (!thrarg->caCertificatesDir.empty())
      cfg.AddCADir(thrarg->caCertificatesDir);
    AREXClient ac(url, cfg);
    XMLNode ServerStatus;
    if (!ac.sstat(ServerStatus)) {
      delete thrarg;
      mom.RetrieverDone();
      return;
    }

    XMLNode GLUEService = ServerStatus;
    std::string tmpsa[] = {"GetResourcePropertyDocumentResponse", "InfoRoot", "Domains", "AdminDomain", "Services", "Service"};
    std::list<std::string> tmpsl(tmpsa, tmpsa+6);

    {
      std::list<std::string>::iterator si = tmpsl.begin();
      for (; si != tmpsl.end() && GLUEService[*si]; GLUEService=GLUEService[*si], si++);
      if (si != tmpsl.end()) {
        std::string path = ".";
        for (std::list<std::string>::iterator si2 = tmpsl.begin(); si2 != si; si2++){
          path += "/" + *si2;
        }
        logger.msg(ERROR, "The node %s has no %s element.", path, *si);
        delete thrarg;
        mom.RetrieverDone();
        return;
      }
    }

    XMLNode NUGCService = ServerStatus;
    std::string tmpsb[] = {"GetResourcePropertyDocumentResponse", "InfoRoot", "nordugrid"};
    std::list<std::string> tmpsk(tmpsb, tmpsb+3);

    {
      std::list<std::string>::iterator si = tmpsk.begin();
      for (; si != tmpsk.end() && NUGCService[*si]; NUGCService=NUGCService[*si], si++);
      if (si != tmpsk.end()) {
        std::string path = ".";
        for (std::list<std::string>::iterator si2 = tmpsk.begin(); si2 != si; si2++){
          path += "/" + *si2;
        }
        logger.msg(ERROR, "The node %s has no %s element.", path, *si);
        delete thrarg;
        mom.RetrieverDone();
        return;
      }
    }

    //std::cout << std::endl << "---Begin response---" << std::endl << ServerStatus << std::endl << "---End response---" << std::endl; //for dubugging

    ExecutionTarget target;

    target.GridFlavour = "ARC1";
    target.Cluster = thrarg->url;
    target.url = url;
    target.InterfaceName = "BES";
    target.Implementor = "NorduGrid";
    target.ImplementationName = "A-REX";

    target.DomainName = url.Host();

    if (GLUEService["ComputingEndpoint"]["HealthState"]) {
      target.HealthState = (std::string) GLUEService["ComputingEndpoint"]["HealthState"];
    } else {
      logger.msg(WARNING, "The Service advertises no Health State.");
    }

    if (GLUEService["Name"]) {
      target.ServiceName = (std::string)GLUEService["Name"];
    } else {
      logger.msg(INFO, "The Service doesn't advertise its Name.");
    }

    if (GLUEService["Capability"]) {
      for (XMLNode n = GLUEService["Capability"]; n; ++n)
	target.Capability.push_back((std::string)n);
    } else {
      logger.msg(INFO, "The Service doesn't advertise its Capability.");
    }

    if (GLUEService["Type"]) {
      target.ServiceType = (std::string)GLUEService["Type"];
    } else {
      logger.msg(WARNING, "The Service doesn't advertise its Type.");
    }

    if (GLUEService["QualityLevel"]) {
      target.QualityLevel = (std::string)GLUEService["QualityLevel"];
    } else {
      logger.msg(WARNING, "The Service doesn't advertise its Quality Level.");
    }

    if (GLUEService["ComputingEndpoint"]["Technology"]) {
      target.Technology = (std::string)GLUEService["ComputingEndpoint"]["Technology"];
    } else {
      logger.msg(INFO, "The Service doesn't advertise its Technology.");
    }

    if (GLUEService["ComputingEndpoint"]["InterfaceName"]) {
      target.InterfaceName = (std::string)GLUEService["ComputingEndpoint"]["InterfaceName"];
    } else if (GLUEService["ComputingEndpoint"]["Interface"]) {
      target.InterfaceName = (std::string)GLUEService["ComputingEndpoint"]["Interface"];
    } else {
      logger.msg(WARNING, "The Service doesn't advertise its Interface.");
    }

    if (GLUEService["ComputingEndpoint"]["InterfaceExtension"]) {
      for (XMLNode n = GLUEService["ComputingEndpoint"]["InterfaceExtension"]; n; ++n)
	target.InterfaceExtension.push_back((std::string)n);
    } else {
      logger.msg(INFO, "The Service doesn't advertise an Interface Extension.");
    }

    if (GLUEService["ComputingEndpoint"]["SupportedProfile"]) {
      for (XMLNode n = GLUEService["ComputingEndpoint"]["SupportedProfile"]; n; ++n)
	target.SupportedProfile.push_back((std::string)n);
    } else {
      logger.msg(INFO, "The Service doesn't advertise any Supported Profile.");
    }

    if (GLUEService["ComputingEndpoint"]["ImplementationVersion"]) {
      target.ImplementationVersion = (std::string)GLUEService["ComputingEndpoint"]["ImplementationVersion"];
    } else {
      logger.msg(INFO, "The Service doesn't advertise an Implementation Version.");
    }

    if (GLUEService["ComputingEndpoint"]["ServingState"]) {
      target.ServingState = (std::string)GLUEService["ComputingEndpoint"]["ServingState"];
    } else {
      logger.msg(WARNING, "The Service doesn't advertise its Serving State.");
    }

    if (GLUEService["ComputingEndpoint"]["IssuerCA"]) {
      target.IssuerCA = (std::string)GLUEService["ComputingEndpoint"]["IssuerCA"];
    } else {
      logger.msg(INFO, "The Service doesn't advertise its Issuer CA.");
    }

    if (GLUEService["ComputingEndpoint"]["TrustedCA"]) {
      XMLNode n = GLUEService["ComputingEndpoint"]["TrustedCA"];
      while (n) {
        target.TrustedCA.push_back((std::string)n);
        ++n; //The increment operator works in an unusual manner (returns void)
      }
    } else {
      logger.msg(INFO, "The Service doesn't advertise any Trusted CA.");
    }

    if (GLUEService["ComputingEndpoint"]["DowntimeStart"]) {
      target.DowntimeStarts = (std::string)GLUEService["ComputingEndpoint"]["DowntimeStart"];
    } else {
      logger.msg(INFO, "The Service doesn't advertise a Downtime Start.");
    }

    if (GLUEService["ComputingEndpoint"]["DowntimeEnd"]) {
      target.DowntimeEnds = (std::string)GLUEService["ComputingEndpoint"]["DowntimeEnd"];
    } else {
      logger.msg(INFO, "The Service doesn't advertise a Downtime End.");
    }

    if (GLUEService["ComputingEndpoint"]["Staging"]) {
      target.Staging = (std::string)GLUEService["ComputingEndpoint"]["Staging"];
    } else {
      logger.msg(INFO, "The Service doesn't advertise any Staging capabilities.");
    }

    if (GLUEService["ComputingEndpoint"]["JobDescription"]) {
      for (XMLNode n = GLUEService["ComputingEndpoint"]["JobDescription"]; n; ++n)
	target.JobDescriptions.push_back((std::string)n);
    } else {
      logger.msg(INFO, "The Service doesn't advertise what Job Description type it accepts.");
    }

    //Attributes below should possibly consider elements in different places (Service/Endpoint/Share etc).

    if (GLUEService["ComputingEndpoint"]["TotalJobs"]) {
      target.TotalJobs = stringtoi((std::string)GLUEService["ComputingEndpoint"]["TotalJobs"]);
    } else if (GLUEService["TotalJobs"]) {
      target.TotalJobs = stringtoi((std::string)GLUEService["TotalJobs"]);
    } else {
      logger.msg(INFO, "The Service doesn't advertise the Total Number of Jobs.");
    }

    if (GLUEService["ComputingEndpoint"]["RunningJobs"]) {
      target.RunningJobs = stringtoi((std::string)GLUEService["ComputingEndpoint"]["RunningJobs"]);
    } else if (GLUEService["RunningJobs"]) {
      target.RunningJobs = stringtoi((std::string)GLUEService["RunningJobs"]);
    } else {
      logger.msg(INFO, "The Service doesn't advertise the Number of Running Jobs.");
    }

    if (GLUEService["ComputingEndpoint"]["WaitingJobs"]) {
      target.WaitingJobs = stringtoi((std::string)GLUEService["ComputingEndpoint"]["WaitingJobs"]);
    } else if (GLUEService["WaitingJobs"]) {
      target.WaitingJobs = stringtoi((std::string)GLUEService["WaitingJobs"]);
    } else {
      logger.msg(INFO, "The Service doesn't advertise the Number of Waiting Jobs.");
    }

    if (GLUEService["ComputingEndpoint"]["StagingJobs"]) {
      target.StagingJobs = stringtoi((std::string)GLUEService["ComputingEndpoint"]["StagingJobs"]);
    } else if (GLUEService["StagingJobs"]) {
      target.StagingJobs = stringtoi((std::string)GLUEService["StagingJobs"]);
    } else {
      logger.msg(INFO, "The Service doesn't advertise the Number of Staging Jobs.");
    }

    if (GLUEService["ComputingEndpoint"]["SuspendedJobs"]) {
      target.SuspendedJobs = stringtoi((std::string)GLUEService["ComputingEndpoint"]["SuspendedJobs"]);
    } else if (GLUEService["SuspendedJobs"]) {
      target.SuspendedJobs = stringtoi((std::string)GLUEService["SuspendedJobs"]);
    } else {
      logger.msg(INFO, "The Service doesn't advertise the Number of Suspended Jobs.");
    }

    if (GLUEService["ComputingEndpoint"]["PreLRMSWaitingJobs"]) {
      target.PreLRMSWaitingJobs = stringtoi((std::string)GLUEService["ComputingEndpoint"]["PreLRMSWaitingJobs"]);
    } else if (GLUEService["PreLRMSWaitingJobs"]) {
      target.PreLRMSWaitingJobs = stringtoi((std::string)GLUEService["PreLRMSWaitingJobs"]);
    } else {
      logger.msg(INFO, "The Service doesn't advertise the Number of Jobs not yet in the LRMS.");
    }

    if (GLUEService["ComputingEndpoint"]["LocalRunningJobs"]) {
      target.LocalRunningJobs = stringtoi((std::string)GLUEService["ComputingEndpoint"]["LocalRunningJobs"]);
    } else if (GLUEService["LocalRunningJobs"]) {
      target.LocalRunningJobs = stringtoi((std::string)GLUEService["LocalRunningJobs"]);
    } else {
      logger.msg(INFO, "The Service doesn't advertise the Number of Local Running Jobs.");
    }

    if (GLUEService["ComputingEndpoint"]["LocalWaitingJobs"]) {
      target.LocalWaitingJobs = stringtoi((std::string)GLUEService["ComputingEndpoint"]["LocalWaitingJobs"]);
    } else if (GLUEService["LocalWaitingJobs"]) {
      target.LocalWaitingJobs = stringtoi((std::string)GLUEService["LocalWaitingJobs"]);
    } else {
      logger.msg(INFO, "The Service doesn't advertise the Number of Local Waiting Jobs.");
    }

// The following target attributes might be problematic since there might be many shares and
// the relevant one is ill defined.

    if (GLUEService["ComputingShares"]["ComputingShare"]["FreeSlots"]) {
      target.FreeSlots = stringtoi((std::string)GLUEService["ComputingShares"]["ComputingShare"]["FreeSlots"]);
    } else {
      logger.msg(INFO, "The Service doesn't advertise the Number of Free Slots.");
    }

    if (GLUEService["ComputingShares"]["ComputingShare"]["FreeSlotsWithDuration"]) {
      std::string value = (std::string)GLUEService["ComputingShares"]["ComputingShare"]["FreeSlotsWithDuration"];
      std::string::size_type pos = 0;
      do {
	std::string::size_type spacepos = value.find(' ', pos);
	std::string entry;
	if (spacepos == std::string::npos)
	  entry = value.substr(pos);
	else
	  entry = value.substr(pos, spacepos - pos);
	int num_cpus;
	Period time;
	std::string::size_type colonpos = entry.find(':');
	if (colonpos == std::string::npos) {
	  num_cpus = stringtoi(entry);
	  time = LONG_MAX;
	}
	else {
	  num_cpus = stringtoi(entry.substr(0, colonpos));
	  time = stringtoi(entry.substr(colonpos + 1)) * 60;
	}
	target.FreeSlotsWithDuration[time] = num_cpus;
	pos = spacepos;
	if (pos != std::string::npos) pos++;
      } while (pos != std::string::npos);      
    } else {
      logger.msg(INFO, "The Service doesn't advertise the Number of Free Slots with Duration.");
    }

    if (GLUEService["ComputingShares"]["ComputingShare"]["UsedSlots"]) {
      target.UsedSlots = stringtoi((std::string)GLUEService["ComputingShares"]["ComputingShare"]["UsedSlots"]);
    } else {
      logger.msg(INFO, "The Service doesn't advertise the Number of Used Slots.");
    }

    if (GLUEService["ComputingShares"]["ComputingShare"]["RequestedSlots"]) {
      target.RequestedSlots = stringtoi((std::string)GLUEService["ComputingShares"]["ComputingShare"]["RequestedSlots"]);
    } else {
      logger.msg(INFO, "The Service doesn't advertise the Number of Requested Slots.");
    }

    if (GLUEService["ComputingShares"]["ComputingShare"]["MappingQueue"]) {
      target.MappingQueue = (std::string)GLUEService["ComputingShares"]["ComputingShare"]["MappingQueue"];
    } else {
      logger.msg(INFO, "The Service doesn't advertise its Mapping Queue.");
    }

    if (GLUEService["ComputingShares"]["ComputingShare"]["MaxWallTime"]) {
      target.MaxWallTime = (std::string)GLUEService["ComputingShares"]["ComputingShare"]["MaxWallTime"];
    } else {
      logger.msg(INFO, "The Service doesn't advertise a Maximum Wall Time.");
    }

    if (GLUEService["ComputingShares"]["ComputingShare"]["MaxTotalWallTime"]) {
      target.MaxTotalWallTime = (std::string)GLUEService["ComputingShares"]["ComputingShare"]["MaxTotalWallTime"];
    } else {
      logger.msg(INFO, "The Service doesn't advertise a Maximum Total Wall Time.");
    }

    if (GLUEService["ComputingShares"]["ComputingShare"]["MinWallTime"]) {
      target.MinWallTime = (std::string)GLUEService["ComputingShares"]["ComputingShare"]["MinWallTime"];
    } else {
      logger.msg(INFO, "The Service doesn't advertise a Minimum Wall Time.");
    }

    if (GLUEService["ComputingShares"]["ComputingShare"]["DefaultWallTime"]) {
      target.DefaultWallTime = (std::string)GLUEService["ComputingShares"]["ComputingShare"]["DefaultWallTime"];
    } else {
      logger.msg(INFO, "The Service doesn't advertise a Default Wall Time.");
    }

    if (GLUEService["ComputingShares"]["ComputingShare"]["MaxCPUTime"]) {
      target.MaxCPUTime = (std::string)GLUEService["ComputingShares"]["ComputingShare"]["MaxCPUTime"];
    } else {
      logger.msg(INFO, "The Service doesn't advertise a Maximum CPU Time.");
    }

    if (GLUEService["ComputingShares"]["ComputingShare"]["MaxTotalCPUTime"]) {
      target.MaxTotalCPUTime = (std::string)GLUEService["ComputingShares"]["ComputingShare"]["MaxTotalCPUTime"];
    } else {
      logger.msg(INFO, "The Service doesn't advertise a Maximum Total CPU Time.");
    }

    if (GLUEService["ComputingShares"]["ComputingShare"]["MinCPUTime"]) {
      target.MinCPUTime = (std::string)GLUEService["ComputingShares"]["ComputingShare"]["MinCPUTime"];
    } else {
      logger.msg(INFO, "The Service doesn't advertise a Minimum CPU Time.");
    }

    if (GLUEService["ComputingShares"]["ComputingShare"]["DefaultCPUTime"]) {
      target.DefaultCPUTime = (std::string)GLUEService["ComputingShares"]["ComputingShare"]["DefaultCPUTime"];
    } else {
      logger.msg(INFO, "The Service doesn't advertise a Default CPU Time.");
    }

    if (GLUEService["ComputingShares"]["ComputingShare"]["MaxTotalJobs"]) {
      target.MaxTotalJobs = stringtoi((std::string)GLUEService["ComputingShares"]["ComputingShare"]["MaxTotalJobs"]);
    } else {
      logger.msg(INFO, "The Service doesn't advertise the Maximum Number of Total Jobs.");
    }

    if (GLUEService["ComputingShares"]["ComputingShare"]["MaxRunningJobs"]) {
      target.MaxRunningJobs = stringtoi((std::string)GLUEService["ComputingShares"]["ComputingShare"]["MaxRunningJobs"]);
    } else {
      logger.msg(INFO, "The Service doesn't advertise the Maximum Number of Running Jobs.");
    }

    if (GLUEService["ComputingShares"]["ComputingShare"]["MaxWaitingJobs"]) {
      target.MaxWaitingJobs = stringtoi((std::string)GLUEService["ComputingShares"]["ComputingShare"]["MaxWaitingJobs"]);
    } else {
      logger.msg(INFO, "The Service doesn't advertise the Maximum Number of Waiting Jobs.");
    }

    /*
    // This attribute does not exist in the latest Glue draft
    // There is a MainMemorySize in the Execution Environment instead...

    if (GLUEService["ComputingShares"]["ComputingShare"]["NodeMemory"]) {
      target.NodeMemory = stringtoi((std::string)GLUEService["ComputingShares"]["ComputingShare"]["NodeMemory"]);
    } else {
      logger.msg(INFO, "The Service doesn't advertise the Amount of Memory per Node.");
    }
    */

    if (GLUEService["ComputingShares"]["ComputingShare"]["MaxPreLRMSWaitingJobs"]) {
      target.MaxPreLRMSWaitingJobs = stringtoi((std::string)GLUEService["ComputingShares"]["ComputingShare"]["MaxPreLRMSWaitingJobs"]);
    } else {
      logger.msg(INFO, "The Service doesn't advertise the Maximum Number of Jobs not yet in the LRMS.");
    }

    if (GLUEService["ComputingShares"]["ComputingShare"]["MaxUserRunningJobs"]) {
      target.MaxUserRunningJobs = stringtoi((std::string)GLUEService["ComputingShares"]["ComputingShare"]["MaxUserRunningJobs"]);
    } else {
      logger.msg(INFO, "The Service doesn't advertise the Maximum Number of Running Jobs per User.");
    }

    if (GLUEService["ComputingShares"]["ComputingShare"]["MaxSlotsPerJob"]) {
      target.MaxSlotsPerJob = stringtoi((std::string)GLUEService["ComputingShares"]["ComputingShare"]["MaxSlotsPerJob"]);
    } else {
      logger.msg(INFO, "The Service doesn't advertise the Maximum Number of Slots per Job.");
    }

    if (GLUEService["ComputingShares"]["ComputingShare"]["MaxStageInStreams"]) {
      target.MaxStageInStreams = stringtoi((std::string)GLUEService["ComputingShares"]["ComputingShare"]["MaxStageInStreams"]);
    } else {
      logger.msg(INFO, "The Service doesn't advertise the Maximum Number of Streams for Stage-in.");
    }

    if (GLUEService["ComputingShares"]["ComputingShare"]["MaxStageOutStreams"]) {
      target.MaxStageOutStreams = stringtoi((std::string)GLUEService["ComputingShares"]["ComputingShare"]["MaxStageOutStreams"]);
    } else {
      logger.msg(INFO, "The Service doesn't advertise the Maximum Number of Streams for Stage-out.");
    }

    if (GLUEService["ComputingShares"]["ComputingShare"]["SchedulingPolicy"]) {
      target.SchedulingPolicy = (std::string)GLUEService["ComputingShares"]["ComputingShare"]["SchedulingPolicy"];
    } else {
      logger.msg(INFO, "The Service doesn't advertise any Scheduling Policy.");
    }

    if (GLUEService["ComputingShares"]["ComputingShare"]["MaxMainMemory"]) {
      target.MaxMainMemory = stringtoi((std::string)GLUEService["ComputingShares"]["ComputingShare"]["MaxMainMemory"]);
    } else {
      logger.msg(INFO, "The Service doesn't advertise the Maximum Physical Memory for Jobs.");
    }

    if (GLUEService["ComputingShares"]["ComputingShare"]["MaxVirtualMemory"]) {
      target.MaxVirtualMemory = stringtoi((std::string)GLUEService["ComputingShares"]["ComputingShare"]["MaxVirtualMemory"]);
    } else {
      logger.msg(INFO, "The Service doesn't advertise the Maximum Virtual Memory for Jobs.");
    }

    if (GLUEService["ComputingShares"]["ComputingShare"]["MaxDiskSpace"]) {
      target.MaxDiskSpace = stringtoi((std::string)GLUEService["ComputingShares"]["ComputingShare"]["MaxDiskSpace"]);
    } else {
      logger.msg(INFO, "The Service doesn't advertise the Maximum Disk Space for Jobs.");
    }

    if (GLUEService["ComputingShares"]["ComputingShare"]["DefaultStorageService"]) {
      target.DefaultStorageService = (std::string)GLUEService["ComputingShares"]["ComputingShare"]["DefaultStorageService"];
    } else {
      logger.msg(INFO, "The Service doesn't advertise a Default Storage Service.");
    }

    if (GLUEService["ComputingShares"]["ComputingShare"]["Preemption"]) {
      target.Preemption = ((std::string)GLUEService["ComputingShares"]["ComputingShare"]["Preemption"]=="true")?true:false;
    } else {
      logger.msg(INFO, "The Service doesn't advertise whether it supports Preemption.");
    }

    if (GLUEService["ComputingShares"]["ComputingShare"]["EstimatedAverageWaitingTime"]) {
      target.EstimatedAverageWaitingTime = (std::string)GLUEService["ComputingShares"]["ComputingShare"]["EstimatedAverageWaitingTime"];
    } else {
      logger.msg(INFO, "The Service doesn't advertise an Estimated Average Waiting Time.");
    }

    if (GLUEService["ComputingShares"]["ComputingShare"]["EstimatedWorstWaitingTime"]) {
      target.EstimatedWorstWaitingTime = stringtoi((std::string)GLUEService["ComputingShares"]["ComputingShare"]["EstimatedWorstWaitingTime"]);
    } else {
      logger.msg(INFO, "The Service doesn't advertise an Estimated Worst Waiting Time.");
    }

    if (GLUEService["ComputingShares"]["ComputingShare"]["ReservationPolicy"]) {
      target.ReservationPolicy = stringtoi((std::string)GLUEService["ComputingShares"]["ComputingShare"]["ReservationPolicy"]);
    } else {
      logger.msg(INFO, "The Service doesn't advertise a Reservation Policy.");
    }

    if (GLUEService["ComputingManager"]["Type"]) {
      target.ManagerProductName = (std::string)GLUEService["ComputingManager"]["Type"];
    } else {
      logger.msg(INFO, "The Service doesn't advertise its LRMS.");
    }

    if (GLUEService["ComputingManager"]["Reservation"]) {
      target.Reservation = ((std::string)GLUEService["ComputingManager"]["Reservation"]=="true")?true:false;
    } else {
      logger.msg(INFO, "The Service doesn't advertise whether it supports Reservation.");
    }

    if (GLUEService["ComputingManager"]["BulkSubmission"]) {
      target.BulkSubmission = ((std::string)GLUEService["ComputingManager"]["BulkSubmission"]=="true")?true:false;
    } else {
      logger.msg(INFO, "The Service doesn't advertise whether it supports BulkSubmission.");
    }

    if (GLUEService["ComputingManager"]["TotalPhysicalCPUs"]) {
      target.TotalPhysicalCPUs = stringtoi((std::string)GLUEService["ComputingManager"]["TotalPhysicalCPUs"]);
    } else {
      logger.msg(INFO, "The Service doesn't advertise the Total Number of Physical CPUs.");
    }

    if (GLUEService["ComputingManager"]["TotalLogicalCPUs"]) {
      target.TotalLogicalCPUs = stringtoi((std::string)GLUEService["ComputingManager"]["TotalLogicalCPUs"]);
    } else {
      logger.msg(INFO, "The Service doesn't advertise the Total Number of Logical CPUs.");
    }

    if (GLUEService["ComputingManager"]["TotalSlots"]) {
      target.TotalSlots = stringtoi((std::string)GLUEService["ComputingManager"]["TotalSlots"]);
    } else {
      logger.msg(INFO, "The Service doesn't advertise the Total Number of Slots.");
    }

    if (GLUEService["ComputingManager"]["Homogeneous"]) {
      target.Homogeneous = ((std::string)GLUEService["ComputingManager"]["Homogeneous"]=="true")?true:false;
    } else {
      logger.msg(INFO, "The Service doesn't advertise whether it is Homogeneous.");
    }

    if (GLUEService["ComputingManager"]["NetworkInfo"]) {
      for (XMLNode n = GLUEService["ComputingManager"]["NetworkInfo"]; n; ++n)
	target.NetworkInfo.push_back((std::string)n);
    } else {
      logger.msg(INFO, "The Service doesn't advertise any Network Info.");
    }

    if (GLUEService["ComputingManager"]["WorkingAreaShared"]) {
      target.WorkingAreaShared = ((std::string)GLUEService["ComputingManager"]["WorkingAreaShared"]=="true")?true:false;
    } else {
      logger.msg(INFO, "The Service doesn't advertise whether it has the Working Area Shared.");
    }

    if (GLUEService["ComputingManager"]["WorkingAreaFree"]) {
      target.WorkingAreaFree = stringtoi((std::string)GLUEService["ComputingManager"]["WorkingAreaFree"]);
    } else {
      logger.msg(INFO, "The Service doesn't advertise the amount of Free Working Area.");
    }

    if (GLUEService["ComputingManager"]["WorkingAreaTotal"]) {
      target.WorkingAreaTotal = stringtoi((std::string)GLUEService["ComputingManager"]["WorkingAreaTotal"]);
    } else {
      logger.msg(INFO, "The Service doesn't advertise the amount of Total Working Area.");
    }

    if (GLUEService["ComputingManager"]["WorkingAreaLifeTime"]) {
      target.WorkingAreaLifeTime = (std::string)GLUEService["ComputingManager"]["WorkingAreaLifeTime"];
    } else {
      logger.msg(INFO, "The Service doesn't advertise the Lifetime of Working Areas.");
    }

    if (GLUEService["ComputingManager"]["CacheFree"]) {
      target.CacheFree = stringtoi((std::string)GLUEService["ComputingManager"]["CacheFree"]);
    } else {
      logger.msg(INFO, "The Service doesn't advertise the amount of Free Cache.");
    }

    if (GLUEService["ComputingManager"]["CacheTotal"]) {
      target.CacheTotal = stringtoi((std::string)GLUEService["ComputingManager"]["CacheTotal"]);
    } else {
      logger.msg(INFO, "The Service doesn't advertise the amount of Total Cache.");
    }

    if (GLUEService["ComputingManager"]["Benchmark"]) {
      for (XMLNode n = GLUEService["ComputingManager"]["Benchmark"]; n; ++n)
        target.Benchmarks[(std::string)n["Type"]] = Glib::Ascii::strtod((std::string)n["Value"]);
    } else if (NUGCService["benchmark"]) {
      for (XMLNode n = NUGCService["benchmark"]; n; ++n){
        std::string tmp = (std::string)n;
        std::string::size_type at = tmp.find('@');
        if (at == std::string::npos){
          logger.msg(WARNING, "Couldn't parse benchmark string: \"%s\".", tmp);
          continue;
        }
        std::string left = tmp.substr(0, at);
        left.resize(left.find_last_of(alphanum)+1);
        std::string right = tmp.substr(at+1, std::string::npos);
        right.erase(0, right.find_first_of(alphanum));
        try {
          target.Benchmarks[left] = Glib::Ascii::strtod(right);
        } catch (std::runtime_error e) {
          logger.msg(WARNING, "Couldn't parse value \"%s\" of benchmark %s. Parse error: \"%s\".", right, left, e.what());
          //should the something be removed from Benchmarks. Probably it was never added...
        }
        //std::cout << std::endl << "Found benchmark:" << std::endl << "  Type: `" << left << "´" << std::endl << "  Value: `" << right << "´" << std::endl; //Debugging: remove!
      }
    } else {
      logger.msg(INFO, "The Service doesn't advertise any Benchmarks.");
    }

    mom.AddTarget(target);

    delete thrarg;
    mom.RetrieverDone();
  }

} // namespace Arc
