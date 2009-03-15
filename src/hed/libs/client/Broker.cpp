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

  void Broker::PreFilterTargets(const Arc::TargetGenerator& targen, Arc::JobDescription jd) {
    jd.getInnerRepresentation(jir);

    for (std::list<Arc::ExecutionTarget>::const_iterator target =    \
           targen.FoundTargets().begin(); target != targen.FoundTargets().end(); \
         target++) {
      logger.msg(DEBUG, "Matchmaking, ExecutionTarget URL:  %s ", (std::string)(*target).url.fullstr());

      // TODO: if the type is a number than we need to check the measure

      if (!((std::string)(jir.EndPointURL.Host())).empty()) {
        if (!((std::string)((*target).url.Host())).empty()) {
          if (((std::string)(*target).url.Host()) != jir.EndPointURL.Host()) {   // Example: knowarc1.grid.niif.hu
            logger.msg(DEBUG, "Matchmaking, URL problem, ExecutionTarget:  %s (url) != JobDescription: %s (EndPointURL)", (std::string)(*target).url.Host(), jir.EndPointURL.Host());
            continue;
          }
        }
        else {
          logger.msg(DEBUG, "Matchmaking, ExecutionTarget:  %s, URL is not defined", (std::string)(*target).url.str());
          continue;
        }

      }

      if ((int)jir.ProcessingStartTime.GetTime() != -1) {
        if ((int)(*target).DowntimeStarts.GetTime() != -1 && (int)(*target).DowntimeEnds.GetTime() != -1) {
          if ((int)(*target).DowntimeStarts.GetTime() > (int)jir.ProcessingStartTime.GetTime() ||
              (int)(*target).DowntimeEnds.GetTime() < (int)jir.ProcessingStartTime.GetTime()) {  // 3423434
            logger.msg(DEBUG, "Matchmaking, ProcessingStartTime problem, JobDescription: %s (ProcessingStartTime) / ExecutionTarget: %s (DowntimeStarts) - %s (DowntimeEnds)", (std::string)jir.ProcessingStartTime, (std::string)(*target).DowntimeStarts, (std::string)(*target).DowntimeEnds);
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

      if (!jir.CEType.empty()) {
        if (!(*target).ImplementationName.empty()) {
          if ((*target).ImplementationName != jir.CEType) {   // Example: A-REX, ARC0
            logger.msg(DEBUG, "Matchmaking, CEType problem, ExecutionTarget: %s (ImplementationName) != JobDescription: %s (CEType)", (*target).ImplementationName, jir.CEType);
            continue;
          }
        }
        else {
          logger.msg(DEBUG, "Matchmaking, ExecutionTarget:  %s, ImplementationName is not defined", (std::string)(*target).url.str());
          continue;
        }
      }

      if (!jir.QueueName.empty()) {
        if (!(*target).MappingQueue.empty()) {
          if ((*target).MappingQueue != jir.QueueName) {   // Example: gridlong
            logger.msg(DEBUG, "Matchmaking, queue problem, ExecutionTarget: %s (MappingQueue) != JobDescription: %s (QueueName)", (*target).MappingQueue, jir.QueueName);
            continue;
          }
        }
        else
          logger.msg(DEBUG, "Matchmaking, ExecutionTarget:  %s, MappingQueue is not defined", (std::string)(*target).url.str());
        // TODO: CREAM, MappingQueue not available in schema
        //
        // continue;
      }

      if ((int)jir.TotalWallTime.GetPeriod() != -1) {
        if ((int)(*target).MaxWallTime.GetPeriod() != -1) {     // Example: 123
          if (!((int)(*target).MaxWallTime.GetPeriod()) >= (int)jir.TotalWallTime.GetPeriod()) {
            logger.msg(DEBUG, "Matchmaking, TotalWallTime problem, ExecutionTarget: %s (MaxWallTime) JobDescription: %s (TotalWallTime)", (std::string)(*target).MaxWallTime, (std::string)jir.TotalWallTime);
            continue;
          }
        }
        else {
          //logger.msg(DEBUG, "Matchmaking, ExecutionTarget: %s, MaxWallTime is not defined", (std::string)(*target).url.str());
          //continue;
        }

        if ((int)(*target).MinWallTime.GetPeriod() != -1) {     // Example: 123
          if (!((int)(*target).MinWallTime.GetPeriod()) > (int)jir.TotalWallTime.GetPeriod()) {
            logger.msg(DEBUG, "Matchmaking, MinWallTime problem, ExecutionTarget: %s (MinWallTime) JobDescription: %s (TotalWallTime)", (std::string)(*target).MinWallTime, (std::string)jir.TotalWallTime);
            continue;
          }
        }
        else {
          //logger.msg(DEBUG, "Matchmaking, ExecutionTarget: %s, MinWallTime is not defined", (std::string)(*target).url.str());
          //continue;
        }
      }

      if ((int)jir.TotalCPUTime.GetPeriod() != -1) {
        if ((int)(*target).MaxCPUTime.GetPeriod() != -1) {     // Example: 456
          if (!((int)(*target).MaxCPUTime.GetPeriod()) >= (int)jir.TotalCPUTime.GetPeriod()) {
            logger.msg(DEBUG, "Matchmaking, TotalCPUTime problem, ExecutionTarget: %s (MaxCPUTime) JobDescription: %s (TotalCPUTime)", (std::string)(*target).MaxCPUTime, (std::string)jir.TotalCPUTime);
            continue;
          }
        }
        else {
          //logger.msg(DEBUG, "Matchmaking, ExecutionTarget: %s, MaxCPUTime is not defined", (std::string)(*target).url.str());
          //continue;
        }

        if ((int)(*target).MinCPUTime.GetPeriod() != -1) {     // Example: 456
          if (!((int)(*target).MinCPUTime.GetPeriod()) > (int)jir.TotalCPUTime.GetPeriod()) {
            logger.msg(DEBUG, "Matchmaking, MinCPUTime problem, ExecutionTarget: %s (MinCPUTime) JobDescription: %s (TotalCPUTime)", (std::string)(*target).MinCPUTime, (std::string)jir.TotalCPUTime);
            continue;
          }
        }
        else {
          //logger.msg(DEBUG, "Matchmaking, ExecutionTarget: %s, MinCPUTime is not defined", (std::string)(*target).url.str());
          //continue;
        }
      }

      if (jir.IndividualPhysicalMemory != -1) {
        if ((*target).MainMemorySize != -1) {     // Example: 678
          if (!((*target).MainMemorySize >= jir.IndividualPhysicalMemory)) {
            logger.msg(DEBUG, "Matchmaking, MainMemorySize problem, ExecutionTarget: %d (MainMemorySize), JobDescription: %d (IndividualPhysicalMemory)", (*target).MainMemorySize, jir.IndividualPhysicalMemory);
            continue;
          }
        }
        else if ((*target).MaxMainMemory != -1) {     // Example: 678
          if (!((*target).MaxMainMemory >= jir.IndividualPhysicalMemory)) {
            logger.msg(DEBUG, "Matchmaking, MaxMainMemory problem, ExecutionTarget: %d (MaxMainMemory), JobDescription: %d (IndividualPhysicalMemory)", (*target).MaxMainMemory, jir.IndividualPhysicalMemory);
            continue;
          }

        }
        else {
          logger.msg(DEBUG, "Matchmaking, ExecutionTarget: %s, MaxMainMemory and MainMemorySize are not defined", (std::string)(*target).url.str());
          continue;
        }
      }

      if (jir.IndividualVirtualMemory != -1) {
        if ((*target).MaxVirtualMemory != -1) {     // Example: 678
          if (!((*target).MaxVirtualMemory >= jir.IndividualVirtualMemory)) {
            logger.msg(DEBUG, "Matchmaking, MaxVirtualMemory problem, ExecutionTarget: %d (MaxVirtualMemory), JobDescription: %d (IndividualVirtualMemory)", (*target).MaxVirtualMemory, jir.IndividualVirtualMemory);
            continue;
          }
        }
        else {
          logger.msg(DEBUG, "Matchmaking, ExecutionTarget: %s, MaxVirtualMemory is not defined", (std::string)(*target).url.str());
          continue;
        }
      }

      if (!jir.Platform.empty()) {
        if (!(*target).Platform.empty()) {    // Example: i386
          if ((*target).Platform != jir.Platform) {
            logger.msg(DEBUG, "Matchmaking, Platform problem, ExecutionTarget: %s (Platform) JobDescription: %s (Platform)", (*target).Platform, jir.Platform);
            continue;
          }
        }
        else {
          logger.msg(DEBUG, "Matchmaking, ExecutionTarget:  %s, Platform is not defined", (std::string)(*target).url.str());
          continue;
        }
      }

      if (!jir.OSFamily.empty()) {
        if (!(*target).OSFamily.empty()) {    // Example: linux
          if ((*target).OSFamily != jir.OSFamily) {
            logger.msg(DEBUG, "Matchmaking, OSFamily problem, ExecutionTarget: %s (OSFamily) JobDescription: %s (OSFamily)", (*target).OSFamily, jir.OSFamily);
            continue;
          }
        }
        else {
          logger.msg(DEBUG, "Matchmaking, ExecutionTarget:  %s, OSFamily is not defined", (std::string)(*target).url.str());
          continue;
        }
      }

      if (!jir.OSName.empty()) {
        if (!(*target).OSName.empty()) {    // Example: ubuntu
          if ((*target).OSName != jir.OSName) {
            logger.msg(DEBUG, "Matchmaking, OSName problem, ExecutionTarget: %s (OSName) JobDescription: %s (OSName)", (*target).OSName, jir.OSName);
            continue;
          }
        }
        else {
          logger.msg(DEBUG, "Matchmaking, ExecutionTarget:  %s, OSName is not defined", (std::string)(*target).url.str());
          continue;
        }
      }

      if (!jir.OSVersion.empty()) {
        if (!(*target).OSVersion.empty()) {    // Example: 4.3.1
          std::string a = "OSVersion-" + (*target).OSVersion;
          std::string b = "OSVersion-" + jir.OSVersion;
          RuntimeEnvironment RE0(a);
          RuntimeEnvironment RE1(b);
          if (RE0 < RE1) {
            logger.msg(DEBUG, "Matchmaking, OSVersion problem, ExecutionTarget: %s (OSVersion) JobDescription: %s (OSVersion)", (*target).OSVersion, jir.OSVersion);
            continue;
          }
        }
        else {
          logger.msg(DEBUG, "Matchmaking, ExecutionTarget:  %s, OSVersion is not defined", (std::string)(*target).url.str());
          continue;
        }
      }

      if (!jir.RunTimeEnvironment.empty()) {
        if (!(*target).ApplicationEnvironments.empty()) {   // Example: ATLAS-9.0.3
          std::list<Arc::RunTimeEnvironmentType>::const_iterator iter1;
          bool next_target;
          bool match;
          for (iter1 = jir.RunTimeEnvironment.begin(); iter1 != jir.RunTimeEnvironment.end(); iter1++) {
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
            // end of jir's version list parsing
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

      if (!jir.NetworkInfo.empty())
        if (!(*target).NetworkInfo.empty()) {    // Example: infiniband
          if (std::find(target->NetworkInfo.begin(), target->NetworkInfo.end(),
                        jir.NetworkInfo) == target->NetworkInfo.end())
            // logger.msg(DEBUG, "Matchmaking, NetworkInfo problem, ExecutionTarget: %s (NetworkInfo) JobDescription: %s (NetworkInfo)", (*target).NetworkInfo, jir.NetworkInfo);
            continue;
          else {
            logger.msg(DEBUG, "Matchmaking, ExecutionTarget:  %s, NetworkInfo is not defined", (std::string)(*target).url.str());
            continue;
          }
        }

      if (jir.SessionDiskSpace != -1) {
        if ((*target).MaxDiskSpace != -1) {     // Example: 5656
          if (!((*target).MaxDiskSpace >= jir.SessionDiskSpace)) {
            logger.msg(DEBUG, "Matchmaking, MaxDiskSpace problem, ExecutionTarget: %s (MaxDiskSpace) JobDescription: %s (SessionDiskSpace)", (*target).MaxDiskSpace, jir.SessionDiskSpace);
            continue;
          }
        }
        else if ((*target).WorkingAreaTotal != -1) {     // Example: 5656
          if (!((*target).WorkingAreaTotal >= jir.SessionDiskSpace)) {
            logger.msg(DEBUG, "Matchmaking, WorkingAreaTotal problem, ExecutionTarget: %s (WorkingAreaTotal) JobDescription: %s (SessionDiskSpace)", (*target).WorkingAreaTotal, jir.SessionDiskSpace);
            continue;
          }
        }
        else {
          logger.msg(DEBUG, "Matchmaking, ExecutionTarget:  %s, MaxDiskSpace and WorkingAreaTotal are not defined", (std::string)(*target).url.str());
          continue;
        }
      }

      if (jir.DiskSpace != -1 && jir.CacheDiskSpace != -1) {
        if ((*target).MaxDiskSpace != -1) {     // Example: 5656
          if (!((*target).MaxDiskSpace >= (jir.DiskSpace - jir.CacheDiskSpace))) {
            logger.msg(DEBUG, "Matchmaking, MaxDiskSpace >= DiskSpace - CacheDiskSpace problem, ExecutionTarget: %d (MaxDiskSpace) JobDescription: %d (DiskSpace) - %d (CacheDiskSpace)", (*target).MaxDiskSpace, jir.DiskSpace, jir.CacheDiskSpace);
            continue;
          }
        }
        else if ((*target).WorkingAreaTotal != -1) {     // Example: 5656
          if (!((*target).WorkingAreaTotal >= (jir.DiskSpace - jir.CacheDiskSpace))) {
            logger.msg(DEBUG, "Matchmaking, WorkingAreaTotal >= DiskSpace - CacheDiskSpace problem, ExecutionTarget: %d (MaxDiskSpace) JobDescription: %d (DiskSpace) - %d (CacheDiskSpace)", (*target).WorkingAreaTotal, jir.DiskSpace, jir.CacheDiskSpace);
            continue;
          }
        }
        else {
          logger.msg(DEBUG, "Matchmaking, ExecutionTarget:  %s, MaxDiskSpace and WorkingAreaTotal are not defined", (std::string)(*target).url.str());
          continue;
        }
      }

      if (jir.DiskSpace != -1) {
        if ((*target).MaxDiskSpace != -1) {     // Example: 5656
          if (!((*target).MaxDiskSpace >= jir.DiskSpace)) {
            logger.msg(DEBUG, "Matchmaking, MaxDiskSpace problem, ExecutionTarget: %d (MaxDiskSpace) JobDescription: %d (DiskSpace)", (*target).MaxDiskSpace, jir.DiskSpace);
            continue;
          }
        }
        else if ((*target).WorkingAreaTotal != -1) {     // Example: 5656
          if (!((*target).WorkingAreaTotal >= jir.DiskSpace)) {
            logger.msg(DEBUG, "Matchmaking, WorkingAreaTotal problem, ExecutionTarget: %d (WorkingAreaTotal) JobDescription: %d (DiskSpace)", (*target).WorkingAreaTotal, jir.DiskSpace);
            continue;
          }
        }
        else {
          logger.msg(DEBUG, "Matchmaking, ExecutionTarget:  %s, MaxDiskSpace and WorkingAreaTotal are not defined", (std::string)(*target).url.str());
          continue;
        }
      }

      if (jir.CacheDiskSpace != -1) {
        if ((*target).CacheTotal != -1) {     // Example: 5656
          if (!((*target).CacheTotal >= jir.CacheDiskSpace)) {
            logger.msg(DEBUG, "Matchmaking, CacheTotal problem, ExecutionTarget: %d (CacheTotal) JobDescription: %d (CacheDiskSpace)", (*target).CacheTotal, jir.CacheDiskSpace);
            continue;
          }
        }
        else {
          logger.msg(DEBUG, "Matchmaking, ExecutionTarget:  %s, CacheTotal is not defined", (std::string)(*target).url.str());
          continue;
        }
      }

      if (jir.Slots != -1) {
        if ((*target).TotalSlots != -1) {     // Example: 5656
          if (!((*target).TotalSlots >= jir.Slots)) {
            logger.msg(DEBUG, "Matchmaking, TotalSlots problem, ExecutionTarget: %d (TotalSlots) JobDescription: %d (Slots)", (*target).TotalSlots, jir.Slots);
            continue;
          }
        }
        else if ((*target).MaxSlotsPerJob != -1) {     // Example: 5656
          if (!((*target).MaxSlotsPerJob >= jir.Slots)) {
            logger.msg(DEBUG, "Matchmaking, MaxSlotsPerJob problem, ExecutionTarget: %d (MaxSlotsPerJob) JobDescription: %d (Slots)", (*target).MaxSlotsPerJob, jir.Slots);
            continue;
          }
        }
        else {
          logger.msg(DEBUG, "Matchmaking, ExecutionTarget:  %s, TotalSlots and MaxSlotsPerJob are not defined", (std::string)(*target).url.str());
          continue;
        }
      }

      if (jir.NumberOfProcesses != -1) {
        if ((*target).TotalSlots != -1) {     // Example: 5656
          if (!((*target).TotalSlots >= jir.NumberOfProcesses)) {
            logger.msg(DEBUG, "Matchmaking, TotalSlots problem, ExecutionTarget: %d (TotalSlots) JobDescription: %d (NumberOfProcesses)", (*target).TotalSlots, jir.NumberOfProcesses);
            continue;
          }
        }
        else if ((*target).MaxSlotsPerJob != -1) {     // Example: 5656
          if (!((*target).MaxSlotsPerJob >= jir.NumberOfProcesses)) {
            logger.msg(DEBUG, "Matchmaking, MaxSlotsPerJob problem, ExecutionTarget: %d (MaxSlotsPerJob) JobDescription: %d (NumberOfProcesses)", (*target).TotalSlots, jir.NumberOfProcesses);
            continue;
          }
        }
        else {
          logger.msg(DEBUG, "Matchmaking, ExecutionTarget:  %s, TotalSlots and MaxSlotsPerJob are not defined", (std::string)(*target).url.str());
          continue;
        }
      }

      if ((int)jir.SessionLifeTime.GetPeriod() != -1) {
        if ((int)(*target).WorkingAreaLifeTime.GetPeriod() != -1) {     // Example: 123
          if (!((int)(*target).WorkingAreaLifeTime.GetPeriod()) >= (int)jir.SessionLifeTime.GetPeriod()) {
            logger.msg(DEBUG, "Matchmaking, WorkingAreaLifeTime problem, ExecutionTarget: %s (WorkingAreaLifeTime) JobDescription: %s (SessionLifeTime)", (std::string)(*target).WorkingAreaLifeTime, (std::string)jir.SessionLifeTime);
            continue;
          }
        }
        else {
          logger.msg(DEBUG, "Matchmaking, ExecutionTarget:  %s, WorkingAreaLifeTime is not defined", (std::string)(*target).url.str());
          continue;
        }
      }

      if (jir.InBound)
        if (!(*target).ConnectivityIn) {     // Example: false (boolean)
          logger.msg(DEBUG, "Matchmaking, ConnectivityIn problem, ExecutionTarget: %s (ConnectivityIn) JobDescription: %s (InBound)", (jir.InBound ? "true" : "false"), ((*target).ConnectivityIn ? "true" : "false"));
          continue;
        }

      if (jir.OutBound)
        if (!(*target).ConnectivityOut) {     // Example: false (boolean)
          logger.msg(DEBUG, "Matchmaking, ConnectivityOut problem, ExecutionTarget: %s (ConnectivityOut) JobDescription: %s (OutBound)", (jir.OutBound ? "true" : "false"), ((*target).ConnectivityOut ? "true" : "false"));
          continue;
        }

      if (!jir.ReferenceTime.empty()) {

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

    std::vector<Arc::ExecutionTarget>::iterator ret_pointer;
    ret_pointer = current;
    current++;

    if (ret_pointer == PossibleTargets.end())
      EndOfList = true;

    return *ret_pointer;
  }

} // namespace Arc
