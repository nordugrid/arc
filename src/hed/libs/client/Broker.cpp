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
#include <arc/credential/Credential.h>
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
      if (!rejectTargets.empty()) {
        std::list<URL>::const_iterator it = rejectTargets.begin();
        for (; it != rejectTargets.end(); ++it) {
          if ((*it) == target->ComputingEndpoint.URLString || (*it) == target->Cluster) {
            // Target should be dropped.
            break;
          }
        }

        // If loop did not reach the end, target should be dropped.
        if (it != rejectTargets.end()) {
          logger.msg(VERBOSE, "Target (%s) was explicitly rejected.", target->ComputingEndpoint.URLString);
          continue;
        }
      }

      if (Match(*target, jobdesc)) {
        PossibleTargets.push_back(&*target);
      }
    }

    logger.msg(VERBOSE, "Possible targets after prefiltering: %d", PossibleTargets.size());

    std::list<ExecutionTarget*>::iterator iter = PossibleTargets.begin();

    for (int i = 1; iter != PossibleTargets.end(); iter++, i++) {
      logger.msg(VERBOSE, "%d. Resource: %s; Queue: %s", i, (*iter)->AdminDomain.Name, (*iter)->ComputingShare.Name);
      logger.msg(VERBOSE, "Health State: %s", (*iter)->ComputingEndpoint.HealthState);
    }

    TargetSortingDone = false;
  }

  bool Broker::Match(const ExecutionTarget& t, const JobDescription& j) const {
    Credential credential(usercfg);
    std::string proxyDN = credential.GetDN();
    std::string proxyIssuerCA = credential.GetCAName();

    logger.msg(VERBOSE, "Performing matchmaking against target (%s).", t.ComputingEndpoint.URLString);

    if ( !(t.ComputingEndpoint.TrustedCA.empty()) && (find(t.ComputingEndpoint.TrustedCA.begin(), t.ComputingEndpoint.TrustedCA.end(), proxyIssuerCA)
            == t.ComputingEndpoint.TrustedCA.end()) ){
        logger.msg(VERBOSE, "The CA issuer (%s) of the credentials (%s) is not trusted by the target (%s).", proxyIssuerCA, proxyDN, t.ComputingEndpoint.URLString);
        return false;
    }

    std::map<std::string, std::string>::const_iterator itAtt;
    if ((itAtt = job->OtherAttributes.find("nordugrid:broker;reject_queue")) != job->OtherAttributes.end()) {
      if (t.ComputingShare.Name.empty()) {
        logger.msg(VERBOSE, "ComputingShareName of ExecutionTarget (%s) is not defined", t.ComputingEndpoint.URLString);
        return false;
      }

      if (t.ComputingShare.Name == itAtt->second) {
        logger.msg(VERBOSE, "ComputingShare (%s) explicitly rejected", itAtt->second);
        return false;
      }
    }

    if (!job->Resources.QueueName.empty()) {
      if (t.ComputingShare.Name.empty()) {
        logger.msg(VERBOSE, "ComputingShareName of ExecutionTarget (%s) is not defined", t.ComputingEndpoint.URLString);
        return false;
      }

      if (t.ComputingShare.Name != job->Resources.QueueName) {
        logger.msg(VERBOSE, "ComputingShare (%s) does not match selected queue (%s)", t.ComputingShare.Name, job->Resources.QueueName);
        return false;
      }
    }

    if ((int)job->Application.ProcessingStartTime.GetTime() != -1) {
      if ((int)t.ComputingEndpoint.DowntimeStarts.GetTime() != -1 && (int)t.ComputingEndpoint.DowntimeEnds.GetTime() != -1) {
        if (t.ComputingEndpoint.DowntimeStarts <= job->Application.ProcessingStartTime && job->Application.ProcessingStartTime <= t.ComputingEndpoint.DowntimeEnds) {
          logger.msg(VERBOSE, "ProcessingStartTime (%s) specified in job description is inside the targets downtime period [ %s - %s ].", (std::string)job->Application.ProcessingStartTime, (std::string)t.ComputingEndpoint.DowntimeStarts, (std::string)t.ComputingEndpoint.DowntimeEnds);
          return false;
        }
      }
      else
        logger.msg(WARNING, "The downtime of the target (%s) is not published. Keeping target.", t.ComputingEndpoint.URLString);
    }

    if (!t.ComputingEndpoint.HealthState.empty()) {

      if (t.ComputingEndpoint.HealthState != "ok") { // Enumeration for healthstate: ok, critical, other, unknown, warning
        logger.msg(VERBOSE, "HealthState of ExecutionTarget (%s) is not OK (%s)", t.ComputingEndpoint.URLString, t.ComputingEndpoint.HealthState);
        return false;
      }
    }
    else {
      logger.msg(VERBOSE, "Matchmaking, ExecutionTarget:  %s, HealthState is not defined", t.ComputingEndpoint.URLString);
      return false;
    }

    if (!job->Resources.CEType.empty()) {
      if (!t.ComputingEndpoint.Implementation().empty()) {
        if (!job->Resources.CEType.isSatisfied(t.ComputingEndpoint.Implementation)) {
          logger.msg(VERBOSE, "Matchmaking, Computing endpoint requirement not satisfied. ExecutionTarget: %s", (std::string)t.ComputingEndpoint.Implementation);
          return false;
        }
      }
      else {
        logger.msg(VERBOSE, "Matchmaking, ExecutionTarget:  %s, ImplementationName is not defined", t.ComputingEndpoint.URLString);
        return false;
      }
    }

    {
      typedef std::pair<std::string, int> EtTimePair;
      EtTimePair etTime[] = {EtTimePair("MaxWallTime", (int)t.ComputingShare.MaxWallTime.GetPeriod()),
                             EtTimePair("MinWallTime", (int)t.ComputingShare.MinWallTime.GetPeriod()),
                             EtTimePair("MaxCPUTime", (int)t.ComputingShare.MaxCPUTime.GetPeriod()),
                             EtTimePair("MinCPUTime", (int)t.ComputingShare.MinCPUTime.GetPeriod())};

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
                return false;
              }
            }
            else { // Benchmark defined => scale using benchmark.
              double targetBenchmark = -1.;
              for (std::map<std::string, double>::const_iterator itTBench = t.Benchmarks.begin();
                   itTBench != t.Benchmarks.end(); itTBench++) {
                if (lower(jTime->second->benchmark.first) == lower(itTBench->first)) {
                  targetBenchmark = itTBench->second;
                  break;
                }
              }

              // Make it possible to scale according to clock rate.
              if (targetBenchmark <= 0. && lower(jTime->second->benchmark.first) == "clock rate") {
                targetBenchmark = (t.CPUClockSpeed > 0. ? (double)t.CPUClockSpeed : 1000.);
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
                  return false;
                }
              }
              else {
                logger.msg(VERBOSE, "Matchmaking, Benchmark %s is not published by the ExecutionTarget.", jTime->second->benchmark.first);
                return false;
              }
            }
          }
          // Do not drop target if it does not publish attribute.
        }
      }
    }

    if (job->Resources.IndividualPhysicalMemory != -1) {
      if (t.MainMemorySize != -1) {     // Example: 678
        if (t.MainMemorySize < job->Resources.IndividualPhysicalMemory) {
          logger.msg(VERBOSE, "Matchmaking, MainMemorySize problem, ExecutionTarget: %d (MainMemorySize), JobDescription: %d (IndividualPhysicalMemory)", t.MainMemorySize, job->Resources.IndividualPhysicalMemory.max);
          return false;
        }
      }
      else if (t.ComputingShare.MaxMainMemory != -1) {     // Example: 678
        if (t.ComputingShare.MaxMainMemory < job->Resources.IndividualPhysicalMemory) {
          logger.msg(VERBOSE, "Matchmaking, MaxMainMemory problem, ExecutionTarget: %d (MaxMainMemory), JobDescription: %d (IndividualPhysicalMemory)", t.ComputingShare.MaxMainMemory, job->Resources.IndividualPhysicalMemory.max);
          return false;
        }

      }
      else {
        logger.msg(VERBOSE, "Matchmaking, ExecutionTarget: %s, MaxMainMemory and MainMemorySize are not defined", t.ComputingEndpoint.URLString);
        return false;
      }
    }

    if (job->Resources.IndividualVirtualMemory != -1) {
      if (t.ComputingShare.MaxVirtualMemory != -1) {     // Example: 678
        if (t.ComputingShare.MaxVirtualMemory < job->Resources.IndividualVirtualMemory) {
          logger.msg(VERBOSE, "Matchmaking, MaxVirtualMemory problem, ExecutionTarget: %d (MaxVirtualMemory), JobDescription: %d (IndividualVirtualMemory)", t.ComputingShare.MaxVirtualMemory, job->Resources.IndividualVirtualMemory.max);
          return false;
        }
      }
      else {
        logger.msg(VERBOSE, "Matchmaking, ExecutionTarget: %s, MaxVirtualMemory is not defined", t.ComputingEndpoint.URLString);
        return false;
      }
    }

    if (!job->Resources.Platform.empty()) {
      if (!t.Platform.empty()) {    // Example: i386
        if (t.Platform != job->Resources.Platform) {
          logger.msg(VERBOSE, "Matchmaking, Platform problem, ExecutionTarget: %s (Platform) JobDescription: %s (Platform)", t.Platform, job->Resources.Platform);
          return false;
        }
      }
      else {
        logger.msg(VERBOSE, "Matchmaking, ExecutionTarget:  %s, Platform is not defined", t.ComputingEndpoint.URLString);
        return false;
      }
    }

    if (!job->Resources.OperatingSystem.empty()) {
      if (!t.OperatingSystem.empty()) {
        if (!job->Resources.OperatingSystem.isSatisfied(t.OperatingSystem)) {
          logger.msg(VERBOSE, "Matchmaking, ExecutionTarget: %s, OperatingSystem requirements not satisfied", t.ComputingEndpoint.URLString);
          return false;
        }
      }
      else {
        logger.msg(VERBOSE, "Matchmaking, ExecutionTarget:  %s,  OperatingSystem is not defined", t.ComputingEndpoint.URLString);
        return false;
      }
    }

    if (!job->Resources.RunTimeEnvironment.empty()) {
      if (!t.ApplicationEnvironments.empty()) {
        if (!job->Resources.RunTimeEnvironment.isSatisfied(t.ApplicationEnvironments)) {
          logger.msg(VERBOSE, "Matchmaking, ExecutionTarget:  %s, RunTimeEnvironment requirements not satisfied", t.ComputingEndpoint.URLString);
          return false;
        }
      }
      else {
        logger.msg(VERBOSE, "Matchmaking, ExecutionTarget: %s, ApplicationEnvironments not defined", t.ComputingEndpoint.URLString);
        return false;
      }
    }

    if (!job->Resources.NetworkInfo.empty())
      if (!t.NetworkInfo.empty()) {    // Example: infiniband
        if (std::find(t.NetworkInfo.begin(), t.NetworkInfo.end(),
                      job->Resources.NetworkInfo) == t.NetworkInfo.end()) {
          logger.msg(VERBOSE, "Matchmaking, NetworkInfo demand not fulfilled, ExecutionTarget do not support %s, specified in the JobDescription.", job->Resources.NetworkInfo);
          return false;
        }
        else {
          logger.msg(VERBOSE, "Matchmaking, ExecutionTarget:  %s, NetworkInfo is not defined", t.ComputingEndpoint.URLString);
          return false;
        }
      }

    if (job->Resources.DiskSpaceRequirement.SessionDiskSpace > -1) {
      if (t.ComputingShare.MaxDiskSpace > -1) {     // Example: 5656
        if (t.ComputingShare.MaxDiskSpace*1024 < job->Resources.DiskSpaceRequirement.SessionDiskSpace) {
          logger.msg(VERBOSE, "Matchmaking, MaxDiskSpace problem, ExecutionTarget: %d MB (MaxDiskSpace); JobDescription: %d MB (SessionDiskSpace)", t.ComputingShare.MaxDiskSpace*1024, job->Resources.DiskSpaceRequirement.SessionDiskSpace);
          return false;
        }
      }

      if (t.WorkingAreaFree > -1) {     // Example: 5656
        if (t.WorkingAreaFree*1024 < job->Resources.DiskSpaceRequirement.SessionDiskSpace) {
          logger.msg(VERBOSE, "Matchmaking, WorkingAreaFree problem, ExecutionTarget: %d MB (WorkingAreaFree); JobDescription: %d MB (SessionDiskSpace)", t.WorkingAreaFree*1024, job->Resources.DiskSpaceRequirement.SessionDiskSpace);
          return false;
        }
      }

      if (t.ComputingShare.MaxDiskSpace <= -1 && t.WorkingAreaFree <= -1) {
        logger.msg(VERBOSE, "Matchmaking, ExecutionTarget:  %s, MaxDiskSpace and WorkingAreaFree are not defined", t.ComputingEndpoint.URLString);
        return false;
      }
    }

    if (job->Resources.DiskSpaceRequirement.DiskSpace.max > -1 && job->Resources.DiskSpaceRequirement.CacheDiskSpace > -1) {
      if (t.ComputingShare.MaxDiskSpace > -1) {     // Example: 5656
        if (t.ComputingShare.MaxDiskSpace*1024 < job->Resources.DiskSpaceRequirement.DiskSpace.max - job->Resources.DiskSpaceRequirement.CacheDiskSpace) {
          logger.msg(VERBOSE, "Matchmaking, MaxDiskSpace*1024 >= DiskSpace - CacheDiskSpace problem, ExecutionTarget: %d MB (MaxDiskSpace); JobDescription: %d MB (DiskSpace), %d MB (CacheDiskSpace)", t.ComputingShare.MaxDiskSpace*1024, job->Resources.DiskSpaceRequirement.DiskSpace.max, job->Resources.DiskSpaceRequirement.CacheDiskSpace);
          return false;
        }
      }

      if (t.WorkingAreaFree > -1) {     // Example: 5656
        if (t.WorkingAreaFree*1024 < job->Resources.DiskSpaceRequirement.DiskSpace.max - job->Resources.DiskSpaceRequirement.CacheDiskSpace) {
          logger.msg(VERBOSE, "Matchmaking, WorkingAreaFree*1024 >= DiskSpace - CacheDiskSpace problem, ExecutionTarget: %d MB (MaxDiskSpace); JobDescription: %d MB (DiskSpace), %d MB (CacheDiskSpace)", t.WorkingAreaFree*1024, job->Resources.DiskSpaceRequirement.DiskSpace.max, job->Resources.DiskSpaceRequirement.CacheDiskSpace);
          return false;
        }
      }

      if (t.ComputingShare.MaxDiskSpace <= -1 && t.WorkingAreaFree <= -1) {
        logger.msg(VERBOSE, "Matchmaking, ExecutionTarget:  %s, MaxDiskSpace and WorkingAreaFree are not defined", t.ComputingEndpoint.URLString);
        return false;
      }
    }

    if (job->Resources.DiskSpaceRequirement.DiskSpace.max > -1) {
      if (t.ComputingShare.MaxDiskSpace > -1) {     // Example: 5656
        if (t.ComputingShare.MaxDiskSpace*1024 < job->Resources.DiskSpaceRequirement.DiskSpace.max) {
          logger.msg(VERBOSE, "Matchmaking, MaxDiskSpace problem, ExecutionTarget: %d MB (MaxDiskSpace); JobDescription: %d MB (DiskSpace)", t.ComputingShare.MaxDiskSpace*1024, job->Resources.DiskSpaceRequirement.DiskSpace.max);
          return false;
        }
      }

      if (t.WorkingAreaFree > -1) {     // Example: 5656
        if (t.WorkingAreaFree*1024 < job->Resources.DiskSpaceRequirement.DiskSpace.max) {
          logger.msg(VERBOSE, "Matchmaking, WorkingAreaFree problem, ExecutionTarget: %d MB (WorkingAreaFree); JobDescription: %d MB (DiskSpace)", t.WorkingAreaFree*1024, job->Resources.DiskSpaceRequirement.DiskSpace.max);
          return false;
        }
      }

      if (t.WorkingAreaFree <= -1 && t.ComputingShare.MaxDiskSpace <= -1) {
        logger.msg(VERBOSE, "Matchmaking, ExecutionTarget:  %s, MaxDiskSpace and WorkingAreaFree are not defined", t.ComputingEndpoint.URLString);
        return false;
      }
    }

    if (job->Resources.DiskSpaceRequirement.CacheDiskSpace > -1) {
      if (t.CacheTotal > -1) {     // Example: 5656
        if (t.CacheTotal*1024 < job->Resources.DiskSpaceRequirement.CacheDiskSpace) {
          logger.msg(VERBOSE, "Matchmaking, CacheTotal problem, ExecutionTarget: %d MB (CacheTotal); JobDescription: %d MB (CacheDiskSpace)", t.CacheTotal*1024, job->Resources.DiskSpaceRequirement.CacheDiskSpace);
          return false;
        }
      }
      else {
        logger.msg(VERBOSE, "Matchmaking, ExecutionTarget:  %s, CacheTotal is not defined", t.ComputingEndpoint.URLString);
        return false;
      }
    }

    if (job->Resources.SlotRequirement.NumberOfSlots != -1) {
      if (t.TotalSlots != -1) {     // Example: 5656
        if (t.TotalSlots < job->Resources.SlotRequirement.NumberOfSlots) {
          logger.msg(VERBOSE, "Matchmaking, TotalSlots problem, ExecutionTarget: %d (TotalSlots) JobDescription: %d (NumberOfProcesses)", t.TotalSlots, job->Resources.SlotRequirement.NumberOfSlots);
          return false;
        }
      }
      if (t.ComputingShare.MaxSlotsPerJob != -1) {     // Example: 5656
        if (t.ComputingShare.MaxSlotsPerJob < job->Resources.SlotRequirement.NumberOfSlots) {
          logger.msg(VERBOSE, "Matchmaking, MaxSlotsPerJob problem, ExecutionTarget: %d (MaxSlotsPerJob) JobDescription: %d (NumberOfProcesses)", t.TotalSlots, job->Resources.SlotRequirement.NumberOfSlots);
          return false;
        }
      }

      if (t.TotalSlots == -1 && t.ComputingShare.MaxSlotsPerJob == -1) {
        logger.msg(VERBOSE, "Matchmaking, ExecutionTarget:  %s, TotalSlots and MaxSlotsPerJob are not defined", t.ComputingEndpoint.URLString);
        return false;
      }
    }

    if ((int)job->Resources.SessionLifeTime.GetPeriod() != -1) {
      if ((int)t.WorkingAreaLifeTime.GetPeriod() != -1) {     // Example: 123
        if (t.WorkingAreaLifeTime < job->Resources.SessionLifeTime) {
          logger.msg(VERBOSE, "Matchmaking, WorkingAreaLifeTime problem, ExecutionTarget: %s (WorkingAreaLifeTime) JobDescription: %s (SessionLifeTime)", (std::string)t.WorkingAreaLifeTime, (std::string)job->Resources.SessionLifeTime);
          return false;
        }
      }
      else {
        logger.msg(VERBOSE, "Matchmaking, ExecutionTarget:  %s, WorkingAreaLifeTime is not defined", t.ComputingEndpoint.URLString);
        return false;
      }
    }

    if ((job->Resources.NodeAccess == NAT_INBOUND ||
         job->Resources.NodeAccess == NAT_INOUTBOUND) &&
        !t.ConnectivityIn) {     // Example: false (boolean)
      logger.msg(VERBOSE, "Matchmaking, ConnectivityIn problem, ExecutionTarget: %s (ConnectivityIn) JobDescription: %s (InBound)", (job->Resources.NodeAccess == NAT_INBOUND ? "INBOUND" : "INOUTBOUND"), (t.ConnectivityIn ? "true" : "false"));
      return false;
    }

    if ((job->Resources.NodeAccess == NAT_OUTBOUND ||
         job->Resources.NodeAccess == NAT_INOUTBOUND) &&
        !t.ConnectivityOut) {     // Example: false (boolean)
      logger.msg(VERBOSE, "Matchmaking, ConnectivityOut problem, ExecutionTarget: %s (ConnectivityOut) JobDescription: %s (OutBound)", (job->Resources.NodeAccess == NAT_OUTBOUND ? "OUTBOUND" : "INOUTBOUND"), (t.ConnectivityIn ? "true" : "false"));
      return false;
    }

    return true;
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
        if ( i-1 == 0 ) logger.msg(Arc::INFO, "For this middleware there are no testjobs defined.");
        else logger.msg(Arc::INFO, "For this middleware only %s testjobs are defined.", ids.str());
      } else {
        if ((*it)->Submit(usercfg, jobdescription, job)) {
          current = it;
          RegisterJobsubmission();
          return true;
        }
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

    if ((*current)->ComputingShare.FreeSlots >= job->Resources.SlotRequirement.NumberOfSlots) {
      (*current)->ComputingShare.FreeSlots -= job->Resources.SlotRequirement.NumberOfSlots;
      if ((*current)->ComputingShare.UsedSlots != -1) {
        (*current)->ComputingShare.UsedSlots += job->Resources.SlotRequirement.NumberOfSlots;
      }
    }
    else if ((*current)->ComputingShare.WaitingJobs != -1) {
      (*current)->ComputingShare.WaitingJobs += job->Resources.SlotRequirement.NumberOfSlots;
    }

    logger.msg(DEBUG, "FreeSlots = %d; UsedSlots = %d; WaitingJobs = %d", (*current)->ComputingShare.FreeSlots, (*current)->ComputingShare.UsedSlots, (*current)->ComputingShare.WaitingJobs);
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
