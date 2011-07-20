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
#include <arc/client/TargetGenerator.h>
#include <arc/message/MCC.h>
#include <arc/data/DataHandle.h>

#include "AREXClient.h"
#include "TargetRetrieverARC1.h"

namespace Arc {

  class ThreadArgARC1 {
  public:
    TargetGenerator *mom;
    const UserConfig *usercfg;
    URL url;
    bool isExecutionTarget;
    std::string flavour;
  };

  ThreadArgARC1* TargetRetrieverARC1::CreateThreadArg(TargetGenerator& mom,
                                                  bool isExecutionTarget) {
    ThreadArgARC1 *arg = new ThreadArgARC1;
    arg->mom = &mom;
    arg->usercfg = &usercfg;
    arg->url = url;
    arg->isExecutionTarget = isExecutionTarget;
    arg->flavour = flavour;
    return arg;
  }

  Logger TargetRetrieverARC1::logger(Logger::getRootLogger(),
                                     "TargetRetriever.ARC1");

  static URL CreateURL(std::string service, ServiceType /* st */) {
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

  TargetRetrieverARC1::TargetRetrieverARC1(const UserConfig& usercfg,
                                           const std::string& service,
                                           ServiceType st,
                                           const std::string& flav)
    : TargetRetriever(usercfg, CreateURL(service, st), st, flav) {}

  TargetRetrieverARC1::~TargetRetrieverARC1() {}

  Plugin* TargetRetrieverARC1::Instance(PluginArgument *arg) {
    TargetRetrieverPluginArgument *trarg =
      dynamic_cast<TargetRetrieverPluginArgument*>(arg);
    if (!trarg)
      return NULL;
    return new TargetRetrieverARC1(*trarg, *trarg, *trarg);
  }

  void TargetRetrieverARC1::GetExecutionTargets(TargetGenerator& mom) {
    logger.msg(VERBOSE, "TargetRetriver%s initialized with %s service url: %s",
               flavour, tostring(serviceType), url.str());
    if(!url) return;

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

    if (serviceType == INDEX && flavour != "ARC1") return;
    if (serviceType == COMPUTING && mom.AddService(flavour, url) ||
        serviceType == INDEX     && mom.AddIndexServer(flavour, url)) {
      ThreadArgARC1 *arg = CreateThreadArg(mom, true);
      if (!CreateThreadFunction((serviceType == COMPUTING ?
                                 &InterrogateTarget : &QueryIndex),
                                arg, &(mom.ServiceCounter()))) {
        delete arg;
      }
    }
  }

  void TargetRetrieverARC1::GetJobs(TargetGenerator& mom) {
    logger.msg(VERBOSE, "TargetRetriver%s initialized with %s service url: %s",
               flavour, tostring(serviceType), url.str());
    if(!url) return;

    if(flavour != "ARC1") return;
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
      ThreadArgARC1 *arg = CreateThreadArg(mom, false);
      if (!CreateThreadFunction((serviceType == COMPUTING ?
                                 &InterrogateTarget : &QueryIndex),
                                arg, &(mom.ServiceCounter()))) {
        delete arg;
      }
    }
  }

  void TargetRetrieverARC1::QueryIndex(void *arg) {
    ThreadArgARC1 *thrarg = (ThreadArgARC1*)arg;

    MCCConfig cfg;
    thrarg->usercfg->ApplyToConfig(cfg);
    AREXClient ac(thrarg->url, cfg, thrarg->usercfg->Timeout());
    std::list< std::pair<URL, ServiceType> > services;
    if (!ac.listServicesFromISIS(services)) {
      delete thrarg;
      return;
    }
    logger.msg(VERBOSE,
               "Found %u execution services from the index service at %s",
               services.size(), thrarg->url.str());

    for (std::list<std::pair<URL, ServiceType> >::iterator it =
           services.begin(); it != services.end(); it++) {
      TargetRetrieverARC1 r(*(thrarg->usercfg), it->first.str(), it->second);
      if (thrarg->isExecutionTarget)
        r.GetExecutionTargets(*(thrarg->mom));
      else
        r.GetJobs(*(thrarg->mom));
    }

    delete thrarg;
  }

