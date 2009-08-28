// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <algorithm>
#include <list>

#include <arc/client/Broker.h>
#include <arc/StringConv.h>

namespace Arc {

  Logger Broker::logger(Logger::getRootLogger(), "broker");

  Broker::Broker(Config *cfg)
    : ACC(cfg),
      TargetSortingDone(false),
      job(NULL) {}

  Broker::~Broker() {}

  void Broker::PreFilterTargets(std::list<ExecutionTarget>& targets,
                                const JobDescription& jobdesc) {
    job = &jobdesc;

    for (std::list<ExecutionTarget>::iterator target = targets.begin();
         target != targets.end(); target++) {
      logger.msg(DEBUG, "Matchmaking, ExecutionTarget URL:  %s ",
                 target->url.str());

      if (!job->Resources.CandidateTarget.empty()) {
        if (target->url.Host().empty())
          logger.msg(DEBUG, "Matchmaking, ExecutionTarget: URL is not properly defined");
        if (target->MappingQueue.empty())
          logger.msg(DEBUG, "Matchmaking, ExecutionTarget:  MappingQueue is not defined");

        bool dropTarget = true;
        
        if (!target->url.Host().empty() || !target->MappingQueue.empty()) {
          for (std::list<ResourceTargetType>::const_iterator it = job->Resources.CandidateTarget.begin();
               it != job->Resources.CandidateTarget.end(); it++) {

            if (!it->EndPointURL.Host().empty() &&
                target->url.Host().empty()) { // Drop target since URL is not defined.
              logger.msg(DEBUG, "Matchmaking problem, URL of ExecutionTarget is not properly defined: %s.", target->url.str());
              break;
            }

            if (!it->QueueName.empty() &&
                target->MappingQueue.empty()) { // Drop target since MappingQueue is not advertised.
              logger.msg(DEBUG, "Matchmaking problem, MappingQueue of ExecutionTarget is not advertised, and a queue (%s) have been requested.", it->QueueName);
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
            logger.msg(DEBUG, "Matchmaking problem, ExecutionTarget does not satisfy any of the CandidateTargets.");
            continue;
          }
        }
        else {
          logger.msg(DEBUG, "Neither URL or MappingQueue is reported by the cluster");
          continue;
        }
      }

      if ((int)job->Application.ProcessingStartTime.GetTime() != -1) {
        if ((int)(*target).DowntimeStarts.GetTime() != -1 && (int)(*target).DowntimeEnds.GetTime() != -1) {
          if ((int)(*target).DowntimeStarts.GetTime() > (int)job->Application.ProcessingStartTime.GetTime() ||
              (int)(*target).DowntimeEnds.GetTime() < (int)job->Application.ProcessingStartTime.GetTime()) {  // 3423434
            logger.msg(DEBUG, "Matchmaking, ProcessingStartTime problem, JobDescription: %s (ProcessingStartTime) / ExecutionTarget: %s (DowntimeStarts) - %s (DowntimeEnds)", (std::string)job->Application.ProcessingStartTime, (std::string)(*target).DowntimeStarts, (std::string)(*target).DowntimeEnds);
            continue;
          }
        }
        else
          logger.msg(DEBUG, "Matchmaking, ExecutionTarget: %s, Downtime is not defined", (std::string)(*target).url.str());
      }

      /* if ((*target).FreeSlots == -1) {
               logger.msg(DEBUG, "Matchmaking, FreeSlots problem, ExecutionTarget: %s, FreeSlots == -1", (std::string)(*target).url.str());
                continue;
         }*/

      if (!(*target).HealthState.empty()) {

        // Enumeration for healthstate: ok, critical, other, unknown, warning

        if ((*target).HealthState != "ok") {
          logger.msg(DEBUG, "Matchmaking, HealthState problem, ExecutionTarget: %s (HealthState) != ok %s", (std::string)(*target).url.str(), (*target).HealthState);
          continue;
        }
      }
      else {
        logger.msg(DEBUG, "Matchmaking, ExecutionTarget:  %s, HealthState is not defined", (std::string)(*target).url.str());
        continue;
      }

      if (!job->Resources.CEType.empty()) {
        if (!(*target).Implementation().empty()) {
          if (job->Resources.CEType.isSatisfied((*target).Implementation)) {
            logger.msg(DEBUG, "Matchmaking, Computing endpoint requirement not satisfied. ExecutionTarget: %s", (std::string)target->Implementation);
            continue;
          }
        }
        else {
          logger.msg(DEBUG, "Matchmaking, ExecutionTarget:  %s, ImplementationName is not defined", (std::string)(*target).url.str());
          continue;
        }
      }

      if ((int)job->Resources.TotalWallTime.range != -1) {
        if ((int)(*target).MaxWallTime.GetPeriod() != -1) {     // Example: 123
          if (!((int)(*target).MaxWallTime.GetPeriod()) >= (int)job->Resources.TotalWallTime.range) {
            logger.msg(DEBUG, "Matchmaking, TotalWallTime problem, ExecutionTarget: %s (MaxWallTime) JobDescription: %d (TotalWallTime)", (std::string)(*target).MaxWallTime, job->Resources.TotalWallTime.range.max);
            continue;
          }
        }
        else {
          //logger.msg(DEBUG, "Matchmaking, ExecutionTarget: %s, MaxWallTime is not defined", (std::string)(*target).url.str());
          //continue;
        }

        if ((int)(*target).MinWallTime.GetPeriod() != -1) {     // Example: 123
          if (!((int)(*target).MinWallTime.GetPeriod()) > (int)job->Resources.TotalWallTime.range) {
            logger.msg(DEBUG, "Matchmaking, MinWallTime problem, ExecutionTarget: %s (MinWallTime) JobDescription: %d (TotalWallTime)", (std::string)(*target).MinWallTime, job->Resources.TotalWallTime.range.max);
            continue;
          }
        }
        else {
          //logger.msg(DEBUG, "Matchmaking, ExecutionTarget: %s, MinWallTime is not defined", (std::string)(*target).url.str());
          //continue;
        }
      }

      if ((int)job->Resources.TotalCPUTime.range != -1) {
        if ((int)(*target).MaxCPUTime.GetPeriod() != -1) {     // Example: 456
          if (!((int)(*target).MaxCPUTime.GetPeriod()) >= (int)job->Resources.TotalCPUTime.range) {
            logger.msg(DEBUG, "Matchmaking, TotalCPUTime problem, ExecutionTarget: %s (MaxCPUTime) JobDescription: %d (TotalCPUTime)", (std::string)(*target).MaxCPUTime, job->Resources.TotalCPUTime.range.max);
            continue;
          }
        }
        else {
          //logger.msg(DEBUG, "Matchmaking, ExecutionTarget: %s, MaxCPUTime is not defined", (std::string)(*target).url.str());
          //continue;
        }

        if ((int)(*target).MinCPUTime.GetPeriod() != -1) {     // Example: 456
          if (!((int)(*target).MinCPUTime.GetPeriod()) > (int)job->Resources.TotalCPUTime.range) {
            logger.msg(DEBUG, "Matchmaking, MinCPUTime problem, ExecutionTarget: %s (MinCPUTime) JobDescription: %s (TotalCPUTime)", (std::string)(*target).MinCPUTime, job->Resources.TotalCPUTime.range.max);
            continue;
          }
        }
        else {
          //logger.msg(DEBUG, "Matchmaking, ExecutionTarget: %s, MinCPUTime is not defined", (std::string)(*target).url.str());
          //continue;
        }
      }

      if (job->Resources.IndividualPhysicalMemory != -1) {
        if ((*target).MainMemorySize != -1) {     // Example: 678
          if (!((*target).MainMemorySize >= job->Resources.IndividualPhysicalMemory)) {
            logger.msg(DEBUG, "Matchmaking, MainMemorySize problem, ExecutionTarget: %d (MainMemorySize), JobDescription: %d (IndividualPhysicalMemory)", (*target).MainMemorySize, job->Resources.IndividualPhysicalMemory.max);
            continue;
          }
        }
        else if ((*target).MaxMainMemory != -1) {     // Example: 678
          if (!((*target).MaxMainMemory >= job->Resources.IndividualPhysicalMemory)) {
            logger.msg(DEBUG, "Matchmaking, MaxMainMemory problem, ExecutionTarget: %d (MaxMainMemory), JobDescription: %d (IndividualPhysicalMemory)", (*target).MaxMainMemory, job->Resources.IndividualPhysicalMemory.max);
            continue;
          }

        }
        else {
          logger.msg(DEBUG, "Matchmaking, ExecutionTarget: %s, MaxMainMemory and MainMemorySize are not defined", (std::string)(*target).url.str());
          continue;
        }
      }

      if (job->Resources.IndividualVirtualMemory != -1) {
        if ((*target).MaxVirtualMemory != -1) {     // Example: 678
          if (!((*target).MaxVirtualMemory >= job->Resources.IndividualVirtualMemory)) {
            logger.msg(DEBUG, "Matchmaking, MaxVirtualMemory problem, ExecutionTarget: %d (MaxVirtualMemory), JobDescription: %d (IndividualVirtualMemory)", (*target).MaxVirtualMemory, job->Resources.IndividualVirtualMemory.max);
            continue;
          }
        }
        else {
          logger.msg(DEBUG, "Matchmaking, ExecutionTarget: %s, MaxVirtualMemory is not defined", (std::string)(*target).url.str());
          continue;
        }
      }

      if (!job->Resources.Platform.empty()) {
        if (!(*target).Platform.empty()) {    // Example: i386
          if ((*target).Platform != job->Resources.Platform) {
            logger.msg(DEBUG, "Matchmaking, Platform problem, ExecutionTarget: %s (Platform) JobDescription: %s (Platform)", (*target).Platform, job->Resources.Platform);
            continue;
          }
        }
        else {
          logger.msg(DEBUG, "Matchmaking, ExecutionTarget:  %s, Platform is not defined", (std::string)(*target).url.str());
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
        if (!target->OperatingSystem.empty()) {
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
        if (!(*target).NetworkInfo.empty()) {    // Example: infiniband
          if (std::find(target->NetworkInfo.begin(), target->NetworkInfo.end(),
                        job->Resources.NetworkInfo) == target->NetworkInfo.end())
            // logger.msg(DEBUG, "Matchmaking, NetworkInfo problem, ExecutionTarget: %s (NetworkInfo) JobDescription: %s (NetworkInfo)", (*target).NetworkInfo, job->NetworkInfo);
            continue;
          else {
            logger.msg(DEBUG, "Matchmaking, ExecutionTarget:  %s, NetworkInfo is not defined", (std::string)(*target).url.str());
            continue;
          }
        }

      if (job->Resources.DiskSpaceRequirement.SessionDiskSpace != -1) {
        if ((*target).MaxDiskSpace != -1) {     // Example: 5656
          if (!((*target).MaxDiskSpace >= job->Resources.DiskSpaceRequirement.SessionDiskSpace)) {
            logger.msg(DEBUG, "Matchmaking, MaxDiskSpace problem, ExecutionTarget: %s (MaxDiskSpace) JobDescription: %s (SessionDiskSpace)", (*target).MaxDiskSpace, job->Resources.DiskSpaceRequirement.SessionDiskSpace);
            continue;
          }
        }
        else if ((*target).WorkingAreaTotal != -1) {     // Example: 5656
          if (!((*target).WorkingAreaTotal >= job->Resources.DiskSpaceRequirement.SessionDiskSpace)) {
            logger.msg(DEBUG, "Matchmaking, WorkingAreaTotal problem, ExecutionTarget: %s (WorkingAreaTotal) JobDescription: %s (SessionDiskSpace)", (*target).WorkingAreaTotal, job->Resources.DiskSpaceRequirement.SessionDiskSpace);
            continue;
          }
        }
        else {
          logger.msg(DEBUG, "Matchmaking, ExecutionTarget:  %s, MaxDiskSpace and WorkingAreaTotal are not defined", (std::string)(*target).url.str());
          continue;
        }
      }

      if (job->Resources.DiskSpaceRequirement.DiskSpace != -1 && job->Resources.DiskSpaceRequirement.CacheDiskSpace != -1) {
        if ((*target).MaxDiskSpace != -1) {     // Example: 5656
          if (!((*target).MaxDiskSpace >= (job->Resources.DiskSpaceRequirement.DiskSpace - job->Resources.DiskSpaceRequirement.CacheDiskSpace))) {
            logger.msg(DEBUG, "Matchmaking, MaxDiskSpace >= DiskSpace - CacheDiskSpace problem, ExecutionTarget: %d (MaxDiskSpace) JobDescription: %d (DiskSpace) - %d (CacheDiskSpace)", (*target).MaxDiskSpace, job->Resources.DiskSpaceRequirement.DiskSpace.max, job->Resources.DiskSpaceRequirement.CacheDiskSpace);
            continue;
          }
        }
        else if ((*target).WorkingAreaTotal != -1) {     // Example: 5656
          if (!((*target).WorkingAreaTotal >= (job->Resources.DiskSpaceRequirement.DiskSpace - job->Resources.DiskSpaceRequirement.CacheDiskSpace))) {
            logger.msg(DEBUG, "Matchmaking, WorkingAreaTotal >= DiskSpace - CacheDiskSpace problem, ExecutionTarget: %d (MaxDiskSpace) JobDescription: %d (DiskSpace) - %d (CacheDiskSpace)", (*target).WorkingAreaTotal, job->Resources.DiskSpaceRequirement.DiskSpace.max, job->Resources.DiskSpaceRequirement.CacheDiskSpace);
            continue;
          }
        }
        else {
          logger.msg(DEBUG, "Matchmaking, ExecutionTarget:  %s, MaxDiskSpace and WorkingAreaTotal are not defined", (std::string)(*target).url.str());
          continue;
        }
      }

      if (job->Resources.DiskSpaceRequirement.DiskSpace != -1) {
        if ((*target).MaxDiskSpace != -1) {     // Example: 5656
          if (!((*target).MaxDiskSpace >= job->Resources.DiskSpaceRequirement.DiskSpace)) {
            logger.msg(DEBUG, "Matchmaking, MaxDiskSpace problem, ExecutionTarget: %d (MaxDiskSpace) JobDescription: %d (DiskSpace)", (*target).MaxDiskSpace, job->Resources.DiskSpaceRequirement.DiskSpace.max);
            continue;
          }
        }
        else if ((*target).WorkingAreaTotal != -1) {     // Example: 5656
          if (!((*target).WorkingAreaTotal >= job->Resources.DiskSpaceRequirement.DiskSpace)) {
            logger.msg(DEBUG, "Matchmaking, WorkingAreaTotal problem, ExecutionTarget: %d (WorkingAreaTotal) JobDescription: %d (DiskSpace)", (*target).WorkingAreaTotal, job->Resources.DiskSpaceRequirement.DiskSpace.max);
            continue;
          }
        }
        else {
          logger.msg(DEBUG, "Matchmaking, ExecutionTarget:  %s, MaxDiskSpace and WorkingAreaTotal are not defined", (std::string)(*target).url.str());
          continue;
        }
      }

      if (job->Resources.DiskSpaceRequirement.CacheDiskSpace != -1) {
        if ((*target).CacheTotal != -1) {     // Example: 5656
          if (!((*target).CacheTotal >= job->Resources.DiskSpaceRequirement.CacheDiskSpace)) {
            logger.msg(DEBUG, "Matchmaking, CacheTotal problem, ExecutionTarget: %d (CacheTotal) JobDescription: %d (CacheDiskSpace)", (*target).CacheTotal, job->Resources.DiskSpaceRequirement.CacheDiskSpace);
            continue;
          }
        }
        else {
          logger.msg(DEBUG, "Matchmaking, ExecutionTarget:  %s, CacheTotal is not defined", (std::string)(*target).url.str());
          continue;
        }
      }

      if (job->Resources.SlotRequirement.NumberOfProcesses != -1) {
        if ((*target).TotalSlots != -1) {     // Example: 5656
          if (!((*target).TotalSlots >= job->Resources.SlotRequirement.NumberOfProcesses)) {
            logger.msg(DEBUG, "Matchmaking, TotalSlots problem, ExecutionTarget: %d (TotalSlots) JobDescription: %d (NumberOfProcesses)", (*target).TotalSlots, job->Resources.SlotRequirement.NumberOfProcesses.max);
            continue;
          }
        }
        else if ((*target).MaxSlotsPerJob != -1) {     // Example: 5656
          if (!((*target).MaxSlotsPerJob >= job->Resources.SlotRequirement.NumberOfProcesses)) {
            logger.msg(DEBUG, "Matchmaking, MaxSlotsPerJob problem, ExecutionTarget: %d (MaxSlotsPerJob) JobDescription: %d (NumberOfProcesses)", (*target).TotalSlots, job->Resources.SlotRequirement.NumberOfProcesses.max);
            continue;
          }
        }
        else {
          logger.msg(DEBUG, "Matchmaking, ExecutionTarget:  %s, TotalSlots and MaxSlotsPerJob are not defined", (std::string)(*target).url.str());
          continue;
        }
      }

      if ((int)job->Resources.SessionLifeTime.GetPeriod() != -1) {
        if ((int)(*target).WorkingAreaLifeTime.GetPeriod() != -1) {     // Example: 123
          if (!((int)(*target).WorkingAreaLifeTime.GetPeriod()) >= (int)job->Resources.SessionLifeTime.GetPeriod()) {
            logger.msg(DEBUG, "Matchmaking, WorkingAreaLifeTime problem, ExecutionTarget: %s (WorkingAreaLifeTime) JobDescription: %s (SessionLifeTime)", (std::string)(*target).WorkingAreaLifeTime, (std::string)job->Resources.SessionLifeTime);
            continue;
          }
        }
        else {
          logger.msg(DEBUG, "Matchmaking, ExecutionTarget:  %s, WorkingAreaLifeTime is not defined", (std::string)(*target).url.str());
          continue;
        }
      }

      if ((job->Resources.NodeAccess == NAT_INBOUND ||
           job->Resources.NodeAccess == NAT_INOUTBOUND) &&
          !(*target).ConnectivityIn) {     // Example: false (boolean)
        logger.msg(DEBUG, "Matchmaking, ConnectivityIn problem, ExecutionTarget: %s (ConnectivityIn) JobDescription: %s (InBound)", (job->Resources.NodeAccess == NAT_INBOUND ? "INBOUND" : "INOUTBOUND"), ((*target).ConnectivityIn ? "true" : "false"));
        continue;
      }

      if ((job->Resources.NodeAccess == NAT_OUTBOUND ||
           job->Resources.NodeAccess == NAT_INOUTBOUND) &&
          !(*target).ConnectivityOut) {     // Example: false (boolean)
        logger.msg(DEBUG, "Matchmaking, ConnectivityOut problem, ExecutionTarget: %s (ConnectivityOut) JobDescription: %s (OutBound)", (job->Resources.NodeAccess == NAT_OUTBOUND ? "OUTBOUND" : "INOUTBOUND"), ((*target).ConnectivityIn ? "true" : "false"));
        continue;
      }

      PossibleTargets.push_back(&(*target));

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
    if (PossibleTargets.size() <= 0 || current == PossibleTargets.end()) {
      return NULL;
    }

    if (!TargetSortingDone) {
      logger.msg(VERBOSE, "Target sorting not done, sorting them now");
      SortTargets();
      current = PossibleTargets.begin();
    }
    else current++;

    return (current != PossibleTargets.end() ? *current : NULL);
  }

  void Broker::RegisterJobsubmission() {
    if (!job || current == PossibleTargets.end()) return;
    if ((*current)->FreeSlots >= job->Resources.SlotRequirement.NumberOfProcesses) {   //The job will start directly
      (*current)->FreeSlots -= job->Resources.SlotRequirement.NumberOfProcesses;
      if ((*current)->UsedSlots != -1)
        (*current)->UsedSlots += job->Resources.SlotRequirement.NumberOfProcesses;
    }
    else                                           //The job will be queued
      if ((*current)->WaitingJobs != -1)
        (*current)->WaitingJobs += job->Resources.SlotRequirement.NumberOfProcesses;
  }
} // namespace Arc
