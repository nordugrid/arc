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
#include <arc/message/MCC.h>
#include <arc/data/DataHandle.h>

#include "AREXClient.h"
#include "TargetRetrieverARC1.h"

namespace Arc {

  struct ThreadArg {
    TargetGenerator *mom;
    const UserConfig *usercfg;
    URL url;
    int targetType;
    int detailLevel;
  };

  ThreadArg* TargetRetrieverARC1::CreateThreadArg(TargetGenerator& mom,
                                                  int targetType,
                                                  int detailLevel) {
    ThreadArg *arg = new ThreadArg;
    arg->mom = &mom;
    arg->usercfg = &usercfg;
    arg->url = url;
    arg->targetType = targetType;
    arg->detailLevel = detailLevel;
    return arg;
  }

  Logger TargetRetrieverARC1::logger(TargetRetriever::logger, "ARC1");

  TargetRetrieverARC1::TargetRetrieverARC1(const UserConfig& usercfg,
                                           const URL& url, ServiceType st)
    : TargetRetriever(usercfg, url, st, "ARC1") {}

  TargetRetrieverARC1::~TargetRetrieverARC1() {}

  Plugin* TargetRetrieverARC1::Instance(PluginArgument *arg) {
    TargetRetrieverPluginArgument *trarg =
      dynamic_cast<TargetRetrieverPluginArgument*>(arg);
    if (!trarg)
      return NULL;
    return new TargetRetrieverARC1(*trarg, *trarg, *trarg);
  }

  void TargetRetrieverARC1::GetTargets(TargetGenerator& mom, int targetType,
                                       int detailLevel) {

    logger.msg(INFO, "TargetRetriverARC1 initialized with %s service url: %s",
               tostring(serviceType), url.str());

    switch (serviceType) {
    case COMPUTING:
      if (mom.AddService(url)) {
        ThreadArg *arg = CreateThreadArg(mom, targetType, detailLevel);
        if (!CreateThreadFunction(&InterrogateTarget, arg)) {
          delete arg;
          mom.RetrieverDone();
        }
      }
      break;
    case INDEX:
      if (mom.AddIndexServer(url)) {
        ThreadArg *arg = CreateThreadArg(mom, targetType, detailLevel);
        if (!CreateThreadFunction(&QueryIndex, arg)) {
          delete arg;
          mom.RetrieverDone();
        }
      }
      break;
    }
  }

  void TargetRetrieverARC1::QueryIndex(void *arg) {
    ThreadArg *thrarg = (ThreadArg*)arg;
    TargetGenerator& mom = *thrarg->mom;
    const UserConfig& usercfg = *thrarg->usercfg;
    URL& url = thrarg->url;
    MCCConfig cfg;
    usercfg.ApplyToConfig(cfg);
    AREXClient ac(url, cfg, usercfg.Timeout());
    std::list< std::pair<URL, ServiceType> > services;
    if (!ac.listServicesFromISIS(services)) {
      delete thrarg;
      mom.RetrieverDone();
      return;
    }
    logger.msg(INFO, "Found %u execution services from the index service at %s", services.size(), url.str());

    for (std::list< std::pair<URL, ServiceType> >::iterator it = services.begin(); it != services.end(); it++) {
      TargetRetrieverARC1 r(usercfg, it->first, it->second);
      r.GetTargets(mom, thrarg->targetType, thrarg->detailLevel);
    }

    delete thrarg;
    mom.RetrieverDone();
  }