  void TargetRetrieverARC1::InterrogateTarget(void *arg) {
    ThreadArgARC1 *thrarg = (ThreadArgARC1*)arg;

    if (thrarg->isExecutionTarget) {
      logger.msg(DEBUG, "Collecting ExecutionTarget (A-REX/BES) information.");
      MCCConfig cfg;
      thrarg->usercfg->ApplyToConfig(cfg);
      AREXClient ac(thrarg->url, cfg, thrarg->usercfg->Timeout(), thrarg->flavour == "ARC1");
      XMLNode servicesQueryResponse;
      if (!ac.sstat(servicesQueryResponse)) {
        delete thrarg;
        return;
      }

      if(thrarg->flavour != "ARC1") {
        // Doing plain BES request first
        for(XMLNode ext = servicesQueryResponse["FactoryResourceAttributesDocument"]["BESExtension"];(bool)ext;++ext) {
          if((std::string)ext == "http://www.nordugrid.org/schemas/a-rex") {
            // Because we are doing all that only for finding proper plugin 
            // code quietly exits. Otherwise it could switch to ARC1.
            delete thrarg;
            return;
          };
        };
        ExecutionTarget target;
        target.GridFlavour = thrarg->flavour;
        target.Cluster = thrarg->url;
        target.url = thrarg->url;
        target.InterfaceName = "BES";
        target.Implementor = "unknown";
        target.DomainName = thrarg->url.Host();
        target.HealthState = "ok";
        logger.msg(VERBOSE, "Generating BES target: %s", target.Cluster.str());
        thrarg->mom->AddTarget(target);
        delete thrarg;
        return;
      }
      std::list<ExecutionTarget> targets;
      ExtractTargets(thrarg->url, servicesQueryResponse, targets);
      for (std::list<ExecutionTarget>::const_iterator it = targets.begin(); it != targets.end(); it++) {
        thrarg->mom->AddTarget(*it);
      }
      delete thrarg;
      return;
    }
    else {
      logger.msg(DEBUG, "Collecting Job (A-REX jobs) information.");

      /*
       * If errors are encountered here there cannot be returned in usual manner
       * since this function should run in a separate thread. There might be a
       * need for reporting errors, maybe a bit flag of some kind.
       */
      DataHandle dir_url(thrarg->url, *(thrarg->usercfg));
      if (!dir_url) {
        logger.msg(INFO, "Failed retrieving job IDs: Unsupported url (%s) given", thrarg->url.str());
        delete thrarg;
        return;
      }

      dir_url->SetSecure(false);
      std::list<FileInfo> files;
      if (!dir_url->List(files, DataPoint::INFO_TYPE_NAME)) {
        if (files.empty()) {
          logger.msg(INFO, "Failed retrieving job IDs");
          delete thrarg;
          return;
        }
        logger.msg(VERBOSE, "Error encoutered during job ID retrieval. All job IDs might not have been retrieved");
      }

      for (std::list<FileInfo>::iterator file = files.begin();
           file != files.end(); file++) {
        Job j;
        j.JobID = thrarg->url;
        j.JobID.ChangePath(j.JobID.Path() + "/" + file->GetName());
        j.Flavour = "ARC1";
        j.Cluster = thrarg->url;
        thrarg->mom->AddJob(j);
      }
      delete thrarg;
      return;
    }
  }

