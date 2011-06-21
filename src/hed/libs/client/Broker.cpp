// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <algorithm>

#include <arc/StringConv.h>
#include <arc/ArcConfig.h>
#include <arc/client/Broker.h>
#include <arc/client/ExecutionTarget.h>
#include <arc/client/Job.h>
#include <arc/loader/FinderLoader.h>

namespace Arc {

  Logger Broker::logger(Logger::getRootLogger(), "Broker");

  Broker::Broker(const UserConfig& usercfg)
    : usercfg(usercfg),
      TargetSortingDone(false),
      job(NULL),
      pCurrent(NULL) {}

  Broker::~Broker() {}

  void Broker::PreFilterTargets(std::list<ExecutionTarget>& targets,
                                const JobDescription& jobdesc,
                                const std::list<URL>& rejectTargets) {
    pCurrent = NULL;
    PossibleTargets.clear();
    job = &jobdesc;

    for (std::list<ExecutionTarget>::iterator target = targets.begin();
         target != targets.end(); target++) {
      logger.msg(VERBOSE, "Performing matchmaking against target (%s).", target->url.str());

      if (!rejectTargets.empty()) {
        std::list<URL>::const_iterator it = rejectTargets.begin();
        for (; it != rejectTargets.end(); ++it) {
          if ((*it) == target->url || (*it) == target->Cluster) {
            // Target should be dropped.
            break;
          }
        }

        // If loop did not reach the end, target should be dropped.
        if (it != rejectTargets.end()) {
          logger.msg(VERBOSE, "Target (%s) was explicitly rejected.", target->url.str());
          continue;
        }
      }

      std::map<std::string, std::string>::const_iterator itAtt;
      if ((itAtt = job->OtherAttributes.find("nordugrid:broker;reject_queue")) != job->OtherAttributes.end()) {
        if (target->ComputingShareName.empty()) {
          logger.msg(VERBOSE, "ComputingShareName of ExecutionTarget (%s) is not defined", target->url.str());
          continue;
        }

        if (target->ComputingShareName == itAtt->second) {
          logger.msg(VERBOSE, "ComputingShare (%s) explicitly rejected", itAtt->second);
          continue;
        }
      }

      if (!job->Resources.QueueName.empty()) {
        if (target->ComputingShareName.empty()) {
          logger.msg(VERBOSE, "ComputingShareName of ExecutionTarget (%s) is not defined", target->url.str());
          continue;
        }

        if (target->ComputingShareName != job->Resources.QueueName) {
          logger.msg(VERBOSE, "ComputingShare (%s) does not match selected queue (%s)", target->ComputingShareName, job->Resources.QueueName);
          continue;
        }
      }

      if ((int)job->Application.ProcessingStartTime.GetTime() != -1) {
        if ((int)target->DowntimeStarts.GetTime() != -1 && (int)target->DowntimeEnds.GetTime() != -1) {
          if (target->DowntimeStarts <= job->Application.ProcessingStartTime && job->Application.ProcessingStartTime <= target->DowntimeEnds) {
            logger.msg(VERBOSE, "ProcessingStartTime (%s) specified in job description is inside the targets downtime period [ %s - %s ].", (std::string)job->Application.ProcessingStartTime, (std::string)target->DowntimeStarts, (std::string)target->DowntimeEnds);
            continue;
          }
        }
        else
          logger.msg(WARNING, "The downtime of the target (%s) is not published. Keeping target.", target->url.str());
      }

      if (!target->HealthState.empty()) {

        if (target->HealthState != "ok") { // Enumeration for healthstate: ok, critical, other, unknown, warning
          logger.msg(VERBOSE, "HealthState of ExecutionTarget (%s) is not OK (%s)", target->url.str(), target->HealthState);
          continue;
        }
      }
      else {
        logger.msg(VERBOSE, "Matchmaking, ExecutionTarget:  %s, HealthState is not defined", target->url.str());
        continue;
      }

      if (!job->Resources.CEType.empty()) {
        if (!target->Implementation().empty()) {
          if (!job->Resources.CEType.isSatisfied(target->Implementation)) {
            logger.msg(VERBOSE, "Matchmaking, Computing endpoint requirement not satisfied. ExecutionTarget: %s", (std::string)target->Implementation);
            continue;
          }
        }
        else {
          logger.msg(VERBOSE, "Matchmaking, ExecutionTarget:  %s, ImplementationName is not defined", target->url.str());
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

        int i = 0;
        for (; i < 4; i++) {
          JobTimePair *jTime = &jobTime[i/2];
          if (((i%2 == 0) && (jTime->second->range.max != -1)) ||
              ((i%2 == 1) && (jTime->second->range.min != -1))) {
            if (etTime[i].second != -1) {
              if (jTime->second->benchmark.first.empty()) { // No benchmark defined, do not scale.
                if (((i%2 == 0) && (jTime->second->range.max > etTime[i].second)) ||
                    ((i%2 == 1) && (jTime->second->range.min < etTime[i].second))) {
                  logger.msg(VERBOSE,
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
                double targetBenchmark = -1.;
                for (std::map<std::string, double>::const_iterator itTBench = target->Benchmarks.begin();
                     itTBench != target->Benchmarks.end(); itTBench++) {
                  if (lower(jTime->second->benchmark.first) == lower(itTBench->first)) {
                    targetBenchmark = itTBench->second;
                    break;
                  }
                }

                // Make it possible to scale according to clock rate.
                if (targetBenchmark <= 0. && lower(jTime->second->benchmark.first) == "clock rate") {
                  targetBenchmark = (target->CPUClockSpeed > 0. ? (double)target->CPUClockSpeed : 1000.);
                }

                if (targetBenchmark > 0.) {
                  if (((i%2 == 0) && (jTime->second->scaleMax(targetBenchmark) > etTime[i].second)) ||
                      ((i%2 == 1) && (jTime->second->scaleMin(targetBenchmark) < etTime[i].second))) {
                    logger.msg(VERBOSE,
                               "Matchmaking, The %s scaled %s (%d) is %s than the %s (%d) published by the ExecutionTarget.",
                               jTime->second->benchmark.first,
                               jTime->first,
                               (i%2 == 0 ? jTime->second->scaleMax(targetBenchmark)
                                         : jTime->second->scaleMin(targetBenchmark)),
                               (i%2 == 0 ? "greater" : "less"),
                               etTime[i].first,
                               etTime[i].second);
                    break;
                  }
                }
                else {
                  logger.msg(VERBOSE, "Matchmaking, Benchmark %s is not published by the ExecutionTarget.", jTime->second->benchmark.first);
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
            logger.msg(VERBOSE, "Matchmaking, MainMemorySize problem, ExecutionTarget: %d (MainMemorySize), JobDescription: %d (IndividualPhysicalMemory)", target->MainMemorySize, job->Resources.IndividualPhysicalMemory.max);
            continue;
          }
        }
        else if (target->MaxMainMemory != -1) {     // Example: 678
          if (target->MaxMainMemory < job->Resources.IndividualPhysicalMemory) {
            logger.msg(VERBOSE, "Matchmaking, MaxMainMemory problem, ExecutionTarget: %d (MaxMainMemory), JobDescription: %d (IndividualPhysicalMemory)", target->MaxMainMemory, job->Resources.IndividualPhysicalMemory.max);
            continue;
          }

        }
        else {
          logger.msg(VERBOSE, "Matchmaking, ExecutionTarget: %s, MaxMainMemory and MainMemorySize are not defined", target->url.str());
          continue;
        }
      }

      if (job->Resources.IndividualVirtualMemory != -1) {
        if (target->MaxVirtualMemory != -1) {     // Example: 678
          if (target->MaxVirtualMemory < job->Resources.IndividualVirtualMemory) {
            logger.msg(VERBOSE, "Matchmaking, MaxVirtualMemory problem, ExecutionTarget: %d (MaxVirtualMemory), JobDescription: %d (IndividualVirtualMemory)", target->MaxVirtualMemory, job->Resources.IndividualVirtualMemory.max);
            continue;
          }
        }
        else {
          logger.msg(VERBOSE, "Matchmaking, ExecutionTarget: %s, MaxVirtualMemory is not defined", target->url.str());
          continue;
        }
      }

      if (!job->Resources.Platform.empty()) {
        if (!target->Platform.empty()) {    // Example: i386
          if (target->Platform != job->Resources.Platform) {
            logger.msg(VERBOSE, "Matchmaking, Platform problem, ExecutionTarget: %s (Platform) JobDescription: %s (Platform)", target->Platform, job->Resources.Platform);
            continue;
          }
        }
        else {
          logger.msg(VERBOSE, "Matchmaking, ExecutionTarget:  %s, Platform is not defined", target->url.str());
          continue;
        }
      }

      if (!job->Resources.OperatingSystem.empty()) {
        if (!target->OperatingSystem.empty()) {
          if (!job->Resources.OperatingSystem.isSatisfied(target->OperatingSystem)) {
            logger.msg(VERBOSE, "Matchmaking, ExecutionTarget: %s, OperatingSystem requirements not satisfied", target->url.str());
            continue;
          }
        }
        else {
          logger.msg(VERBOSE, "Matchmaking, ExecutionTarget:  %s,  OperatingSystem is not defined", target->url.str());
          continue;
        }
      }

      if (!job->Resources.RunTimeEnvironment.empty()) {
        if (!target->ApplicationEnvironments.empty()) {
          if (!job->Resources.RunTimeEnvironment.isSatisfied(target->ApplicationEnvironments)) {
            logger.msg(VERBOSE, "Matchmaking, ExecutionTarget:  %s, RunTimeEnvironment requirements not satisfied", target->url.str());
            continue;
          }
        }
        else {
          logger.msg(VERBOSE, "Matchmaking, ExecutionTarget: %s, ApplicationEnvironments not defined", target->url.str());
          continue;
        }
      }

      if (!job->Resources.NetworkInfo.empty())
        if (!target->NetworkInfo.empty()) {    // Example: infiniband
          if (std::find(target->NetworkInfo.begin(), target->NetworkInfo.end(),
                        job->Resources.NetworkInfo) == target->NetworkInfo.end()) {
            logger.msg(VERBOSE, "Matchmaking, NetworkInfo demand not fulfilled, ExecutionTarget do not support %s, specified in the JobDescription.", job->Resources.NetworkInfo);
            continue;
          }
          else {
            logger.msg(VERBOSE, "Matchmaking, ExecutionTarget:  %s, NetworkInfo is not defined", target->url.str());
            continue;
          }
        }

      if (job->Resources.DiskSpaceRequirement.SessionDiskSpace != -1) {
        if (target->MaxDiskSpace != -1) {     // Example: 5656
          if (target->MaxDiskSpace < job->Resources.DiskSpaceRequirement.SessionDiskSpace) {
            logger.msg(VERBOSE, "Matchmaking, MaxDiskSpace problem, ExecutionTarget: %d (MaxDiskSpace) JobDescription: %d (SessionDiskSpace)", target->MaxDiskSpace, job->Resources.DiskSpaceRequirement.SessionDiskSpace);
            continue;
          }
        }
        else if (target->WorkingAreaTotal != -1) {     // Example: 5656
          if (target->WorkingAreaTotal < job->Resources.DiskSpaceRequirement.SessionDiskSpace) {
            logger.msg(VERBOSE, "Matchmaking, WorkingAreaTotal problem, ExecutionTarget: %d (WorkingAreaTotal) JobDescription: %d (SessionDiskSpace)", target->WorkingAreaTotal, job->Resources.DiskSpaceRequirement.SessionDiskSpace);
            continue;
          }
        }
        else {
          logger.msg(VERBOSE, "Matchmaking, ExecutionTarget:  %s, MaxDiskSpace and WorkingAreaTotal are not defined", target->url.str());
          continue;
        }
      }

      if (job->Resources.DiskSpaceRequirement.DiskSpace != -1 && job->Resources.DiskSpaceRequirement.CacheDiskSpace != -1) {
        if (target->MaxDiskSpace != -1) {     // Example: 5656
          if (target->MaxDiskSpace < job->Resources.DiskSpaceRequirement.DiskSpace - job->Resources.DiskSpaceRequirement.CacheDiskSpace) {
            logger.msg(VERBOSE, "Matchmaking, MaxDiskSpace >= DiskSpace - CacheDiskSpace problem, ExecutionTarget: %d (MaxDiskSpace) JobDescription: %d (DiskSpace) - %d (CacheDiskSpace)", target->MaxDiskSpace, job->Resources.DiskSpaceRequirement.DiskSpace.max, job->Resources.DiskSpaceRequirement.CacheDiskSpace);
            continue;
          }
        }
        else if (target->WorkingAreaTotal != -1) {     // Example: 5656
          if (target->WorkingAreaTotal < job->Resources.DiskSpaceRequirement.DiskSpace - job->Resources.DiskSpaceRequirement.CacheDiskSpace) {
            logger.msg(VERBOSE, "Matchmaking, WorkingAreaTotal >= DiskSpace - CacheDiskSpace problem, ExecutionTarget: %d (MaxDiskSpace) JobDescription: %d (DiskSpace) - %d (CacheDiskSpace)", target->WorkingAreaTotal, job->Resources.DiskSpaceRequirement.DiskSpace.max, job->Resources.DiskSpaceRequirement.CacheDiskSpace);
            continue;
          }
        }
        else {
          logger.msg(VERBOSE, "Matchmaking, ExecutionTarget:  %s, MaxDiskSpace and WorkingAreaTotal are not defined", target->url.str());
          continue;
        }
      }

      if (job->Resources.DiskSpaceRequirement.DiskSpace != -1) {
        if (target->MaxDiskSpace != -1) {     // Example: 5656
          if (target->MaxDiskSpace < job->Resources.DiskSpaceRequirement.DiskSpace) {
            logger.msg(VERBOSE, "Matchmaking, MaxDiskSpace problem, ExecutionTarget: %d (MaxDiskSpace) JobDescription: %d (DiskSpace)", target->MaxDiskSpace, job->Resources.DiskSpaceRequirement.DiskSpace.max);
            continue;
          }
        }
        else if (target->WorkingAreaTotal != -1) {     // Example: 5656
          if (target->WorkingAreaTotal < job->Resources.DiskSpaceRequirement.DiskSpace) {
            logger.msg(VERBOSE, "Matchmaking, WorkingAreaTotal problem, ExecutionTarget: %d (WorkingAreaTotal) JobDescription: %d (DiskSpace)", target->WorkingAreaTotal, job->Resources.DiskSpaceRequirement.DiskSpace.max);
            continue;
          }
        }
        else {
          logger.msg(VERBOSE, "Matchmaking, ExecutionTarget:  %s, MaxDiskSpace and WorkingAreaTotal are not defined", target->url.str());
          continue;
        }
      }

      if (job->Resources.DiskSpaceRequirement.CacheDiskSpace != -1) {
        if (target->CacheTotal != -1) {     // Example: 5656
          if (target->CacheTotal < job->Resources.DiskSpaceRequirement.CacheDiskSpace) {
            logger.msg(VERBOSE, "Matchmaking, CacheTotal problem, ExecutionTarget: %d (CacheTotal) JobDescription: %d (CacheDiskSpace)", target->CacheTotal, job->Resources.DiskSpaceRequirement.CacheDiskSpace);
            continue;
          }
        }
        else {
          logger.msg(VERBOSE, "Matchmaking, ExecutionTarget:  %s, CacheTotal is not defined", target->url.str());
          continue;
        }
      }

      if (job->Resources.SlotRequirement.NumberOfSlots != -1) {
        if (target->TotalSlots != -1) {     // Example: 5656
          if (target->TotalSlots < job->Resources.SlotRequirement.NumberOfSlots) {
            logger.msg(VERBOSE, "Matchmaking, TotalSlots problem, ExecutionTarget: %d (TotalSlots) JobDescription: %d (NumberOfProcesses)", target->TotalSlots, job->Resources.SlotRequirement.NumberOfSlots.max);
            continue;
          }
        }
        if (target->MaxSlotsPerJob != -1) {     // Example: 5656
          if (target->MaxSlotsPerJob < job->Resources.SlotRequirement.NumberOfSlots) {
            logger.msg(VERBOSE, "Matchmaking, MaxSlotsPerJob problem, ExecutionTarget: %d (MaxSlotsPerJob) JobDescription: %d (NumberOfProcesses)", target->TotalSlots, job->Resources.SlotRequirement.NumberOfSlots.max);
            continue;
          }
        }

        if (target->TotalSlots == -1 && target->MaxSlotsPerJob == -1) {
          logger.msg(VERBOSE, "Matchmaking, ExecutionTarget:  %s, TotalSlots and MaxSlotsPerJob are not defined", target->url.str());
          continue;
        }
      }

      if ((int)job->Resources.SessionLifeTime.GetPeriod() != -1) {
        if ((int)target->WorkingAreaLifeTime.GetPeriod() != -1) {     // Example: 123
          if (target->WorkingAreaLifeTime < job->Resources.SessionLifeTime) {
            logger.msg(VERBOSE, "Matchmaking, WorkingAreaLifeTime problem, ExecutionTarget: %s (WorkingAreaLifeTime) JobDescription: %s (SessionLifeTime)", (std::string)target->WorkingAreaLifeTime, (std::string)job->Resources.SessionLifeTime);
            continue;
          }
        }
        else {
          logger.msg(VERBOSE, "Matchmaking, ExecutionTarget:  %s, WorkingAreaLifeTime is not defined", target->url.str());
          continue;
        }
      }

      if ((job->Resources.NodeAccess == NAT_INBOUND ||
           job->Resources.NodeAccess == NAT_INOUTBOUND) &&
          !target->ConnectivityIn) {     // Example: false (boolean)
        logger.msg(VERBOSE, "Matchmaking, ConnectivityIn problem, ExecutionTarget: %s (ConnectivityIn) JobDescription: %s (InBound)", (job->Resources.NodeAccess == NAT_INBOUND ? "INBOUND" : "INOUTBOUND"), (target->ConnectivityIn ? "true" : "false"));
        continue;
      }

      if ((job->Resources.NodeAccess == NAT_OUTBOUND ||
           job->Resources.NodeAccess == NAT_INOUTBOUND) &&
          !target->ConnectivityOut) {     // Example: false (boolean)
        logger.msg(VERBOSE, "Matchmaking, ConnectivityOut problem, ExecutionTarget: %s (ConnectivityOut) JobDescription: %s (OutBound)", (job->Resources.NodeAccess == NAT_OUTBOUND ? "OUTBOUND" : "INOUTBOUND"), (target->ConnectivityIn ? "true" : "false"));
        continue;
      }

      PossibleTargets.push_back(&*target);

    } //end loop over all found targets

    logger.msg(VERBOSE, "Possible targets after prefiltering: %d", PossibleTargets.size());

    std::list<ExecutionTarget*>::iterator iter = PossibleTargets.begin();

    for (int i = 1; iter != PossibleTargets.end(); iter++, i++) {
      logger.msg(VERBOSE, "%d. Resource: %s; Queue: %s", i, (*iter)->DomainName, (*iter)->ComputingShareName);
      logger.msg(VERBOSE, "Health State: %s", (*iter)->HealthState);
    }

    TargetSortingDone = false;
  }

  const ExecutionTarget* Broker::GetBestTarget() {
    if (PossibleTargets.empty() || current == PossibleTargets.end()) {
      return NULL;
    }

    if (!TargetSortingDone) {
      logger.msg(DEBUG, "Target sorting not done, sorting them now");
      SortTargets();
      current = PossibleTargets.begin();
    }
    else {
      current++;
    }

    pCurrent = *current;

    return (current != PossibleTargets.end() ? *current : NULL);
  }

  void Broker::UseAllTargets(std::list<ExecutionTarget>& targets) {

    PossibleTargets.clear();

    for (std::list<ExecutionTarget>::iterator target = targets.begin();
         target != targets.end(); target++) {
      PossibleTargets.push_back(&*target);
    }

    TargetSortingDone = false;
  }

  bool Broker::Test(std::list<ExecutionTarget>& targets, const int& testid, Job& job) {
    // Skipping PreFiltering using every targets
    UseAllTargets(targets);

    if (PossibleTargets.empty()) {
      return false;
    }

    SortTargets();

    for (std::list<ExecutionTarget*>::iterator it = PossibleTargets.begin();
         it != PossibleTargets.end(); it++) {
      JobDescription jobdescription;
      // Return if test jobdescription is not defined
      if (!((*it)->GetTestJob(usercfg, testid, jobdescription))) {
        std::ostringstream ids;
        int i = 0;
        while ((*it)->GetTestJob(usercfg, ++i, jobdescription)) {
          if ( i-1 == 0 ) ids << i;
          else ids << ", " << i;
        }
        if ( i-1 == 0 ) logger.msg(Arc::ERROR, "For this middleware there are no testjobs defined.");
        else logger.msg(Arc::ERROR, "For this middleware only %s testjobs are defined.", ids.str());
        return false;
      }
      if ((*it)->Submit(usercfg, jobdescription, job)) {
        current = it;
        RegisterJobsubmission();
        return true;
      }
    }

    return false;
  }

  void Broker::Sort() {
    if (TargetSortingDone) {
      return;
    }

    if (!PossibleTargets.empty()) {
      SortTargets();
    }
    current = PossibleTargets.begin();
    pCurrent = *current;
    TargetSortingDone = true;
  }

  bool Broker::Submit(std::list<ExecutionTarget>& targets, const JobDescription& jobdescription, Job& job, const std::list<URL>& rejectTargets) {
    PreFilterTargets(targets, jobdescription, rejectTargets);
    if (PossibleTargets.empty()) {
      return false;
    }

    SortTargets();

    for (std::list<ExecutionTarget*>::iterator it = PossibleTargets.begin();
         it != PossibleTargets.end(); it++) {
      if ((*it)->Submit(usercfg, jobdescription, job)) {
        current = it;
        RegisterJobsubmission();
        return true;
      }
    }

    return false;
  }

  void Broker::RegisterJobsubmission() {
    if (!job || PossibleTargets.empty() || current == PossibleTargets.end() || !TargetSortingDone) {
      return;
    }

    if ((*current)->FreeSlots >= job->Resources.SlotRequirement.NumberOfSlots) {
      (*current)->FreeSlots -= job->Resources.SlotRequirement.NumberOfSlots;
      if ((*current)->UsedSlots != -1) {
        (*current)->UsedSlots += job->Resources.SlotRequirement.NumberOfSlots;
      }
    }
    else if ((*current)->WaitingJobs != -1) {
      (*current)->WaitingJobs += job->Resources.SlotRequirement.NumberOfSlots;
    }

    logger.msg(DEBUG, "FreeSlots = %d; UsedSlots = %d; WaitingJobs = %d", (*current)->FreeSlots, (*current)->UsedSlots, (*current)->WaitingJobs);
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

    if(!factory_->load(FinderLoader::GetLibrariesList(),
                       "HED:Broker", name)) {
      logger.msg(ERROR, "Broker plugin \"%s\" not found.", name);
      return NULL;
    }

    BrokerPluginArgument arg(usercfg);
    Broker *broker = factory_->GetInstance<Broker>("HED:Broker", name, &arg, false);

    if (!broker) {
      logger.msg(ERROR, "Broker %s could not be created", name);
      return NULL;
    }

    brokers.push_back(broker);
    logger.msg(INFO, "Loaded Broker %s", name);
    return broker;
  }

} // namespace Arc