  void TargetRetrieverARC1::InterrogateTarget(void *arg) {
    ThreadArg *thrarg = (ThreadArg*)arg;
    TargetGenerator& mom = *thrarg->mom;
    const UserConfig& usercfg = *thrarg->usercfg;
    int targetType = thrarg->targetType;
    URL& url = thrarg->url;

    if (targetType == 0) {
      MCCConfig cfg;
      usercfg.ApplyToConfig(cfg);
      AREXClient ac(url, cfg, usercfg.Timeout());
      XMLNode servicesQueryResponse;
      if (!ac.sstat(servicesQueryResponse)) {
        delete thrarg;
        mom.RetrieverDone();
        return;
      }

      for (XMLNode GLUEService = servicesQueryResponse["ComputingService"];
           GLUEService; ++GLUEService) {
        ExecutionTarget target;

        target.GridFlavour = "ARC1";
        target.Cluster = thrarg->url;
        target.url = url;
        target.InterfaceName = "BES";
        target.Implementor = "NorduGrid";

        target.DomainName = url.Host();

        XMLNode ComputingEndpoint = GLUEService["ComputingEndpoint"];
        if (ComputingEndpoint["HealthState"])
          target.HealthState = (std::string)ComputingEndpoint["HealthState"];
        else
          logger.msg(WARNING, "The Service advertises no Health State.");

        if (GLUEService["Name"])
          target.ServiceName = (std::string)GLUEService["Name"];
        else
          logger.msg(INFO, "The Service doesn't advertise its Name.");

        if (GLUEService["Capability"])
          for (XMLNode n = GLUEService["Capability"]; n; ++n)
            target.Capability.push_back((std::string)n);
        else
          logger.msg(INFO, "The Service doesn't advertise its Capability.");

        if (GLUEService["Type"])
          target.ServiceType = (std::string)GLUEService["Type"];
        else
          logger.msg(WARNING, "The Service doesn't advertise its Type.");

        if (GLUEService["QualityLevel"])
          target.QualityLevel = (std::string)GLUEService["QualityLevel"];
        else
          logger.msg(WARNING, "The Service doesn't advertise its Quality Level.");

        if (ComputingEndpoint["Technology"])
          target.Technology = (std::string)ComputingEndpoint["Technology"];
        else
          logger.msg(INFO, "The Service doesn't advertise its Technology.");

        if (ComputingEndpoint["InterfaceName"])
          target.InterfaceName = (std::string)ComputingEndpoint["InterfaceName"];
        else if (ComputingEndpoint["Interface"])
          target.InterfaceName = (std::string)ComputingEndpoint["Interface"];
        else
          logger.msg(WARNING, "The Service doesn't advertise its Interface.");

        if (ComputingEndpoint["InterfaceExtension"])
          for (XMLNode n = ComputingEndpoint["InterfaceExtension"]; n; ++n)
            target.InterfaceExtension.push_back((std::string)n);
        else
          logger.msg(INFO, "The Service doesn't advertise an Interface Extension.");

        if (ComputingEndpoint["SupportedProfile"])
          for (XMLNode n = ComputingEndpoint["SupportedProfile"]; n; ++n)
            target.SupportedProfile.push_back((std::string)n);
        else
          logger.msg(INFO, "The Service doesn't advertise any Supported Profile.");

        if (ComputingEndpoint["ImplementationName"])
          if (ComputingEndpoint["ImplementationVersion"])
            target.Implementation =
              Software((std::string)ComputingEndpoint["ImplementationName"],
                       (std::string)ComputingEndpoint["ImplementationVersion"]);
          else {
            target.Implementation = Software((std::string)ComputingEndpoint["ImplementationName"]);
            logger.msg(INFO, "The Service doesn't advertise an Implementation Version.");
          }
        else
          logger.msg(INFO, "The Service doesn't advertise an Implementation Name.");


        if (ComputingEndpoint["ServingState"])
          target.ServingState = (std::string)ComputingEndpoint["ServingState"];
        else
          logger.msg(WARNING, "The Service doesn't advertise its Serving State.");

        if (ComputingEndpoint["IssuerCA"])
          target.IssuerCA = (std::string)ComputingEndpoint["IssuerCA"];
        else
          logger.msg(INFO, "The Service doesn't advertise its Issuer CA.");

        if (ComputingEndpoint["TrustedCA"]) {
          XMLNode n = ComputingEndpoint["TrustedCA"];
          while (n) {
            target.TrustedCA.push_back((std::string)n);
            ++n; //The increment operator works in an unusual manner (returns void)
          }
        }
        else
          logger.msg(INFO, "The Service doesn't advertise any Trusted CA.");

        if (ComputingEndpoint["DowntimeStart"])
          target.DowntimeStarts = (std::string)ComputingEndpoint["DowntimeStart"];
        else
          logger.msg(INFO, "The Service doesn't advertise a Downtime Start.");

        if (ComputingEndpoint["DowntimeEnd"])
          target.DowntimeEnds = (std::string)ComputingEndpoint["DowntimeEnd"];
        else
          logger.msg(INFO, "The Service doesn't advertise a Downtime End.");

        if (ComputingEndpoint["Staging"])
          target.Staging = (std::string)ComputingEndpoint["Staging"];
        else
          logger.msg(INFO, "The Service doesn't advertise any Staging capabilities.");

        if (ComputingEndpoint["JobDescription"])
          for (XMLNode n = ComputingEndpoint["JobDescription"]; n; ++n)
            target.JobDescriptions.push_back((std::string)n);
        else
          logger.msg(INFO, "The Service doesn't advertise what Job Description type it accepts.");

        //Attributes below should possibly consider elements in different places (Service/Endpoint/Share etc).

        if (ComputingEndpoint["TotalJobs"])
          target.TotalJobs = stringtoi((std::string)ComputingEndpoint["TotalJobs"]);
        else if (GLUEService["TotalJobs"])
          target.TotalJobs = stringtoi((std::string)GLUEService["TotalJobs"]);
        else
          logger.msg(INFO, "The Service doesn't advertise the Total Number of Jobs.");

        if (ComputingEndpoint["RunningJobs"])
          target.RunningJobs = stringtoi((std::string)ComputingEndpoint["RunningJobs"]);
        else if (GLUEService["RunningJobs"])
          target.RunningJobs = stringtoi((std::string)GLUEService["RunningJobs"]);
        else
          logger.msg(INFO, "The Service doesn't advertise the Number of Running Jobs.");

        if (ComputingEndpoint["WaitingJobs"])
          target.WaitingJobs = stringtoi((std::string)ComputingEndpoint["WaitingJobs"]);
        else if (GLUEService["WaitingJobs"])
          target.WaitingJobs = stringtoi((std::string)GLUEService["WaitingJobs"]);
        else
          logger.msg(INFO, "The Service doesn't advertise the Number of Waiting Jobs.");

        if (ComputingEndpoint["StagingJobs"])
          target.StagingJobs = stringtoi((std::string)ComputingEndpoint["StagingJobs"]);
        else if (GLUEService["StagingJobs"])
          target.StagingJobs = stringtoi((std::string)GLUEService["StagingJobs"]);
        else
          logger.msg(INFO, "The Service doesn't advertise the Number of Staging Jobs.");

        if (ComputingEndpoint["SuspendedJobs"])
          target.SuspendedJobs = stringtoi((std::string)ComputingEndpoint["SuspendedJobs"]);
        else if (GLUEService["SuspendedJobs"])
          target.SuspendedJobs = stringtoi((std::string)GLUEService["SuspendedJobs"]);
        else
          logger.msg(INFO, "The Service doesn't advertise the Number of Suspended Jobs.");

        if (ComputingEndpoint["PreLRMSWaitingJobs"])
          target.PreLRMSWaitingJobs = stringtoi((std::string)ComputingEndpoint["PreLRMSWaitingJobs"]);
        else if (GLUEService["PreLRMSWaitingJobs"])
          target.PreLRMSWaitingJobs = stringtoi((std::string)GLUEService["PreLRMSWaitingJobs"]);
        else
          logger.msg(INFO, "The Service doesn't advertise the Number of Jobs not yet in the LRMS.");

        if (ComputingEndpoint["LocalRunningJobs"])
          target.LocalRunningJobs = stringtoi((std::string)ComputingEndpoint["LocalRunningJobs"]);
        else if (GLUEService["LocalRunningJobs"])
          target.LocalRunningJobs = stringtoi((std::string)GLUEService["LocalRunningJobs"]);
        else
          logger.msg(INFO, "The Service doesn't advertise the Number of Local Running Jobs.");

        if (ComputingEndpoint["LocalWaitingJobs"])
          target.LocalWaitingJobs = stringtoi((std::string)ComputingEndpoint["LocalWaitingJobs"]);
        else if (GLUEService["LocalWaitingJobs"])
          target.LocalWaitingJobs = stringtoi((std::string)GLUEService["LocalWaitingJobs"]);
        else
          logger.msg(INFO, "The Service doesn't advertise the Number of Local Waiting Jobs.");

        /*
         * The following target attributes might be problematic since there
         * might be many shares and the relevant one is ill defined. Currently
         * only the first ComputingShare is used.
         */
        XMLNode ComputingShare = ComputingShare;
        if (ComputingShare["FreeSlots"])
          target.FreeSlots = stringtoi((std::string)ComputingShare["FreeSlots"]);
        else
          logger.msg(INFO, "The Service doesn't advertise the Number of Free Slots.");

        if (ComputingShare["FreeSlotsWithDuration"]) {
          std::string value = (std::string)ComputingShare["FreeSlotsWithDuration"];
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
            if (pos != std::string::npos)
              pos++;
          } while (pos != std::string::npos);
        }
        else
          logger.msg(INFO, "The Service doesn't advertise the Number of Free Slots with Duration.");

        if (ComputingShare["UsedSlots"])
          target.UsedSlots = stringtoi((std::string)ComputingShare["UsedSlots"]);
        else
          logger.msg(INFO, "The Service doesn't advertise the Number of Used Slots.");

        if (ComputingShare["RequestedSlots"])
          target.RequestedSlots = stringtoi((std::string)ComputingShare["RequestedSlots"]);
        else
          logger.msg(INFO, "The Service doesn't advertise the Number of Requested Slots.");

        if (ComputingShare["MappingQueue"])
          target.MappingQueue = (std::string)ComputingShare["MappingQueue"];
        else
          logger.msg(INFO, "The Service doesn't advertise its Mapping Queue.");

        if (ComputingShare["MaxWallTime"])
          target.MaxWallTime = (std::string)ComputingShare["MaxWallTime"];
        else
          logger.msg(INFO, "The Service doesn't advertise a Maximum Wall Time.");

        if (ComputingShare["MaxTotalWallTime"])
          target.MaxTotalWallTime = (std::string)ComputingShare["MaxTotalWallTime"];
        else
          logger.msg(INFO, "The Service doesn't advertise a Maximum Total Wall Time.");

        if (ComputingShare["MinWallTime"])
          target.MinWallTime = (std::string)ComputingShare["MinWallTime"];
        else
          logger.msg(INFO, "The Service doesn't advertise a Minimum Wall Time.");

        if (ComputingShare["DefaultWallTime"])
          target.DefaultWallTime = (std::string)ComputingShare["DefaultWallTime"];
        else
          logger.msg(INFO, "The Service doesn't advertise a Default Wall Time.");

        if (ComputingShare["MaxCPUTime"])
          target.MaxCPUTime = (std::string)ComputingShare["MaxCPUTime"];
        else
          logger.msg(INFO, "The Service doesn't advertise a Maximum CPU Time.");

        if (ComputingShare["MaxTotalCPUTime"])
          target.MaxTotalCPUTime = (std::string)ComputingShare["MaxTotalCPUTime"];
        else
          logger.msg(INFO, "The Service doesn't advertise a Maximum Total CPU Time.");

        if (ComputingShare["MinCPUTime"])
          target.MinCPUTime = (std::string)ComputingShare["MinCPUTime"];
        else
          logger.msg(INFO, "The Service doesn't advertise a Minimum CPU Time.");

        if (ComputingShare["DefaultCPUTime"])
          target.DefaultCPUTime = (std::string)ComputingShare["DefaultCPUTime"];
        else
          logger.msg(INFO, "The Service doesn't advertise a Default CPU Time.");

        if (ComputingShare["MaxTotalJobs"])
          target.MaxTotalJobs = stringtoi((std::string)ComputingShare["MaxTotalJobs"]);
        else
          logger.msg(INFO, "The Service doesn't advertise the Maximum Number of Total Jobs.");

        if (ComputingShare["MaxRunningJobs"])
          target.MaxRunningJobs = stringtoi((std::string)ComputingShare["MaxRunningJobs"]);
        else
          logger.msg(INFO, "The Service doesn't advertise the Maximum Number of Running Jobs.");

        if (ComputingShare["MaxWaitingJobs"])
          target.MaxWaitingJobs = stringtoi((std::string)ComputingShare["MaxWaitingJobs"]);
        else
          logger.msg(INFO, "The Service doesn't advertise the Maximum Number of Waiting Jobs.");

        /*
           // This attribute does not exist in the latest Glue draft
           // There is a MainMemorySize in the Execution Environment instead...

           if (ComputingShare["NodeMemory"]) {
           target.NodeMemory = stringtoi((std::string)ComputingShare["NodeMemory"]);
           } else {
           logger.msg(INFO, "The Service doesn't advertise the Amount of Memory per Node.");
           }
         */

        if (ComputingShare["MaxPreLRMSWaitingJobs"])
          target.MaxPreLRMSWaitingJobs = stringtoi((std::string)ComputingShare["MaxPreLRMSWaitingJobs"]);
        else
          logger.msg(INFO, "The Service doesn't advertise the Maximum Number of Jobs not yet in the LRMS.");

        if (ComputingShare["MaxUserRunningJobs"])
          target.MaxUserRunningJobs = stringtoi((std::string)ComputingShare["MaxUserRunningJobs"]);
        else
          logger.msg(INFO, "The Service doesn't advertise the Maximum Number of Running Jobs per User.");

        if (ComputingShare["MaxSlotsPerJob"])
          target.MaxSlotsPerJob = stringtoi((std::string)ComputingShare["MaxSlotsPerJob"]);
        else
          logger.msg(INFO, "The Service doesn't advertise the Maximum Number of Slots per Job.");

        if (ComputingShare["MaxStageInStreams"])
          target.MaxStageInStreams = stringtoi((std::string)ComputingShare["MaxStageInStreams"]);
        else
          logger.msg(INFO, "The Service doesn't advertise the Maximum Number of Streams for Stage-in.");

        if (ComputingShare["MaxStageOutStreams"])
          target.MaxStageOutStreams = stringtoi((std::string)ComputingShare["MaxStageOutStreams"]);
        else
          logger.msg(INFO, "The Service doesn't advertise the Maximum Number of Streams for Stage-out.");

        if (ComputingShare["SchedulingPolicy"])
          target.SchedulingPolicy = (std::string)ComputingShare["SchedulingPolicy"];
        else
          logger.msg(INFO, "The Service doesn't advertise any Scheduling Policy.");

        if (ComputingShare["MaxMainMemory"])
          target.MaxMainMemory = stringtoi((std::string)ComputingShare["MaxMainMemory"]);
        else
          logger.msg(INFO, "The Service doesn't advertise the Maximum Physical Memory for Jobs.");

        if (ComputingShare["MaxVirtualMemory"])
          target.MaxVirtualMemory = stringtoi((std::string)ComputingShare["MaxVirtualMemory"]);
        else
          logger.msg(INFO, "The Service doesn't advertise the Maximum Virtual Memory for Jobs.");

        if (ComputingShare["MaxDiskSpace"])
          target.MaxDiskSpace = stringtoi((std::string)ComputingShare["MaxDiskSpace"]);
        else
          logger.msg(INFO, "The Service doesn't advertise the Maximum Disk Space for Jobs.");

        if (ComputingShare["DefaultStorageService"])
          target.DefaultStorageService = (std::string)ComputingShare["DefaultStorageService"];
        else
          logger.msg(INFO, "The Service doesn't advertise a Default Storage Service.");

        if (ComputingShare["Preemption"])
          target.Preemption = ((std::string)ComputingShare["Preemption"] == "true") ? true : false;
        else
          logger.msg(INFO, "The Service doesn't advertise whether it supports Preemption.");

        if (ComputingShare["EstimatedAverageWaitingTime"])
          target.EstimatedAverageWaitingTime = (std::string)ComputingShare["EstimatedAverageWaitingTime"];
        else
          logger.msg(INFO, "The Service doesn't advertise an Estimated Average Waiting Time.");

        if (ComputingShare["EstimatedWorstWaitingTime"])
          target.EstimatedWorstWaitingTime = stringtoi((std::string)ComputingShare["EstimatedWorstWaitingTime"]);
        else
          logger.msg(INFO, "The Service doesn't advertise an Estimated Worst Waiting Time.");

        if (ComputingShare["ReservationPolicy"])
          target.ReservationPolicy = stringtoi((std::string)ComputingShare["ReservationPolicy"]);
        else
          logger.msg(INFO, "The Service doesn't advertise a Reservation Policy.");

        XMLNode ComputingManager = GLUEService["ComputingManager"];
        if (ComputingManager["Type"])
          target.ManagerProductName = (std::string)ComputingManager["Type"];
        else
          logger.msg(INFO, "The Service doesn't advertise its LRMS.");

        if (ComputingManager["Reservation"])
          target.Reservation = ((std::string)ComputingManager["Reservation"] == "true") ? true : false;
        else
          logger.msg(INFO, "The Service doesn't advertise whether it supports Reservation.");

        if (ComputingManager["BulkSubmission"])
          target.BulkSubmission = ((std::string)ComputingManager["BulkSubmission"] == "true") ? true : false;
        else
          logger.msg(INFO, "The Service doesn't advertise whether it supports BulkSubmission.");

        if (ComputingManager["TotalPhysicalCPUs"])
          target.TotalPhysicalCPUs = stringtoi((std::string)ComputingManager["TotalPhysicalCPUs"]);
        else
          logger.msg(INFO, "The Service doesn't advertise the Total Number of Physical CPUs.");

        if (ComputingManager["TotalLogicalCPUs"])
          target.TotalLogicalCPUs = stringtoi((std::string)ComputingManager["TotalLogicalCPUs"]);
        else
          logger.msg(INFO, "The Service doesn't advertise the Total Number of Logical CPUs.");

        if (ComputingManager["TotalSlots"])
          target.TotalSlots = stringtoi((std::string)ComputingManager["TotalSlots"]);
        else
          logger.msg(INFO, "The Service doesn't advertise the Total Number of Slots.");

        if (ComputingManager["Homogeneous"])
          target.Homogeneous = ((std::string)ComputingManager["Homogeneous"] == "true") ? true : false;
        else
          logger.msg(INFO, "The Service doesn't advertise whether it is Homogeneous.");

        if (ComputingManager["NetworkInfo"])
          for (XMLNode n = ComputingManager["NetworkInfo"]; n; ++n)
            target.NetworkInfo.push_back((std::string)n);
        else
          logger.msg(INFO, "The Service doesn't advertise any Network Info.");

        if (ComputingManager["WorkingAreaShared"])
          target.WorkingAreaShared = ((std::string)ComputingManager["WorkingAreaShared"] == "true") ? true : false;
        else
          logger.msg(INFO, "The Service doesn't advertise whether it has the Working Area Shared.");

        if (ComputingManager["WorkingAreaFree"])
          target.WorkingAreaFree = stringtoi((std::string)ComputingManager["WorkingAreaFree"]);
        else
          logger.msg(INFO, "The Service doesn't advertise the amount of Free Working Area.");

        if (ComputingManager["WorkingAreaTotal"])
          target.WorkingAreaTotal = stringtoi((std::string)ComputingManager["WorkingAreaTotal"]);
        else
          logger.msg(INFO, "The Service doesn't advertise the amount of Total Working Area.");

        if (ComputingManager["WorkingAreaLifeTime"])
          target.WorkingAreaLifeTime = (std::string)ComputingManager["WorkingAreaLifeTime"];
        else
          logger.msg(INFO, "The Service doesn't advertise the Lifetime of Working Areas.");

        if (ComputingManager["CacheFree"])
          target.CacheFree = stringtoi((std::string)ComputingManager["CacheFree"]);
        else
          logger.msg(INFO, "The Service doesn't advertise the amount of Free Cache.");

        if (ComputingManager["CacheTotal"])
          target.CacheTotal = stringtoi((std::string)ComputingManager["CacheTotal"]);
        else
          logger.msg(INFO, "The Service doesn't advertise the amount of Total Cache.");

        if (ComputingManager["Benchmark"])
          for (XMLNode n = ComputingManager["Benchmark"]; n; ++n) {
            double value;
            if (n["Type"] && n["Value"] &&
                stringto((std::string)n["Value"], value))
              target.Benchmarks[(std::string)n["Type"]] = value;
            else {
              logger.msg(WARNING, "Couldn't parse benchmark XML:\n%s", (std::string)n);
              continue;
            }
          }
        else
          logger.msg(INFO, "The Service doesn't advertise any Benchmarks.");

        if (ComputingManager["ApplicationEnvironments"]["ApplicationEnvironment"])
          for (XMLNode n = ComputingManager["ApplicationEnvironments"]["ApplicationEnvironment"]; n; ++n) {
            ApplicationEnvironment ae((std::string)n["AppName"], (std::string)n["AppVersion"]);
            ae.State = (std::string)n["State"];
            if (n["FreeSlots"])
              ae.FreeSlots = stringtoi((std::string)n["FreeSlots"]);
            else
              ae.FreeSlots = target.FreeSlots;
            if (n["FreeJobs"])
              ae.FreeJobs = stringtoi((std::string)n["FreeJobs"]);
            else
              ae.FreeJobs = -1;
            if (n["FreeUserSeats"])
              ae.FreeUserSeats = stringtoi((std::string)n["FreeUserSeats"]);
            else
              ae.FreeUserSeats = -1;
            target.ApplicationEnvironments.push_back(ae);
          }
        else
          logger.msg(INFO, "The Service doesn't advertise any Application Environments.");
        mom.AddTarget(target);
      }
    }
    else if (targetType == 1) {
      DataHandle dir_url(url, usercfg);
      if (!dir_url) {
        logger.msg(ERROR, "Unsupported url given");
        return;
      }

      dir_url->SetSecure(false);
      std::list<FileInfo> files;
      if (!dir_url->ListFiles(files, false, false, false)) {
        if (files.size() == 0) {
          logger.msg(ERROR, "Failed listing metafiles");
          return;
        }
        logger.msg(INFO, "Warning: "
                   "Failed listing metafiles but some information is obtained");
      }

      for (std::list<FileInfo>::iterator file = files.begin();
           file != files.end(); file++) {
        NS ns;
        XMLNode info(ns, "Job");
        info.NewChild("JobID") =
          (std::string)url.str() + "/" + file->GetName();
        info.NewChild("Flavour") = "ARC1";
        info.NewChild("Cluster") = url.str();
        mom.AddJob(info);
      }
    }

    delete thrarg;
    mom.RetrieverDone();
  }

} // namespace Arc
