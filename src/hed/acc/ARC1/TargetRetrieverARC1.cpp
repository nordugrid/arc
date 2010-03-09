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

    logger.msg(VERBOSE, "TargetRetriverARC1 initialized with %s service url: %s",
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
    logger.msg(VERBOSE, "Found %u execution services from the index service at %s", services.size(), url.str());

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

        logger.msg(VERBOSE, "Generating A-REX target: %s", target.Cluster.str());

        XMLNode ComputingEndpoint = GLUEService["ComputingEndpoint"];
        if (ComputingEndpoint["HealthState"])
          target.HealthState = (std::string)ComputingEndpoint["HealthState"];
        else
          logger.msg(VERBOSE, "The Service advertises no Health State.");
        if (ComputingEndpoint["HealthStateInfo"])
          target.HealthState = (std::string)ComputingEndpoint["HealthStateInfo"];
        if (GLUEService["Name"])
          target.ServiceName = (std::string)GLUEService["Name"];
        if (ComputingEndpoint["Capability"])
          for (XMLNode n = ComputingEndpoint["Capability"]; n; ++n)
            target.Capability.push_back((std::string)n);
        else if (GLUEService["Capability"])
          for (XMLNode n = GLUEService["Capability"]; n; ++n)
            target.Capability.push_back((std::string)n);
        if (GLUEService["Type"])
          target.ServiceType = (std::string)GLUEService["Type"];
        else
          logger.msg(VERBOSE, "The Service doesn't advertise its Type.");
        if (ComputingEndpoint["QualityLevel"])
          target.QualityLevel = (std::string)ComputingEndpoint["QualityLevel"];
        else if (GLUEService["QualityLevel"])
          target.QualityLevel = (std::string)GLUEService["QualityLevel"];
        else
          logger.msg(VERBOSE, "The Service doesn't advertise its Quality Level.");

        if (ComputingEndpoint["Technology"])
          target.Technology = (std::string)ComputingEndpoint["Technology"];
        if (ComputingEndpoint["InterfaceName"])
          target.InterfaceName = (std::string)ComputingEndpoint["InterfaceName"];
        else if (ComputingEndpoint["Interface"])
          target.InterfaceName = (std::string)ComputingEndpoint["Interface"];
        else
          logger.msg(VERBOSE, "The Service doesn't advertise its Interface.");
        if (ComputingEndpoint["InterfaceVersion"])
          target.InterfaceName = (std::string)ComputingEndpoint["InterfaceVersion"];
        if (ComputingEndpoint["InterfaceExtension"])
          for (XMLNode n = ComputingEndpoint["InterfaceExtension"]; n; ++n)
            target.InterfaceExtension.push_back((std::string)n);
        if (ComputingEndpoint["SupportedProfile"])
          for (XMLNode n = ComputingEndpoint["SupportedProfile"]; n; ++n)
            target.SupportedProfile.push_back((std::string)n);
        if (ComputingEndpoint["Implementor"])
          target.Implementor = (std::string)ComputingEndpoint["Implementor"];
        if (ComputingEndpoint["ImplementationName"])
          if (ComputingEndpoint["ImplementationVersion"])
            target.Implementation =
              Software((std::string)ComputingEndpoint["ImplementationName"],
                       (std::string)ComputingEndpoint["ImplementationVersion"]);
          else
            target.Implementation = Software((std::string)ComputingEndpoint["ImplementationName"]);
        if (ComputingEndpoint["ServingState"])
          target.ServingState = (std::string)ComputingEndpoint["ServingState"];
        else
          logger.msg(VERBOSE, "The Service doesn't advertise its Serving State.");
        if (ComputingEndpoint["IssuerCA"])
          target.IssuerCA = (std::string)ComputingEndpoint["IssuerCA"];
        if (ComputingEndpoint["TrustedCA"]) {
          XMLNode n = ComputingEndpoint["TrustedCA"];
          while (n) {
            target.TrustedCA.push_back((std::string)n);
            ++n; //The increment operator works in an unusual manner (returns void)
          }
        }
        if (ComputingEndpoint["DowntimeStart"])
          target.DowntimeStarts = (std::string)ComputingEndpoint["DowntimeStart"];
        if (ComputingEndpoint["DowntimeEnd"])
          target.DowntimeEnds = (std::string)ComputingEndpoint["DowntimeEnd"];
        if (ComputingEndpoint["Staging"])
          target.Staging = (std::string)ComputingEndpoint["Staging"];
        if (ComputingEndpoint["JobDescription"])
          for (XMLNode n = ComputingEndpoint["JobDescription"]; n; ++n)
            target.JobDescriptions.push_back((std::string)n);

        //Attributes below should possibly consider elements in different places (Service/Endpoint/Share etc).
        if (ComputingEndpoint["TotalJobs"])
          target.TotalJobs = stringtoi((std::string)ComputingEndpoint["TotalJobs"]);
        else if (GLUEService["TotalJobs"])
          target.TotalJobs = stringtoi((std::string)GLUEService["TotalJobs"]);
        if (ComputingEndpoint["RunningJobs"])
          target.RunningJobs = stringtoi((std::string)ComputingEndpoint["RunningJobs"]);
        else if (GLUEService["RunningJobs"])
          target.RunningJobs = stringtoi((std::string)GLUEService["RunningJobs"]);
        if (ComputingEndpoint["WaitingJobs"])
          target.WaitingJobs = stringtoi((std::string)ComputingEndpoint["WaitingJobs"]);
        else if (GLUEService["WaitingJobs"])
          target.WaitingJobs = stringtoi((std::string)GLUEService["WaitingJobs"]);
        if (ComputingEndpoint["StagingJobs"])
          target.StagingJobs = stringtoi((std::string)ComputingEndpoint["StagingJobs"]);
        else if (GLUEService["StagingJobs"])
          target.StagingJobs = stringtoi((std::string)GLUEService["StagingJobs"]);
        if (ComputingEndpoint["SuspendedJobs"])
          target.SuspendedJobs = stringtoi((std::string)ComputingEndpoint["SuspendedJobs"]);
        else if (GLUEService["SuspendedJobs"])
          target.SuspendedJobs = stringtoi((std::string)GLUEService["SuspendedJobs"]);
        if (ComputingEndpoint["PreLRMSWaitingJobs"])
          target.PreLRMSWaitingJobs = stringtoi((std::string)ComputingEndpoint["PreLRMSWaitingJobs"]);
        else if (GLUEService["PreLRMSWaitingJobs"])
          target.PreLRMSWaitingJobs = stringtoi((std::string)GLUEService["PreLRMSWaitingJobs"]);
        if (ComputingEndpoint["LocalRunningJobs"])
          target.LocalRunningJobs = stringtoi((std::string)ComputingEndpoint["LocalRunningJobs"]);
        else if (GLUEService["LocalRunningJobs"])
          target.LocalRunningJobs = stringtoi((std::string)GLUEService["LocalRunningJobs"]);
        if (ComputingEndpoint["LocalWaitingJobs"])
          target.LocalWaitingJobs = stringtoi((std::string)ComputingEndpoint["LocalWaitingJobs"]);
        else if (GLUEService["LocalWaitingJobs"])
          target.LocalWaitingJobs = stringtoi((std::string)GLUEService["LocalWaitingJobs"]);
        if (ComputingEndpoint["LocalSuspendedJobs"])
          target.LocalSuspendedJobs = stringtoi((std::string)ComputingEndpoint["LocalSuspendedJobs"]);
        else if (GLUEService["LocalSuspendedJobs"])
          target.LocalWaitingJobs = stringtoi((std::string)GLUEService["LocalSuspendedJobs"]);

        /*
         * The following target attributes might be problematic since there might be many shares and
         * the relevant one is ill defined. Only the first share is currently stored.
         */
        XMLNode ComputingShare = GLUEService["ComputingShares"]["ComputingShare"];
        if (ComputingShare["FreeSlots"])
          target.FreeSlots = stringtoi((std::string)ComputingShare["FreeSlots"]);
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
        if (ComputingShare["UsedSlots"])
          target.UsedSlots = stringtoi((std::string)ComputingShare["UsedSlots"]);
        if (ComputingShare["RequestedSlots"])
          target.RequestedSlots = stringtoi((std::string)ComputingShare["RequestedSlots"]);
        if (ComputingShare["Name"])
          target.ComputingShareName = (std::string)ComputingShare["Name"];
        if (ComputingShare["MaxWallTime"])
          target.MaxWallTime = (std::string)ComputingShare["MaxWallTime"];
        if (ComputingShare["MaxTotalWallTime"])
          target.MaxTotalWallTime = (std::string)ComputingShare["MaxTotalWallTime"];
        if (ComputingShare["MinWallTime"])
          target.MinWallTime = (std::string)ComputingShare["MinWallTime"];
        if (ComputingShare["DefaultWallTime"])
          target.DefaultWallTime = (std::string)ComputingShare["DefaultWallTime"];
        if (ComputingShare["MaxCPUTime"])
          target.MaxCPUTime = (std::string)ComputingShare["MaxCPUTime"];
        if (ComputingShare["MaxTotalCPUTime"])
          target.MaxTotalCPUTime = (std::string)ComputingShare["MaxTotalCPUTime"];
        if (ComputingShare["MinCPUTime"])
          target.MinCPUTime = (std::string)ComputingShare["MinCPUTime"];
        if (ComputingShare["DefaultCPUTime"])
          target.DefaultCPUTime = (std::string)ComputingShare["DefaultCPUTime"];
        if (ComputingShare["MaxTotalJobs"])
          target.MaxTotalJobs = stringtoi((std::string)ComputingShare["MaxTotalJobs"]);
        if (ComputingShare["MaxRunningJobs"])
          target.MaxRunningJobs = stringtoi((std::string)ComputingShare["MaxRunningJobs"]);
        if (ComputingShare["MaxWaitingJobs"])
          target.MaxWaitingJobs = stringtoi((std::string)ComputingShare["MaxWaitingJobs"]);
        if (ComputingShare["MaxPreLRMSWaitingJobs"])
          target.MaxPreLRMSWaitingJobs = stringtoi((std::string)ComputingShare["MaxPreLRMSWaitingJobs"]);
        if (ComputingShare["MaxUserRunningJobs"])
          target.MaxUserRunningJobs = stringtoi((std::string)ComputingShare["MaxUserRunningJobs"]);
        if (ComputingShare["MaxSlotsPerJob"])
          target.MaxSlotsPerJob = stringtoi((std::string)ComputingShare["MaxSlotsPerJob"]);
        if (ComputingShare["MaxStageInStreams"])
          target.MaxStageInStreams = stringtoi((std::string)ComputingShare["MaxStageInStreams"]);
        if (ComputingShare["MaxStageOutStreams"])
          target.MaxStageOutStreams = stringtoi((std::string)ComputingShare["MaxStageOutStreams"]);
        if (ComputingShare["SchedulingPolicy"])
          target.SchedulingPolicy = (std::string)ComputingShare["SchedulingPolicy"];
        if (ComputingShare["MaxMainMemory"])
          target.MaxMainMemory = stringtoi((std::string)ComputingShare["MaxMainMemory"]);
        if (ComputingShare["MaxVirtualMemory"])
          target.MaxVirtualMemory = stringtoi((std::string)ComputingShare["MaxVirtualMemory"]);
        if (ComputingShare["MaxDiskSpace"])
          target.MaxDiskSpace = stringtoi((std::string)ComputingShare["MaxDiskSpace"]);
        if (ComputingShare["DefaultStorageService"])
          target.DefaultStorageService = (std::string)ComputingShare["DefaultStorageService"];
        if (ComputingShare["Preemption"])
          target.Preemption = ((std::string)ComputingShare["Preemption"] == "true") ? true : false;
        if (ComputingShare["EstimatedAverageWaitingTime"])
          target.EstimatedAverageWaitingTime = (std::string)ComputingShare["EstimatedAverageWaitingTime"];
        if (ComputingShare["EstimatedWorstWaitingTime"])
          target.EstimatedWorstWaitingTime = stringtoi((std::string)ComputingShare["EstimatedWorstWaitingTime"]);
        if (ComputingShare["ReservationPolicy"])
          target.ReservationPolicy = stringtoi((std::string)ComputingShare["ReservationPolicy"]);

        XMLNode ComputingManager = GLUEService["ComputingManager"];
        if (ComputingManager["ProductName"])
          target.ManagerProductName = (std::string)ComputingManager["ProductName"];
        else if (ComputingManager["Type"]) // is this non-standard fallback needed?
          target.ManagerProductName = (std::string)ComputingManager["Type"];
        if (ComputingManager["ProductVersion"])
          target.ManagerProductName = (std::string)ComputingManager["ProductVersion"];
        if (ComputingManager["Reservation"])
          target.Reservation = ((std::string)ComputingManager["Reservation"] == "true") ? true : false;
        if (ComputingManager["BulkSubmission"])
          target.BulkSubmission = ((std::string)ComputingManager["BulkSubmission"] == "true") ? true : false;
        if (ComputingManager["TotalPhysicalCPUs"])
          target.TotalPhysicalCPUs = stringtoi((std::string)ComputingManager["TotalPhysicalCPUs"]);
        if (ComputingManager["TotalLogicalCPUs"])
          target.TotalLogicalCPUs = stringtoi((std::string)ComputingManager["TotalLogicalCPUs"]);
        if (ComputingManager["TotalSlots"])
          target.TotalSlots = stringtoi((std::string)ComputingManager["TotalSlots"]);
        if (ComputingManager["Homogeneous"])
          target.Homogeneous = ((std::string)ComputingManager["Homogeneous"] == "true") ? true : false;
        if (ComputingManager["NetworkInfo"])
          for (XMLNode n = ComputingManager["NetworkInfo"]; n; ++n)
            target.NetworkInfo.push_back((std::string)n);
        if (ComputingManager["WorkingAreaShared"])
          target.WorkingAreaShared = ((std::string)ComputingManager["WorkingAreaShared"] == "true") ? true : false;
        if (ComputingManager["WorkingAreaFree"])
          target.WorkingAreaFree = stringtoi((std::string)ComputingManager["WorkingAreaFree"]);
        if (ComputingManager["WorkingAreaTotal"])
          target.WorkingAreaTotal = stringtoi((std::string)ComputingManager["WorkingAreaTotal"]);
        if (ComputingManager["WorkingAreaLifeTime"])
          target.WorkingAreaLifeTime = (std::string)ComputingManager["WorkingAreaLifeTime"];
        if (ComputingManager["CacheFree"])
          target.CacheFree = stringtoi((std::string)ComputingManager["CacheFree"]);
        if (ComputingManager["CacheTotal"])
          target.CacheTotal = stringtoi((std::string)ComputingManager["CacheTotal"]);
        for (XMLNode n = ComputingManager["Benchmark"]; n; ++n) {
          double value;
          if (n["Type"] && n["Value"] &&
              stringto((std::string)n["Value"], value))
            target.Benchmarks[(std::string)n["Type"]] = value;
          else {
            logger.msg(VERBOSE, "Couldn't parse benchmark XML:\n%s", (std::string)n);
            continue;
          }
        }
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

        mom.AddTarget(target);
      }
    }
    else if (targetType == 1) {
      /*
       * If errors are encountered here there cannot be returned in usual manner
       * since this function should run in a separate thread. There might be a
       * need for reporting errors, maybe a bit flag of some kind.
       */
      DataHandle dir_url(url, usercfg);
      if (!dir_url) {
        logger.msg(INFO, "Failed retrieving job IDs: Unsupported url (%s) given", url.str());
        mom.RetrieverDone();
        return;
      }

      dir_url->SetSecure(false);
      std::list<FileInfo> files;
      if (!dir_url->ListFiles(files, false, false, false)) {
        if (files.size() == 0) {
          logger.msg(INFO, "Failed retrieving job IDs");
          mom.RetrieverDone();
          return;
        }
        logger.msg(VERBOSE, "Error encoutered during job ID retrieval. All job IDs might not have been retrieved");
      }

      for (std::list<FileInfo>::iterator file = files.begin();
           file != files.end(); file++) {
        NS ns;
        XMLNode info(ns, "Job");
        info.NewChild("JobID") = (std::string)url.str() + "/" + file->GetName();
        info.NewChild("Flavour") = "ARC1";
        info.NewChild("Cluster") = url.str();
        mom.AddJob(info);
      }
    }

    delete thrarg;
    mom.RetrieverDone();
  }

} // namespace Arc
