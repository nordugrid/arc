// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <algorithm>

#include <arc/StringConv.h>
#include <arc/client/Broker.h>
#include <arc/client/ExecutionTarget.h>
#include <arc/loader/FinderLoader.h>

namespace Arc {

  Logger Broker::logger(Logger::getRootLogger(), "Broker");

  Broker::Broker(const UserConfig& usercfg)
    : usercfg(usercfg),
      TargetSortingDone(false),
      job() {}

  Broker::~Broker() {}

  void Broker::PreFilterTargets(std::list<ExecutionTarget>& targets,
                                const JobDescription& jobdesc) {
    job = &jobdesc;

    for (std::list<ExecutionTarget>::iterator target = targets.begin();
         target != targets.end(); target++) {
      logger.msg(DEBUG, "Performing matchmaking against target (%s).", target->url.str());

      if (!job->Resources.CandidateTarget.empty()) {
        if (target->url.Host().empty())
          logger.msg(DEBUG, "URL of ExecutionTarget is not properly defined");
        if (target->MappingQueue.empty())
          logger.msg(DEBUG, "MappingQueue of ExecutionTarget (%s) is not defined", target->url.str());

        bool dropTarget = true;

        if (!target->url.Host().empty() || !target->MappingQueue.empty()) {
          for (std::list<ResourceTargetType>::const_iterator it = job->Resources.CandidateTarget.begin();
               it != job->Resources.CandidateTarget.end(); it++) {

            if (!it->EndPointURL.Host().empty() &&
                target->url.Host().empty()) { // Drop target since URL is not defined.
              logger.msg(DEBUG, "URL of ExecutionTarget is not properly defined: %s.", target->url.str());
              break;
            }

            if (!it->QueueName.empty() &&
                target->MappingQueue.empty()) { // Drop target since MappingQueue is not published.
              logger.msg(DEBUG, "MappingQueue of ExecutionTarget is not published, and a queue (%s) have been requested.", it->QueueName);
              break;
            }

            if (!it->EndPointURL.Host().empty() &&
                target->url.Host() == it->EndPointURL.Host()) { // Example: knowarc1.grid.niif.hu
              dropTarget = false;
              break;
            }

            if (!it->QueueName.empty() &&
                target->MappingQueue == it->QueueName) {
              dropTarget = false;
              break;
            }
          }

          if (dropTarget) {
            logger.msg(DEBUG, "ExecutionTarget does not satisfy any of the CandidateTargets.");
            continue;
          }
        }
        else {
          logger.msg(DEBUG, "Neither URL or MappingQueue is reported by the cluster");
          continue;
        }
      }

      if ((int)job->Application.ProcessingStartTime.GetTime() != -1) {
        if ((int)target->DowntimeStarts.GetTime() != -1 && (int)target->DowntimeEnds.GetTime() != -1) {
          if (target->DowntimeStarts <= job->Application.ProcessingStartTime && job->Application.ProcessingStartTime <= target->DowntimeEnds) {
            logger.msg(DEBUG, "ProcessingStartTime (%s) specified in job description is inside the targets downtime period [ %s - %s ].", (std::string)job->Application.ProcessingStartTime, (std::string)target->DowntimeStarts, (std::string)target->DowntimeEnds);
            continue;
          }
        }
        else
          logger.msg(WARNING, "The downtime of the target (%s) is not published. Keeping target.", target->url.str());
      }

      if (target->FreeSlots == 0) {
        logger.msg(DEBUG, "Dropping ExecutionTarget %s: No free slots", target->url.str());
        continue;
      }

      if (!target->HealthState.empty()) {

        if (target->HealthState != "ok") { // Enumeration for healthstate: ok, critical, other, unknown, warning
          logger.msg(DEBUG, "HealthState of ExecutionTarget (%s) is not OK (%s)", target->url.str(), target->HealthState);
          continue;
        }
      }
      else {
        logger.msg(DEBUG, "Matchmaking, ExecutionTarget:  %s, HealthState is not defined", target->url.str());
        continue;
      }

      if (!job->Resources.CEType.empty()) {
        if (!target->Implementation().empty()) {
          if (!job->Resources.CEType.isSatisfied(target->Implementation)) {
            logger.msg(DEBUG, "Matchmaking, Computing endpoint requirement not satisfied. ExecutionTarget: %s", (std::string)target->Implementation);
            continue;
          }
        }
        else {
          logger.msg(DEBUG, "Matchmaking, ExecutionTarget:  %s, ImplementationName is not defined", target->url.str());
          continue;
        }
      }

      {
        typedef std::pair<std::string, int> EtTimePair;
        EtTimePair etTime[] = {EtTimePair("MaxWallTime", (int)target->MaxWallTime.GetPeriod()),
                               EtTimePair("MinWallTime", (int)target->MinWallTime.GetPeriod()),
                               EtTimePair("MaxCPUTime", (int)target->MaxCPUTime.GetPeriod()),
                               EtTimePair("MinCPUTime", (int)target->MinCPUTime.GetPeriod())};

        typedef std::pair<std::string, const ScalableTime<int>*> JobTimePair;
        JobTimePair jobTime[] = {JobTimePair("TotalWallTime", &job->Resources.TotalWallTime),
                                 JobTimePair("TotalCPUTime", &job->Resources.TotalCPUTime)};

        // Check if ARC-clockrate is defined, if not add it. Included to support the XRSL attribute gridtime.
        if (job->Resources.TotalCPUTime.benchmark.first == "ARC-clockrate") {
          target->Benchmarks["ARC-clockrate"] = (target->CPUClockSpeed > 0 ? (double)target->CPUClockSpeed : 1000.);
        }

        int i = 0;
        for (; i < 4; i++) {
          JobTimePair *jTime = &jobTime[i/2];
          if (i%2 == 0 && jTime->second->range.max != -1 ||
              i%2 == 1 && jTime->second->range.min != -1) {
            if (etTime[i].second != -1) {
              if (jTime->second->benchmark.first.empty()) { // No benchmark defined, do not scale.
                if (i%2 == 0 && jTime->second->range.max > etTime[i].second ||
                    i%2 == 1 && jTime->second->range.min < etTime[i].second) {
                  logger.msg(DEBUG,
                             "Matchmaking, %s (%d) is %s than %s (%d) published by the ExecutionTarget.",
                             jTime->first,
                             (i%2 == 0 ? jTime->second->range.max
                                       : jTime->second->range.min),
                             (i%2 == 0 ? "greater" : "less"),
                             etTime[i].first,
                             etTime[i].second);
                  break;
                }
              }
              else { // Benchmark defined => scale using benchmark.
                if (target->Benchmarks.find(jTime->second->benchmark.first) != target->Benchmarks.end()) {
                  if (i%2 == 0 && jTime->second->scaleMax(target->Benchmarks.find(jTime->second->benchmark.first)->second) > etTime[i].second ||
                      i%2 == 1 && jTime->second->scaleMin(target->Benchmarks.find(jTime->second->benchmark.first)->second) < etTime[i].second) {
                    logger.msg(DEBUG,
                               "Matchmaking, The %s scaled %s (%d) is %s than the %s (%d) published by the ExecutionTarget.",
                               jTime->second->benchmark.first,
                               jTime->first,
                               (i%2 == 0 ? jTime->second->scaleMax(target->Benchmarks.find(jTime->second->benchmark.first)->second)
                                         : jTime->second->scaleMin(target->Benchmarks.find(jTime->second->benchmark.first)->second)),
                               (i%2 == 0 ? "greater" : "less"),
                               etTime[i].first,
                               etTime[i].second);
                    break;
                  }
                }
                else {
                  logger.msg(DEBUG, "Matchmaking, Benchmark %s is not published by the ExecutionTarget.", jTime->second->benchmark.first);
                  break;
                }
              }
            }
            // Do not drop target if it does not publish attribute.
          }
        }

        if (i != 4) // Above loop exited too early, which means target should be dropped.
          continue;
      }

      if (job->Resources.IndividualPhysicalMemory != -1) {
        if (target->MainMemorySize != -1) {     // Example: 678
          if (target->MainMemorySize < job->Resources.IndividualPhysicalMemory) {
            logger.msg(DEBUG, "Matchmaking, MainMemorySize problem, ExecutionTarget: %d (MainMemorySize), JobDescription: %d (IndividualPhysicalMemory)", target->MainMemorySize, job->Resources.IndividualPhysicalMemory.max);
            continue;
          }
        }
        else if (target->MaxMainMemory != -1) {     // Example: 678
          if (target->MaxMainMemory < job->Resources.IndividualPhysicalMemory) {
            logger.msg(DEBUG, "Matchmaking, MaxMainMemory problem, ExecutionTarget: %d (MaxMainMemory), JobDescription: %d (IndividualPhysicalMemory)", target->MaxMainMemory, job->Resources.IndividualPhysicalMemory.max);
            continue;
          }

        }
        else {
          logger.msg(DEBUG, "Matchmaking, ExecutionTarget: %s, MaxMainMemory and MainMemorySize are not defined", target->url.str());
          continue;
        }
      }

      if (job->Resources.IndividualVirtualMemory != -1) {
        if (target->MaxVirtualMemory != -1) {     // Example: 678
          if (target->MaxVirtualMemory < job->Resources.IndividualVirtualMemory) {
            logger.msg(DEBUG, "Matchmaking, MaxVirtualMemory problem, ExecutionTarget: %d (MaxVirtualMemory), JobDescription: %d (IndividualVirtualMemory)", target->MaxVirtualMemory, job->Resources.IndividualVirtualMemory.max);
            continue;
          }
        }
        else {
          logger.msg(DEBUG, "Matchmaking, ExecutionTarget: %s, MaxVirtualMemory is not defined", target->url.str());
          continue;
        }
      }

      if (!job->Resources.Platform.empty()) {
        if (!target->Platform.empty()) {    // Example: i386
          if (target->Platform != job->Resources.Platform) {
            logger.msg(DEBUG, "Matchmaking, Platform problem, ExecutionTarget: %s (Platform) JobDescription: %s (Platform)", target->Platform, job->Resources.Platform);
            continue;
          }
        }
        else {
          logger.msg(DEBUG, "Matchmaking, ExecutionTarget:  %s, Platform is not defined", target->url.str());
          continue;
        }
      }

      if (!job->Resources.OperatingSystem.empty()) {
        if (!target->OperatingSystem.empty()) {
          if (!job->Resources.OperatingSystem.isSatisfied(target->OperatingSystem)) {
            logger.msg(DEBUG, "Matchmaking, ExecutionTarget: %s, OperatingSystem requirements not satisfied", target->url.str());
            continue;
          }
        }
        else {
          logger.msg(DEBUG, "Matchmaking, ExecutionTarget:  %s,  OperatingSystem is not defined", target->url.str());
          continue;
        }
      }

      if (!job->Resources.RunTimeEnvironment.empty()) {
        if (!target->ApplicationEnvironments.empty()) {
          if (!job->Resources.RunTimeEnvironment.isSatisfied(target->ApplicationEnvironments)) {
            logger.msg(DEBUG, "Matchmaking, ExecutionTarget:  %s, RunTimeEnvironment requirements not satisfied", target->url.str());
            continue;
          }
        }
        else {
          logger.msg(DEBUG, "Matchmaking, ExecutionTarget: %s, ApplicationEnvironments not defined", target->url.str());
          continue;
        }
      }

      if (!job->Resources.NetworkInfo.empty())
        if (!target->NetworkInfo.empty()) {    // Example: infiniband
          if (std::find(target->NetworkInfo.begin(), target->NetworkInfo.end(),
                        job->Resources.NetworkInfo) == target->NetworkInfo.end()) {
            logger.msg(DEBUG, "Matchmaking, NetworkInfo demand not fulfilled, ExecutionTarget do not support %s, specified in the JobDescription.", job->Resources.NetworkInfo);
            continue;
          }
          else {
            logger.msg(DEBUG, "Matchmaking, ExecutionTarget:  %s, NetworkInfo is not defined", target->url.str());
            continue;
          }
        }

      if (job->Resources.DiskSpaceRequirement.SessionDiskSpace != -1) {
        if (target->MaxDiskSpace != -1) {     // Example: 5656
          if (target->MaxDiskSpace < job->Resources.DiskSpaceRequirement.SessionDiskSpace) {
            logger.msg(DEBUG, "Matchmaking, MaxDiskSpace problem, ExecutionTarget: %d (MaxDiskSpace) JobDescription: %d (SessionDiskSpace)", target->MaxDiskSpace, job->Resources.DiskSpaceRequirement.SessionDiskSpace);
            continue;
          }
        }
        else if (target->WorkingAreaTotal != -1) {     // Example: 5656
          if (target->WorkingAreaTotal < job->Resources.DiskSpaceRequirement.SessionDiskSpace) {
            logger.msg(DEBUG, "Matchmaking, WorkingAreaTotal problem, ExecutionTarget: %d (WorkingAreaTotal) JobDescription: %d (SessionDiskSpace)", target->WorkingAreaTotal, job->Resources.DiskSpaceRequirement.SessionDiskSpace);
            continue;
          }
        }
        else {
          logger.msg(DEBUG, "Matchmaking, ExecutionTarget:  %s, MaxDiskSpace and WorkingAreaTotal are not defined", target->url.str());
          continue;
        }
      }

      if (job->Resources.DiskSpaceRequirement.DiskSpace != -1 && job->Resources.DiskSpaceRequirement.CacheDiskSpace != -1) {
        if (target->MaxDiskSpace != -1) {     // Example: 5656
          if (target->MaxDiskSpace < job->Resources.DiskSpaceRequirement.DiskSpace - job->Resources.DiskSpaceRequirement.CacheDiskSpace) {
            logger.msg(DEBUG, "Matchmaking, MaxDiskSpace >= DiskSpace - CacheDiskSpace problem, ExecutionTarget: %d (MaxDiskSpace) JobDescription: %d (DiskSpace) - %d (CacheDiskSpace)", target->MaxDiskSpace, job->Resources.DiskSpaceRequirement.DiskSpace.max, job->Resources.DiskSpaceRequirement.CacheDiskSpace);
            continue;
          }
        }
        else if (target->WorkingAreaTotal != -1) {     // Example: 5656
          if (target->WorkingAreaTotal < job->Resources.DiskSpaceRequirement.DiskSpace - job->Resources.DiskSpaceRequirement.CacheDiskSpace) {
            logger.msg(DEBUG, "Matchmaking, WorkingAreaTotal >= DiskSpace - CacheDiskSpace problem, ExecutionTarget: %d (MaxDiskSpace) JobDescription: %d (DiskSpace) - %d (CacheDiskSpace)", target->WorkingAreaTotal, job->Resources.DiskSpaceRequirement.DiskSpace.max, job->Resources.DiskSpaceRequirement.CacheDiskSpace);
            continue;
          }
        }
        else {
          logger.msg(DEBUG, "Matchmaking, ExecutionTarget:  %s, MaxDiskSpace and WorkingAreaTotal are not defined", target->url.str());
          continue;
        }
      }

      if (job->Resources.DiskSpaceRequirement.DiskSpace != -1) {
        if (target->MaxDiskSpace != -1) {     // Example: 5656
          if (target->MaxDiskSpace < job->Resources.DiskSpaceRequirement.DiskSpace) {
            logger.msg(DEBUG, "Matchmaking, MaxDiskSpace problem, ExecutionTarget: %d (MaxDiskSpace) JobDescription: %d (DiskSpace)", target->MaxDiskSpace, job->Resources.DiskSpaceRequirement.DiskSpace.max);
            continue;
          }
        }
        else if (target->WorkingAreaTotal != -1) {     // Example: 5656
          if (target->WorkingAreaTotal < job->Resources.DiskSpaceRequirement.DiskSpace) {
            logger.msg(DEBUG, "Matchmaking, WorkingAreaTotal problem, ExecutionTarget: %d (WorkingAreaTotal) JobDescription: %d (DiskSpace)", target->WorkingAreaTotal, job->Resources.DiskSpaceRequirement.DiskSpace.max);
            continue;
          }
        }
        else {
          logger.msg(DEBUG, "Matchmaking, ExecutionTarget:  %s, MaxDiskSpace and WorkingAreaTotal are not defined", target->url.str());
          continue;
        }
      }

      if (job->Resources.DiskSpaceRequirement.CacheDiskSpace != -1) {
        if (target->CacheTotal != -1) {     // Example: 5656
          if (target->CacheTotal < job->Resources.DiskSpaceRequirement.CacheDiskSpace) {
            logger.msg(DEBUG, "Matchmaking, CacheTotal problem, ExecutionTarget: %d (CacheTotal) JobDescription: %d (CacheDiskSpace)", target->CacheTotal, job->Resources.DiskSpaceRequirement.CacheDiskSpace);
            continue;
          }
        }
        else {
          logger.msg(DEBUG, "Matchmaking, ExecutionTarget:  %s, CacheTotal is not defined", target->url.str());
          continue;
        }
      }

      if (job->Resources.SlotRequirement.NumberOfSlots != -1) {
        if (target->TotalSlots != -1) {     // Example: 5656
          if (target->TotalSlots < job->Resources.SlotRequirement.NumberOfSlots) {
            logger.msg(DEBUG, "Matchmaking, TotalSlots problem, ExecutionTarget: %d (TotalSlots) JobDescription: %d (NumberOfProcesses)", target->TotalSlots, job->Resources.SlotRequirement.NumberOfSlots.max);
            continue;
          }
        }
        if (target->MaxSlotsPerJob != -1) {     // Example: 5656
          if (target->MaxSlotsPerJob < job->Resources.SlotRequirement.NumberOfSlots) {
            logger.msg(DEBUG, "Matchmaking, MaxSlotsPerJob problem, ExecutionTarget: %d (MaxSlotsPerJob) JobDescription: %d (NumberOfProcesses)", target->TotalSlots, job->Resources.SlotRequirement.NumberOfSlots.max);
            continue;
          }
        }

        if (target->TotalSlots == -1 && target->MaxSlotsPerJob == -1) {
          logger.msg(DEBUG, "Matchmaking, ExecutionTarget:  %s, TotalSlots and MaxSlotsPerJob are not defined", target->url.str());
          continue;
        }
      }

      if ((int)job->Resources.SessionLifeTime.GetPeriod() != -1) {
        if ((int)target->WorkingAreaLifeTime.GetPeriod() != -1) {     // Example: 123
          if (target->WorkingAreaLifeTime < job->Resources.SessionLifeTime) {
            logger.msg(DEBUG, "Matchmaking, WorkingAreaLifeTime problem, ExecutionTarget: %s (WorkingAreaLifeTime) JobDescription: %s (SessionLifeTime)", (std::string)target->WorkingAreaLifeTime, (std::string)job->Resources.SessionLifeTime);
            continue;
          }
        }
        else {
          logger.msg(DEBUG, "Matchmaking, ExecutionTarget:  %s, WorkingAreaLifeTime is not defined", target->url.str());
          continue;
        }
      }

      if ((job->Resources.NodeAccess == NAT_INBOUND ||
           job->Resources.NodeAccess == NAT_INOUTBOUND) &&
          !target->ConnectivityIn) {     // Example: false (boolean)
        logger.msg(DEBUG, "Matchmaking, ConnectivityIn problem, ExecutionTarget: %s (ConnectivityIn) JobDescription: %s (InBound)", (job->Resources.NodeAccess == NAT_INBOUND ? "INBOUND" : "INOUTBOUND"), (target->ConnectivityIn ? "true" : "false"));
        continue;
      }

      if ((job->Resources.NodeAccess == NAT_OUTBOUND ||
           job->Resources.NodeAccess == NAT_INOUTBOUND) &&
          !target->ConnectivityOut) {     // Example: false (boolean)
        logger.msg(DEBUG, "Matchmaking, ConnectivityOut problem, ExecutionTarget: %s (ConnectivityOut) JobDescription: %s (OutBound)", (job->Resources.NodeAccess == NAT_OUTBOUND ? "OUTBOUND" : "INOUTBOUND"), (target->ConnectivityIn ? "true" : "false"));
        continue;
      }

      PossibleTargets.push_back(&*target);

    } //end loop over all found targets

    logger.msg(DEBUG, "Possible targets after prefiltering: %d", PossibleTargets.size());

    std::list<ExecutionTarget*>::iterator iter = PossibleTargets.begin();

    for (int i = 1; iter != PossibleTargets.end(); iter++, i++) {
      logger.msg(DEBUG, "%d. Cluster: %s", i, (*iter)->DomainName);
      logger.msg(DEBUG, "Health State: %s", (*iter)->HealthState);
    }

    TargetSortingDone = false;
  }

