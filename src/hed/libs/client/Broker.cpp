// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <algorithm>
#include <list>

#include <arc/client/Broker.h>
#include <arc/job/runtimeenvironment.h>
#include <arc/StringConv.h>

namespace Arc {

  Arc::Logger Broker::logger(Arc::Logger::getRootLogger(), "broker");

  Broker::Broker(Config *cfg)
    : ACC(cfg),
      PreFilteringDone(false),
      TargetSortingDone(false) {}

  Broker::~Broker() {}

  void Broker::PreFilterTargets(const TargetGenerator& targen,
                                const JobDescription& job) {
    for (std::list<ExecutionTarget>::const_iterator target =    \
           targen.FoundTargets().begin();
         target != targen.FoundTargets().end(); target++) {
      logger.msg(DEBUG, "Matchmaking, ExecutionTarget URL:  %s ",
                 target->url.str());

      // TODO: if the type is a number than we need to check the measure

      if (!((std::string)(job.EndPointURL.Host())).empty()) {
        if (!((std::string)((*target).url.Host())).empty()) {
          if (((std::string)(*target).url.Host()) != job.EndPointURL.Host()) {   // Example: knowarc1.grid.niif.hu
            logger.msg(DEBUG, "Matchmaking, URL problem, ExecutionTarget:  %s (url) != JobDescription: %s (EndPointURL)", (std::string)(*target).url.Host(), job.EndPointURL.Host());
            continue;
          }
        }
        else {
          logger.msg(DEBUG, "Matchmaking, ExecutionTarget:  %s, URL is not defined", (std::string)(*target).url.str());
          continue;
        }

      }

      if ((int)job.ProcessingStartTime.GetTime() != -1) {
        if ((int)(*target).DowntimeStarts.GetTime() != -1 && (int)(*target).DowntimeEnds.GetTime() != -1) {
          if ((int)(*target).DowntimeStarts.GetTime() > (int)job.ProcessingStartTime.GetTime() ||
              (int)(*target).DowntimeEnds.GetTime() < (int)job.ProcessingStartTime.GetTime()) {  // 3423434
            logger.msg(DEBUG, "Matchmaking, ProcessingStartTime problem, JobDescription: %s (ProcessingStartTime) / ExecutionTarget: %s (DowntimeStarts) - %s (DowntimeEnds)", (std::string)job.ProcessingStartTime, (std::string)(*target).DowntimeStarts, (std::string)(*target).DowntimeEnds);
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

      if (!job.CEType.empty()) {
        if (!(*target).ImplementationName.empty()) {
          if ((*target).ImplementationName != job.CEType) {   // Example: A-REX, ARC0
            logger.msg(DEBUG, "Matchmaking, CEType problem, ExecutionTarget: %s (ImplementationName) != JobDescription: %s (CEType)", (*target).ImplementationName, job.CEType);
            continue;
          }
        }
        else {
          logger.msg(DEBUG, "Matchmaking, ExecutionTarget:  %s, ImplementationName is not defined", (std::string)(*target).url.str());
          continue;
        }
      }

      if (!job.QueueName.empty()) {
        if (!(*target).MappingQueue.empty()) {
          if ((*target).MappingQueue != job.QueueName) {   // Example: gridlong
            logger.msg(DEBUG, "Matchmaking, queue problem, ExecutionTarget: %s (MappingQueue) != JobDescription: %s (QueueName)", (*target).MappingQueue, job.QueueName);
            continue;
          }
        }
        else
          logger.msg(DEBUG, "Matchmaking, ExecutionTarget:  %s, MappingQueue is not defined", (std::string)(*target).url.str());
        // TODO: CREAM, MappingQueue not available in schema
        //
        // continue;
      }

      if ((int)job.TotalWallTime.GetPeriod() != -1) {
        if ((int)(*target).MaxWallTime.GetPeriod() != -1) {     // Example: 123
          if (!((int)(*target).MaxWallTime.GetPeriod()) >= (int)job.TotalWallTime.GetPeriod()) {
            logger.msg(DEBUG, "Matchmaking, TotalWallTime problem, ExecutionTarget: %s (MaxWallTime) JobDescription: %s (TotalWallTime)", (std::string)(*target).MaxWallTime, (std::string)job.TotalWallTime);
            continue;
          }
        }
        else {
          //logger.msg(DEBUG, "Matchmaking, ExecutionTarget: %s, MaxWallTime is not defined", (std::string)(*target).url.str());
          //continue;
        }

        if ((int)(*target).MinWallTime.GetPeriod() != -1) {     // Example: 123
          if (!((int)(*target).MinWallTime.GetPeriod()) > (int)job.TotalWallTime.GetPeriod()) {
            logger.msg(DEBUG, "Matchmaking, MinWallTime problem, ExecutionTarget: %s (MinWallTime) JobDescription: %s (TotalWallTime)", (std::string)(*target).MinWallTime, (std::string)job.TotalWallTime);
            continue;
          }
        }
        else {
          //logger.msg(DEBUG, "Matchmaking, ExecutionTarget: %s, MinWallTime is not defined", (std::string)(*target).url.str());
          //continue;
        }
      }

      if ((int)job.TotalCPUTime.GetPeriod() != -1) {
        if ((int)(*target).MaxCPUTime.GetPeriod() != -1) {     // Example: 456
          if (!((int)(*target).MaxCPUTime.GetPeriod()) >= (int)job.TotalCPUTime.GetPeriod()) {
            logger.msg(DEBUG, "Matchmaking, TotalCPUTime problem, ExecutionTarget: %s (MaxCPUTime) JobDescription: %s (TotalCPUTime)", (std::string)(*target).MaxCPUTime, (std::string)job.TotalCPUTime);
            continue;
          }
        }
        else {
          //logger.msg(DEBUG, "Matchmaking, ExecutionTarget: %s, MaxCPUTime is not defined", (std::string)(*target).url.str());
          //continue;
        }

        if ((int)(*target).MinCPUTime.GetPeriod() != -1) {     // Example: 456
          if (!((int)(*target).MinCPUTime.GetPeriod()) > (int)job.TotalCPUTime.GetPeriod()) {
            logger.msg(DEBUG, "Matchmaking, MinCPUTime problem, ExecutionTarget: %s (MinCPUTime) JobDescription: %s (TotalCPUTime)", (std::string)(*target).MinCPUTime, (std::string)job.TotalCPUTime);
            continue;
          }
        }
        else {
          //logger.msg(DEBUG, "Matchmaking, ExecutionTarget: %s, MinCPUTime is not defined", (std::string)(*target).url.str());
          //continue;
        }
      }

      if (job.IndividualPhysicalMemory != -1) {
        if ((*target).MainMemorySize != -1) {     // Example: 678
          if (!((*target).MainMemorySize >= job.IndividualPhysicalMemory)) {
            logger.msg(DEBUG, "Matchmaking, MainMemorySize problem, ExecutionTarget: %d (MainMemorySize), JobDescription: %d (IndividualPhysicalMemory)", (*target).MainMemorySize, job.IndividualPhysicalMemory);
            continue;
          }
        }
        else if ((*target).MaxMainMemory != -1) {     // Example: 678
          if (!((*target).MaxMainMemory >= job.IndividualPhysicalMemory)) {
            logger.msg(DEBUG, "Matchmaking, MaxMainMemory problem, ExecutionTarget: %d (MaxMainMemory), JobDescription: %d (IndividualPhysicalMemory)", (*target).MaxMainMemory, job.IndividualPhysicalMemory);
            continue;
          }

        }
        else {
          logger.msg(DEBUG, "Matchmaking, ExecutionTarget: %s, MaxMainMemory and MainMemorySize are not defined", (std::string)(*target).url.str());
          continue;
        }
      }

      if (job.IndividualVirtualMemory != -1) {
        if ((*target).MaxVirtualMemory != -1) {     // Example: 678
          if (!((*target).MaxVirtualMemory >= job.IndividualVirtualMemory)) {
            logger.msg(DEBUG, "Matchmaking, MaxVirtualMemory problem, ExecutionTarget: %d (MaxVirtualMemory), JobDescription: %d (IndividualVirtualMemory)", (*target).MaxVirtualMemory, job.IndividualVirtualMemory);
            continue;
          }
        }
        else {
          logger.msg(DEBUG, "Matchmaking, ExecutionTarget: %s, MaxVirtualMemory is not defined", (std::string)(*target).url.str());
          continue;
        }
      }

      if (!job.Platform.empty()) {
        if (!(*target).Platform.empty()) {    // Example: i386
          if ((*target).Platform != job.Platform) {
            logger.msg(DEBUG, "Matchmaking, Platform problem, ExecutionTarget: %s (Platform) JobDescription: %s (Platform)", (*target).Platform, job.Platform);
            continue;
          }
        }
        else {
          logger.msg(DEBUG, "Matchmaking, ExecutionTarget:  %s, Platform is not defined", (std::string)(*target).url.str());
          continue;
        }
      }

      if (!job.OSFamily.empty()) {
        if (!(*target).OSFamily.empty()) {    // Example: linux
          if ((*target).OSFamily != job.OSFamily) {
            logger.msg(DEBUG, "Matchmaking, OSFamily problem, ExecutionTarget: %s (OSFamily) JobDescription: %s (OSFamily)", (*target).OSFamily, job.OSFamily);
            continue;
          }
        }
        else {
          logger.msg(DEBUG, "Matchmaking, ExecutionTarget:  %s, OSFamily is not defined", (std::string)(*target).url.str());
          continue;
        }
      }

      if (!job.OSName.empty()) {
        if (!(*target).OSName.empty()) {    // Example: ubuntu
          if ((*target).OSName != job.OSName) {
            logger.msg(DEBUG, "Matchmaking, OSName problem, ExecutionTarget: %s (OSName) JobDescription: %s (OSName)", (*target).OSName, job.OSName);
            continue;
          }
        }
        else {
          logger.msg(DEBUG, "Matchmaking, ExecutionTarget:  %s, OSName is not defined", (std::string)(*target).url.str());
          continue;
        }
      }

      if (!job.OSVersion.empty()) {
        if (!(*target).OSVersion.empty()) {    // Example: 4.3.1
          std::string a = "OSVersion-" + (*target).OSVersion;
          std::string b = "OSVersion-" + job.OSVersion;
          RuntimeEnvironment RE0(a);
          RuntimeEnvironment RE1(b);
          if (RE0 < RE1) {
            logger.msg(DEBUG, "Matchmaking, OSVersion problem, ExecutionTarget: %s (OSVersion) JobDescription: %s (OSVersion)", (*target).OSVersion, job.OSVersion);
            continue;
          }
        }
        else {
          logger.msg(DEBUG, "Matchmaking, ExecutionTarget:  %s, OSVersion is not defined", (std::string)(*target).url.str());
          continue;
        }
      }

      if (!job.RunTimeEnvironment.empty()) {
        if (!(*target).ApplicationEnvironments.empty()) {   // Example: ATLAS-9.0.3
          std::list<Arc::RunTimeEnvironmentType>::const_iterator iter1;
          bool next_target;
          bool match;
          for (iter1 = job.RunTimeEnvironment.begin(); iter1 != job.RunTimeEnvironment.end(); iter1++) {
            next_target = false;
            std::list<std::string>::const_iterator iter2;
            match = false;
            std::list<Arc::ApplicationEnvironment>::const_iterator iter3;
            if ((*iter1).Version.size() == 0)
              for (iter3 = (*target).ApplicationEnvironments.begin(); iter3 != (*target).ApplicationEnvironments.end(); iter3++) {
                RuntimeEnvironment rt((*iter3).Name);
                if ((*iter1).Name == rt.Name()) {
                  match = true;
                  break;
                }
              }
            for (iter2 = (*iter1).Version.begin(); iter2 != (*iter1).Version.end(); iter2++)
              for (iter3 = (*target).ApplicationEnvironments.begin(); iter3 != (*target).ApplicationEnvironments.end(); iter3++) {
                RuntimeEnvironment rt((*iter3).Name);
                if ((*iter1).Name == rt.Name() && (*iter2) == rt.Version()) {
                  match = true;
                  break;
                }
              }
            // end of job's version list parsing
            if (!match) {
              next_target = true;
              logger.msg(DEBUG, "Matchmaking, ExecutionTarget:  %s, this RunTimeEnvironment is not installed: %s", (std::string)(*target).url.str(), (*iter1).Name);
              break;
            }
          }
          if (next_target)
            continue;
        }
        else {
          logger.msg(DEBUG, "Matchmaking, ExecutionTarget:  %s, RunTimeEnvironment is not defined", (std::string)(*target).url.str());
          continue;
        }

      }

      if (!job.NetworkInfo.empty())
        if (!(*target).NetworkInfo.empty()) {    // Example: infiniband
          if (std::find(target->NetworkInfo.begin(), target->NetworkInfo.end(),
                        job.NetworkInfo) == target->NetworkInfo.end())
            // logger.msg(DEBUG, "Matchmaking, NetworkInfo problem, ExecutionTarget: %s (NetworkInfo) JobDescription: %s (NetworkInfo)", (*target).NetworkInfo, job.NetworkInfo);
            continue;
          else {
            logger.msg(DEBUG, "Matchmaking, ExecutionTarget:  %s, NetworkInfo is not defined", (std::string)(*target).url.str());
            continue;
          }
        }

      if (job.SessionDiskSpace != -1) {
        if ((*target).MaxDiskSpace != -1) {     // Example: 5656
          if (!((*target).MaxDiskSpace >= job.SessionDiskSpace)) {
            logger.msg(DEBUG, "Matchmaking, MaxDiskSpace problem, ExecutionTarget: %s (MaxDiskSpace) JobDescription: %s (SessionDiskSpace)", (*target).MaxDiskSpace, job.SessionDiskSpace);
            continue;
          }
        }
        else if ((*target).WorkingAreaTotal != -1) {     // Example: 5656
          if (!((*target).WorkingAreaTotal >= job.SessionDiskSpace)) {
            logger.msg(DEBUG, "Matchmaking, WorkingAreaTotal problem, ExecutionTarget: %s (WorkingAreaTotal) JobDescription: %s (SessionDiskSpace)", (*target).WorkingAreaTotal, job.SessionDiskSpace);
            continue;
          }
        }
        else {
          logger.msg(DEBUG, "Matchmaking, ExecutionTarget:  %s, MaxDiskSpace and WorkingAreaTotal are not defined", (std::string)(*target).url.str());
          continue;
        }
      }

      if (job.DiskSpace != -1 && job.CacheDiskSpace != -1) {
        if ((*target).MaxDiskSpace != -1) {     // Example: 5656
          if (!((*target).MaxDiskSpace >= (job.DiskSpace - job.CacheDiskSpace))) {
            logger.msg(DEBUG, "Matchmaking, MaxDiskSpace >= DiskSpace - CacheDiskSpace problem, ExecutionTarget: %d (MaxDiskSpace) JobDescription: %d (DiskSpace) - %d (CacheDiskSpace)", (*target).MaxDiskSpace, job.DiskSpace, job.CacheDiskSpace);
            continue;
          }
        }
        else if ((*target).WorkingAreaTotal != -1) {     // Example: 5656
          if (!((*target).WorkingAreaTotal >= (job.DiskSpace - job.CacheDiskSpace))) {
            logger.msg(DEBUG, "Matchmaking, WorkingAreaTotal >= DiskSpace - CacheDiskSpace problem, ExecutionTarget: %d (MaxDiskSpace) JobDescription: %d (DiskSpace) - %d (CacheDiskSpace)", (*target).WorkingAreaTotal, job.DiskSpace, job.CacheDiskSpace);
            continue;
          }
        }
        else {
          logger.msg(DEBUG, "Matchmaking, ExecutionTarget:  %s, MaxDiskSpace and WorkingAreaTotal are not defined", (std::string)(*target).url.str());
          continue;
        }
      }

      if (job.DiskSpace != -1) {
        if ((*target).MaxDiskSpace != -1) {     // Example: 5656
          if (!((*target).MaxDiskSpace >= job.DiskSpace)) {
            logger.msg(DEBUG, "Matchmaking, MaxDiskSpace problem, ExecutionTarget: %d (MaxDiskSpace) JobDescription: %d (DiskSpace)", (*target).MaxDiskSpace, job.DiskSpace);
            continue;
          }
        }
        else if ((*target).WorkingAreaTotal != -1) {     // Example: 5656
          if (!((*target).WorkingAreaTotal >= job.DiskSpace)) {
            logger.msg(DEBUG, "Matchmaking, WorkingAreaTotal problem, ExecutionTarget: %d (WorkingAreaTotal) JobDescription: %d (DiskSpace)", (*target).WorkingAreaTotal, job.DiskSpace);
            continue;
          }
        }
        else {
          logger.msg(DEBUG, "Matchmaking, ExecutionTarget:  %s, MaxDiskSpace and WorkingAreaTotal are not defined", (std::string)(*target).url.str());
          continue;
        }
      }

      if (job.CacheDiskSpace != -1) {
        if ((*target).CacheTotal != -1) {     // Example: 5656
          if (!((*target).CacheTotal >= job.CacheDiskSpace)) {
            logger.msg(DEBUG, "Matchmaking, CacheTotal problem, ExecutionTarget: %d (CacheTotal) JobDescription: %d (CacheDiskSpace)", (*target).CacheTotal, job.CacheDiskSpace);
            continue;
          }
        }
        else {
          logger.msg(DEBUG, "Matchmaking, ExecutionTarget:  %s, CacheTotal is not defined", (std::string)(*target).url.str());
          continue;
        }
      }

      if (job.Slots != -1) {
        if ((*target).TotalSlots != -1) {     // Example: 5656
          if (!((*target).TotalSlots >= job.Slots)) {
            logger.msg(DEBUG, "Matchmaking, TotalSlots problem, ExecutionTarget: %d (TotalSlots) JobDescription: %d (Slots)", (*target).TotalSlots, job.Slots);
            continue;
          }
        }
        else if ((*target).MaxSlotsPerJob != -1) {     // Example: 5656
          if (!((*target).MaxSlotsPerJob >= job.Slots)) {
            logger.msg(DEBUG, "Matchmaking, MaxSlotsPerJob problem, ExecutionTarget: %d (MaxSlotsPerJob) JobDescription: %d (Slots)", (*target).MaxSlotsPerJob, job.Slots);
            continue;
          }
        }
        else {
          logger.msg(DEBUG, "Matchmaking, ExecutionTarget:  %s, TotalSlots and MaxSlotsPerJob are not defined", (std::string)(*target).url.str());
          continue;
        }
      }

      if (job.NumberOfProcesses != -1) {
        if ((*target).TotalSlots != -1) {     // Example: 5656
          if (!((*target).TotalSlots >= job.NumberOfProcesses)) {
            logger.msg(DEBUG, "Matchmaking, TotalSlots problem, ExecutionTarget: %d (TotalSlots) JobDescription: %d (NumberOfProcesses)", (*target).TotalSlots, job.NumberOfProcesses);
            continue;
          }
        }
        else if ((*target).MaxSlotsPerJob != -1) {     // Example: 5656
          if (!((*target).MaxSlotsPerJob >= job.NumberOfProcesses)) {
            logger.msg(DEBUG, "Matchmaking, MaxSlotsPerJob problem, ExecutionTarget: %d (MaxSlotsPerJob) JobDescription: %d (NumberOfProcesses)", (*target).TotalSlots, job.NumberOfProcesses);
            continue;
          }
        }
        else {
          logger.msg(DEBUG, "Matchmaking, ExecutionTarget:  %s, TotalSlots and MaxSlotsPerJob are not defined", (std::string)(*target).url.str());
          continue;
        }
      }

      if ((int)job.SessionLifeTime.GetPeriod() != -1) {
        if ((int)(*target).WorkingAreaLifeTime.GetPeriod() != -1) {     // Example: 123
          if (!((int)(*target).WorkingAreaLifeTime.GetPeriod()) >= (int)job.SessionLifeTime.GetPeriod()) {
            logger.msg(DEBUG, "Matchmaking, WorkingAreaLifeTime problem, ExecutionTarget: %s (WorkingAreaLifeTime) JobDescription: %s (SessionLifeTime)", (std::string)(*target).WorkingAreaLifeTime, (std::string)job.SessionLifeTime);
            continue;
          }
        }
        else {
          logger.msg(DEBUG, "Matchmaking, ExecutionTarget:  %s, WorkingAreaLifeTime is not defined", (std::string)(*target).url.str());
          continue;
        }
      }

      if (job.InBound)
        if (!(*target).ConnectivityIn) {     // Example: false (boolean)
          logger.msg(DEBUG, "Matchmaking, ConnectivityIn problem, ExecutionTarget: %s (ConnectivityIn) JobDescription: %s (InBound)", (job.InBound ? "true" : "false"), ((*target).ConnectivityIn ? "true" : "false"));
          continue;
        }

      if (job.OutBound)
        if (!(*target).ConnectivityOut) {     // Example: false (boolean)
          logger.msg(DEBUG, "Matchmaking, ConnectivityOut problem, ExecutionTarget: %s (ConnectivityOut) JobDescription: %s (OutBound)", (job.OutBound ? "true" : "false"), ((*target).ConnectivityOut ? "true" : "false"));
          continue;
        }

      if (!job.ReferenceTime.empty()) {

        // TODO: we need a better walltime calculation algorithm

      }

      PossibleTargets.push_back(*target);

    } //end loop over all found targets

    logger.msg(DEBUG, "Possible targets after prefiltering: %d", PossibleTargets.size());

    std::vector<ExecutionTarget>::iterator iter = PossibleTargets.begin();

    for (int i = 1; iter != PossibleTargets.end(); iter++, i++) {
      logger.msg(DEBUG, "%d. Cluster: %s", i, iter->DomainName);
      logger.msg(DEBUG, "Health State: %s", iter->HealthState);
    }

    PreFilteringDone = true;
    TargetSortingDone = false;

  }

  ExecutionTarget& Broker::GetBestTarget(bool& EndOfList) {

    if (PossibleTargets.size() <= 0) {
      EndOfList = true;
      return *PossibleTargets.end();
    }

    if (!TargetSortingDone) {
      logger.msg(VERBOSE, "Target sorting not done, sorting them now");
      SortTargets();
      current = PossibleTargets.begin();
    }

    std::vector<ExecutionTarget>::iterator ret_pointer;
    ret_pointer = current;
    current++;

    if (ret_pointer == PossibleTargets.end())
      EndOfList = true;

    return *ret_pointer;
  }

} // namespace Arc