  void TargetRetrieverARC1::ExtractTargets(const URL& url, XMLNode response, std::list<ExecutionTarget>& targets) {
    targets.clear();

    /*
     * A-REX will not return multiple ComputingService elements, but if the
     * response comes from an index server then there might be multiple.
     */
    for (XMLNode GLUEService = response["ComputingService"];
         GLUEService; ++GLUEService) {
      targets.push_back(ExecutionTarget());

      targets.back().GridFlavour = "ARC1";
      targets.back().Cluster = url;
      targets.back().url = url;
      targets.back().InterfaceName = "BES";
      targets.back().Implementor = "NorduGrid";

      targets.back().DomainName = url.Host();

      logger.msg(VERBOSE, "Generating A-REX target: %s", targets.back().Cluster.str());

      XMLNode ComputingEndpoint = GLUEService["ComputingEndpoint"];
      if (ComputingEndpoint["HealthState"]) {
        targets.back().HealthState = (std::string)ComputingEndpoint["HealthState"];
      } else {
        logger.msg(VERBOSE, "The Service advertises no Health State.");
      }
      if (ComputingEndpoint["HealthStateInfo"]) {
        targets.back().HealthState = (std::string)ComputingEndpoint["HealthStateInfo"];
      }
      if (GLUEService["Name"]) {
        targets.back().ServiceName = (std::string)GLUEService["Name"];
      }
      if (ComputingEndpoint["Capability"]) {
        for (XMLNode n = ComputingEndpoint["Capability"]; n; ++n) {
          targets.back().Capability.push_back((std::string)n);
        }
      } else if (GLUEService["Capability"]) {
        for (XMLNode n = GLUEService["Capability"]; n; ++n) {
          targets.back().Capability.push_back((std::string)n);
        }
      }
      if (GLUEService["Type"]) {
        targets.back().ServiceType = (std::string)GLUEService["Type"];
      } else {
        logger.msg(VERBOSE, "The Service doesn't advertise its Type.");
      }
      if (ComputingEndpoint["QualityLevel"]) {
        targets.back().QualityLevel = (std::string)ComputingEndpoint["QualityLevel"];
      } else if (GLUEService["QualityLevel"]) {
        targets.back().QualityLevel = (std::string)GLUEService["QualityLevel"];
      } else {
        logger.msg(VERBOSE, "The Service doesn't advertise its Quality Level.");
      }

      if (ComputingEndpoint["Technology"]) {
        targets.back().Technology = (std::string)ComputingEndpoint["Technology"];
      }
      if (ComputingEndpoint["InterfaceName"]) {
        targets.back().InterfaceName = (std::string)ComputingEndpoint["InterfaceName"];
      } else if (ComputingEndpoint["Interface"]) {
        targets.back().InterfaceName = (std::string)ComputingEndpoint["Interface"];
      } else {
        logger.msg(VERBOSE, "The Service doesn't advertise its Interface.");
      }
      if (ComputingEndpoint["InterfaceVersion"]) {
        targets.back().InterfaceName = (std::string)ComputingEndpoint["InterfaceVersion"];
      }
      if (ComputingEndpoint["InterfaceExtension"]) {
        for (XMLNode n = ComputingEndpoint["InterfaceExtension"]; n; ++n) {
          targets.back().InterfaceExtension.push_back((std::string)n);
        }
      }
      if (ComputingEndpoint["SupportedProfile"]) {
        for (XMLNode n = ComputingEndpoint["SupportedProfile"]; n; ++n) {
          targets.back().SupportedProfile.push_back((std::string)n);
        }
      }
      if (ComputingEndpoint["Implementor"]) {
        targets.back().Implementor = (std::string)ComputingEndpoint["Implementor"];
      }
      if (ComputingEndpoint["ImplementationName"]) {
        if (ComputingEndpoint["ImplementationVersion"]) {
          targets.back().Implementation =
            Software((std::string)ComputingEndpoint["ImplementationName"],
                     (std::string)ComputingEndpoint["ImplementationVersion"]);
        } else {
          targets.back().Implementation = Software((std::string)ComputingEndpoint["ImplementationName"]);
        }
      }
      if (ComputingEndpoint["ServingState"]) {
        targets.back().ServingState = (std::string)ComputingEndpoint["ServingState"];
      } else {
        logger.msg(VERBOSE, "The Service doesn't advertise its Serving State.");
      }
      if (ComputingEndpoint["IssuerCA"]) {
        targets.back().IssuerCA = (std::string)ComputingEndpoint["IssuerCA"];
      }
      if (ComputingEndpoint["TrustedCA"]) { 
        XMLNode n = ComputingEndpoint["TrustedCA"];
        while (n) {
          targets.back().TrustedCA.push_back((std::string)n);
          ++n; //The increment operator works in an unusual manner (returns void)
        }
      }
      if (ComputingEndpoint["DowntimeStart"]) {
        targets.back().DowntimeStarts = (std::string)ComputingEndpoint["DowntimeStart"];
      }
      if (ComputingEndpoint["DowntimeEnd"]) {
        targets.back().DowntimeEnds = (std::string)ComputingEndpoint["DowntimeEnd"];
      }
      if (ComputingEndpoint["Staging"]) {
        targets.back().Staging = (std::string)ComputingEndpoint["Staging"];
      }
      if (ComputingEndpoint["JobDescription"]) {
        for (XMLNode n = ComputingEndpoint["JobDescription"]; n; ++n) {
          targets.back().JobDescriptions.push_back((std::string)n);
        }
      }

      //Attributes below should possibly consider elements in different places (Service/Endpoint/Share etc).
      if (ComputingEndpoint["TotalJobs"]) {
        targets.back().TotalJobs = stringtoi((std::string)ComputingEndpoint["TotalJobs"]);
      } else if (GLUEService["TotalJobs"]) {
        targets.back().TotalJobs = stringtoi((std::string)GLUEService["TotalJobs"]);
      }
      if (ComputingEndpoint["RunningJobs"]) {
        targets.back().RunningJobs = stringtoi((std::string)ComputingEndpoint["RunningJobs"]);
      } else if (GLUEService["RunningJobs"]) {
        targets.back().RunningJobs = stringtoi((std::string)GLUEService["RunningJobs"]);
      }
      if (ComputingEndpoint["WaitingJobs"]) {
        targets.back().WaitingJobs = stringtoi((std::string)ComputingEndpoint["WaitingJobs"]);
      } else if (GLUEService["WaitingJobs"]) {
        targets.back().WaitingJobs = stringtoi((std::string)GLUEService["WaitingJobs"]);
      }
      if (ComputingEndpoint["StagingJobs"]) {
        targets.back().StagingJobs = stringtoi((std::string)ComputingEndpoint["StagingJobs"]);
      } else if (GLUEService["StagingJobs"]) {
        targets.back().StagingJobs = stringtoi((std::string)GLUEService["StagingJobs"]);
      }
      if (ComputingEndpoint["SuspendedJobs"]) {
        targets.back().SuspendedJobs = stringtoi((std::string)ComputingEndpoint["SuspendedJobs"]);
      } else if (GLUEService["SuspendedJobs"]) {
        targets.back().SuspendedJobs = stringtoi((std::string)GLUEService["SuspendedJobs"]);
      }
      if (ComputingEndpoint["PreLRMSWaitingJobs"]) {
        targets.back().PreLRMSWaitingJobs = stringtoi((std::string)ComputingEndpoint["PreLRMSWaitingJobs"]);
      } else if (GLUEService["PreLRMSWaitingJobs"]) {
        targets.back().PreLRMSWaitingJobs = stringtoi((std::string)GLUEService["PreLRMSWaitingJobs"]);
      }
      if (ComputingEndpoint["LocalRunningJobs"]) {
        targets.back().LocalRunningJobs = stringtoi((std::string)ComputingEndpoint["LocalRunningJobs"]);
      } else if (GLUEService["LocalRunningJobs"]) {
        targets.back().LocalRunningJobs = stringtoi((std::string)GLUEService["LocalRunningJobs"]);
      }
      if (ComputingEndpoint["LocalWaitingJobs"]) {
        targets.back().LocalWaitingJobs = stringtoi((std::string)ComputingEndpoint["LocalWaitingJobs"]);
      } else if (GLUEService["LocalWaitingJobs"]) {
        targets.back().LocalWaitingJobs = stringtoi((std::string)GLUEService["LocalWaitingJobs"]);
      }
      if (ComputingEndpoint["LocalSuspendedJobs"]) {
        targets.back().LocalSuspendedJobs = stringtoi((std::string)ComputingEndpoint["LocalSuspendedJobs"]);
      } else if (GLUEService["LocalSuspendedJobs"]) {
        targets.back().LocalWaitingJobs = stringtoi((std::string)GLUEService["LocalSuspendedJobs"]);
      }

      XMLNode ComputingManager = GLUEService["ComputingManager"];
      if (ComputingManager["ProductName"]) {
        targets.back().ManagerProductName = (std::string)ComputingManager["ProductName"];
      } else if (ComputingManager["Type"]) { // is this non-standard fallback needed?
        targets.back().ManagerProductName = (std::string)ComputingManager["Type"];
      }
      if (ComputingManager["ProductVersion"]) {
        targets.back().ManagerProductName = (std::string)ComputingManager["ProductVersion"];
      }
      if (ComputingManager["Reservation"]) {
        targets.back().Reservation = ((std::string)ComputingManager["Reservation"] == "true") ? true : false;
      }
      if (ComputingManager["BulkSubmission"]) {
        targets.back().BulkSubmission = ((std::string)ComputingManager["BulkSubmission"] == "true") ? true : false;
      }
      if (ComputingManager["TotalPhysicalCPUs"]) {
        targets.back().TotalPhysicalCPUs = stringtoi((std::string)ComputingManager["TotalPhysicalCPUs"]);
      }
      if (ComputingManager["TotalLogicalCPUs"]) {
        targets.back().TotalLogicalCPUs = stringtoi((std::string)ComputingManager["TotalLogicalCPUs"]);
      }
      if (ComputingManager["TotalSlots"]) {
        targets.back().TotalSlots = stringtoi((std::string)ComputingManager["TotalSlots"]);
      }
      if (ComputingManager["Homogeneous"]) {
        targets.back().Homogeneous = ((std::string)ComputingManager["Homogeneous"] == "true") ? true : false;
      }
      if (ComputingManager["NetworkInfo"]) {
        for (XMLNode n = ComputingManager["NetworkInfo"]; n; ++n) {
          targets.back().NetworkInfo.push_back((std::string)n);
        }
      }
      if (ComputingManager["WorkingAreaShared"]) {
        targets.back().WorkingAreaShared = ((std::string)ComputingManager["WorkingAreaShared"] == "true") ? true : false;
      }
      if (ComputingManager["WorkingAreaFree"]) {
        targets.back().WorkingAreaFree = stringtoi((std::string)ComputingManager["WorkingAreaFree"]);
      }
      if (ComputingManager["WorkingAreaTotal"]) {
        targets.back().WorkingAreaTotal = stringtoi((std::string)ComputingManager["WorkingAreaTotal"]);
      }
      if (ComputingManager["WorkingAreaLifeTime"]) {
        targets.back().WorkingAreaLifeTime = (std::string)ComputingManager["WorkingAreaLifeTime"];
      }
      if (ComputingManager["CacheFree"]) {
        targets.back().CacheFree = stringtoi((std::string)ComputingManager["CacheFree"]);
      }
      if (ComputingManager["CacheTotal"]) {
        targets.back().CacheTotal = stringtoi((std::string)ComputingManager["CacheTotal"]);
      }
      for (XMLNode n = ComputingManager["Benchmark"]; n; ++n) {
        double value;
        if (n["Type"] && n["Value"] &&
            stringto((std::string)n["Value"], value)) {
          targets.back().Benchmarks[(std::string)n["Type"]] = value;
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
          ae.FreeSlots = targets.back().FreeSlots;
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
        targets.back().ApplicationEnvironments.push_back(ae);
      }

      XMLNode ComputingShare = GLUEService["ComputingShare"];
      for (int i = 0; ComputingShare[i]; i++) {
        ExecutionTarget& currentTarget = targets.back();
        if (ComputingShare[i+1]) {
          targets.push_back(ExecutionTarget(targets.back()));
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
            if (fswdPair.size() > 2 || !stringto(fswdPair.front(), freeSlots) || fswdPair.size() == 2 && !stringto(fswdPair.back(), duration)) {
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
    }
  }

} // namespace Arc