  const ExecutionTarget* Broker::GetBestTarget() {
    if (PossibleTargets.size() <= 0 || current == PossibleTargets.end())
      return NULL;

    if (!TargetSortingDone) {
      logger.msg(VERBOSE, "Target sorting not done, sorting them now");
      SortTargets();
      current = PossibleTargets.begin();
    }
    else
      current++;

    return (current != PossibleTargets.end() ? *current : NULL);
  }

  void Broker::RegisterJobsubmission() {
    if (!job || current == PossibleTargets.end())
      return;
    if ((*current)->FreeSlots >= job->Resources.SlotRequirement.NumberOfSlots) {   //The job will start directly
      (*current)->FreeSlots -= job->Resources.SlotRequirement.NumberOfSlots;
      if ((*current)->UsedSlots != -1)
        (*current)->UsedSlots += job->Resources.SlotRequirement.NumberOfSlots;
    }
    else                                           //The job will be queued
      if ((*current)->WaitingJobs != -1)
        (*current)->WaitingJobs += job->Resources.SlotRequirement.NumberOfSlots;
  }

  BrokerLoader::BrokerLoader()
    : Loader(BaseConfig().MakeConfig(Config()).Parent()) {}

  BrokerLoader::~BrokerLoader() {
    for (std::list<Broker*>::iterator it = brokers.begin();
         it != brokers.end(); it++)
      delete *it;
  }

  Broker* BrokerLoader::load(const std::string& name,
                             const UserConfig& usercfg) {
    if (name.empty())
      return NULL;

    PluginList list = FinderLoader::GetPluginList("HED:Broker");
    factory_->load(list[name], "HED:Broker");

    BrokerPluginArgument arg(usercfg);
    Broker *broker = factory_->GetInstance<Broker>("HED:Broker", name, &arg);

    if (!broker) {
      logger.msg(ERROR, "Broker %s could not be created", name);
      return NULL;
    }

    brokers.push_back(broker);
    logger.msg(INFO, "Loaded Broker %s", name);
    return broker;
  }

} // namespace Arc
