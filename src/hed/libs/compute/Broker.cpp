// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <algorithm>

#include <arc/StringConv.h>
#include <arc/ArcConfig.h>
#include <arc/compute/Broker.h>
#include <arc/compute/ExecutionTarget.h>
#include <arc/compute/Job.h>
#include <arc/credential/Credential.h>
#include <arc/loader/FinderLoader.h>

namespace Arc {

  Logger Broker::logger(Logger::getRootLogger(), "Broker");
  Logger ExecutionTargetSorter::logger(Logger::getRootLogger(), "ExecutionTargetSorter");

  Broker::Broker(const UserConfig& uc, const JobDescription& j, const std::string& name) : uc(uc), j(&j), p(getLoader().load(uc, j, name, false)) {
  }

  Broker::Broker(const UserConfig& uc, const std::string& name) : uc(uc), j(NULL), p(getLoader().load(uc, name, false)) {
  }

  Broker::Broker(const Broker& b) : uc(b.uc), j(b.j), p(b.p) {
    p = getLoader().copy(p.Ptr(), false);
  }

  Broker::~Broker() {
  }

  Broker& Broker::operator=(const Broker& b) {
    j = b.j;
    p = getLoader().copy(p.Ptr(), false);
    return *this;
  }

  bool Broker::operator() (const ExecutionTarget& lhs, const ExecutionTarget& rhs) const {
    return (bool)p?(*p)(lhs, rhs):true;
  }

  bool Broker::isValid(bool alsoCheckJobDescription) const {
    return (bool)p && (!alsoCheckJobDescription || j != NULL);
  }

  void Broker::set(const JobDescription& _j) const {
    if ((bool)p) { j = &_j; p->set(_j); };
  }

  bool Broker::match(const ExecutionTarget& t) const {
    logger.msg(VERBOSE, "Performing matchmaking against target (%s).", t.ComputingEndpoint->URLString);

    bool plugin_match = false;
    if(!p) {
      if(j) plugin_match = Broker::genericMatch(t,*j,uc);
    } else {
      plugin_match = p->match(t);
    }

    if (plugin_match) {
      logger.msg(VERBOSE, "Matchmaking, ExecutionTarget: %s matches job description", t.ComputingEndpoint->URLString);
    }
    return plugin_match;
  }

  bool decodeDN(std::string in, std::list<std::string>& out) {
    in = trim(in," ");
    if(in[0] == '/') { // /N1=V1/N2=V2 kind
      std::string::size_type pos = 0;
      while(true) {
        std::string item;
        pos = get_token(item, in, pos, "/");
        if((pos == std::string::npos) && item.empty()) break;
        std::string::size_type p = item.find('=');
        if(p == std::string::npos) {
          // Most probably this belongs to previous item
          if(out.size() > 0) {
            *out.begin() += "/"+item;
          } else {
            out.push_front(item);
          }
        } else {
          out.push_front(item);
        }
      }
    } else { // N2=V2,N1=V1 kind
      tokenize(in,out,",");
    }
    for(std::list<std::string>::iterator item = out.begin(); item != out.end(); ++item) {
      std::string::size_type p = item->find('=');
      if(p != std::string::npos) {
        *item = trim(item->substr(0,p)," ") + "=" + trim(item->substr(p+1)," ");
      } else {
        *item = trim(*item," ");
      }
    }
    return true;
  }

  static bool compareDN(const std::list<std::string>& dn1, const std::list<std::string>& dn2) {
    if(dn1.size() != dn2.size()) return false;
    std::list<std::string>::const_iterator d1 = dn1.begin();
    std::list<std::string>::const_iterator d2 = dn2.begin();
    while(d1 != dn1.end()) {
      if(*d1 != *d2) return false;
      ++d1; ++d2;
    }
    return true;
  }

  static bool compareDN(const std::string& dn1, const std::string& dn2) {
    std::list<std::string> dnl1;
    std::list<std::string> dnl2;
    if(!decodeDN(dn1,dnl1)) return false;
    if(!decodeDN(dn2,dnl2)) return false;
    return compareDN(dnl1,dnl2);
  }

  static std::list<std::string>::iterator findDN(std::list<std::string>::iterator first, std::list<std::string>::iterator last, const std::string& item) {
    for(;first != last;++first) {
      if(compareDN(*first,item)) break;
    }
    return first;
  }

  bool Broker::genericMatch(const ExecutionTarget& t, const JobDescription& j, const UserConfig& uc) {
    // Maybe Credential can be passed to plugins through one more set()

    std::map<std::string, std::string>::const_iterator itAtt;

    if ((itAtt = j.OtherAttributes.find("nordugrid:broker;ignore_ca")) == j.OtherAttributes.end()) {
      std::string proxyIssuerCA;
      std::string proxyDN;
      if (!uc.OToken().empty()) {
        // If we have token matching by X.509 has no sense because user can be authenticated by both.
        Credential credential(uc, std::string(2, '\0'));
        proxyDN = credential.GetDN();
        proxyIssuerCA = credential.GetCAName();
      }
      if ( !(proxyIssuerCA.empty()) && !(t.ComputingEndpoint->TrustedCA.empty()) && (findDN(t.ComputingEndpoint->TrustedCA.begin(), t.ComputingEndpoint->TrustedCA.end(), proxyIssuerCA)
              == t.ComputingEndpoint->TrustedCA.end()) ){
          logger.msg(VERBOSE, "The CA issuer (%s) of the credentials (%s) is not trusted by the target (%s).", proxyIssuerCA, proxyDN, t.ComputingEndpoint->URLString);
          return false;
      }
    }

    if ((itAtt = j.OtherAttributes.find("nordugrid:broker;reject_queue")) != j.OtherAttributes.end()) {
      if (t.ComputingShare->MappingQueue.empty()) {
        if (t.ComputingShare->Name.empty()) {
          logger.msg(VERBOSE, "ComputingShareName of ExecutionTarget (%s) is not defined", t.ComputingEndpoint->URLString);
          return false;
        }
        if (t.ComputingShare->Name == itAtt->second) {
          logger.msg(VERBOSE, "ComputingShare (%s) explicitly rejected", itAtt->second);
          return false;
        }
      } else {
        if (t.ComputingShare->MappingQueue == itAtt->second) {
          logger.msg(VERBOSE, "ComputingShare (%s) explicitly rejected", itAtt->second);
          return false;
        }
      }
    }

    if (!j.Resources.QueueName.empty()) {
      if (t.ComputingShare->MappingQueue.empty()) {
        if (t.ComputingShare->Name.empty()) {
          logger.msg(VERBOSE, "ComputingShareName of ExecutionTarget (%s) is not defined", t.ComputingEndpoint->URLString);
          return false;
        }
        if (t.ComputingShare->Name != j.Resources.QueueName) {
          logger.msg(VERBOSE, "ComputingShare (%s) does not match selected queue (%s)", t.ComputingShare->Name, j.Resources.QueueName);
          return false;
        }
      } else {
        if (t.ComputingShare->MappingQueue != j.Resources.QueueName) {
          logger.msg(VERBOSE, "ComputingShare (%s) does not match selected queue (%s)", t.ComputingShare->MappingQueue, j.Resources.QueueName);
          return false;
        }
      }
    }

    if ((int)j.Application.ProcessingStartTime.GetTime() != -1) {
      if ((int)t.ComputingEndpoint->DowntimeStarts.GetTime() != -1 && (int)t.ComputingEndpoint->DowntimeEnds.GetTime() != -1) {
        if (t.ComputingEndpoint->DowntimeStarts <= j.Application.ProcessingStartTime && j.Application.ProcessingStartTime <= t.ComputingEndpoint->DowntimeEnds) {
          logger.msg(VERBOSE, "ProcessingStartTime (%s) specified in job description is inside the targets downtime period [ %s - %s ].", (std::string)j.Application.ProcessingStartTime, (std::string)t.ComputingEndpoint->DowntimeStarts, (std::string)t.ComputingEndpoint->DowntimeEnds);
          return false;
        }
      }
      else
        logger.msg(WARNING, "The downtime of the target (%s) is not published. Keeping target.", t.ComputingEndpoint->URLString);
    }

    if (!t.ComputingEndpoint->HealthState.empty()) {

      if (lower(t.ComputingEndpoint->HealthState) != "ok") { // Enumeration for healthstate: ok, critical, other, unknown, warning
        logger.msg(VERBOSE, "HealthState of ExecutionTarget (%s) is not OK (%s)", t.ComputingEndpoint->URLString, t.ComputingEndpoint->HealthState);
        return false;
      }
    }
    else {
      logger.msg(VERBOSE, "Matchmaking, ExecutionTarget:  %s, HealthState is not defined", t.ComputingEndpoint->URLString);
      return false;
    }

    if (!j.Resources.CEType.empty()) {
      if (!t.ComputingEndpoint->Implementation().empty()) {
        if (!j.Resources.CEType.isSatisfied(t.ComputingEndpoint->Implementation)) {
          logger.msg(VERBOSE, "Matchmaking, Computing endpoint requirement not satisfied. ExecutionTarget: %s", (std::string)t.ComputingEndpoint->Implementation);
          return false;
        }
      }
      else {
        logger.msg(VERBOSE, "Matchmaking, ExecutionTarget:  %s, ImplementationName is not defined", t.ComputingEndpoint->URLString);
        return false;
      }
    }

    {
      typedef std::pair<std::string, int> EtTimePair;
      EtTimePair etTime[] = {EtTimePair("MaxWallTime", (int)t.ComputingShare->MaxWallTime.GetPeriod()),
                             EtTimePair("MinWallTime", (int)t.ComputingShare->MinWallTime.GetPeriod()),
                             EtTimePair("MaxCPUTime", (int)t.ComputingShare->MaxCPUTime.GetPeriod()),
                             EtTimePair("MinCPUTime", (int)t.ComputingShare->MinCPUTime.GetPeriod())};

      typedef std::pair<std::string, const ScalableTime<int>*> JobTimePair;
      JobTimePair jobTime[] = {JobTimePair("IndividualWallTime", &j.Resources.IndividualWallTime),
                               JobTimePair("IndividualCPUTime", &j.Resources.IndividualCPUTime)};

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
              for (std::map<std::string, double>::const_iterator itTBench = t.Benchmarks->begin();
                   itTBench != t.Benchmarks->end(); itTBench++) {
                if (lower(jTime->second->benchmark.first) == lower(itTBench->first)) {
                  targetBenchmark = itTBench->second;
                  break;
                }
              }

              // Make it possible to scale according to clock rate.
              if (targetBenchmark <= 0. && lower(jTime->second->benchmark.first) == "clock rate") {
                targetBenchmark = (t.ExecutionEnvironment->CPUClockSpeed > 0. ? (double)t.ExecutionEnvironment->CPUClockSpeed : 1000.);
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

    {
      const int totalcputime = (j.Resources.TotalCPUTime.range.min > 0 ? j.Resources.TotalCPUTime.range.min : j.Resources.TotalCPUTime.range.max);
      if (totalcputime != -1) {
        if (t.ComputingShare->MaxTotalCPUTime.GetPeriod() != -1) {
          if (t.ComputingShare->MaxTotalCPUTime.GetPeriod() < j.Resources.TotalCPUTime.range.min) {
            logger.msg(VERBOSE, "Matchmaking, MaxTotalCPUTime problem, ExecutionTarget: %d (MaxTotalCPUTime), JobDescription: %d (TotalCPUTime)", t.ComputingShare->MaxTotalCPUTime.GetPeriod(), totalcputime);
            return false;
          }
        }
        else if (t.ComputingShare->MaxCPUTime.GetPeriod() != -1) {
          const int slots = (j.Resources.SlotRequirement.NumberOfSlots > 0 ? j.Resources.SlotRequirement.NumberOfSlots : 1);
          if (t.ComputingShare->MaxCPUTime.GetPeriod() < totalcputime/slots) {
            logger.msg(VERBOSE, "Matchmaking, MaxCPUTime problem, ExecutionTarget: %d (MaxCPUTime), JobDescription: %d (TotalCPUTime/NumberOfSlots)", t.ComputingShare->MaxCPUTime.GetPeriod(), totalcputime/slots);
            return false;
          }
        }
        else {
          logger.msg(VERBOSE, "Matchmaking, ExecutionTarget:  %s, MaxTotalCPUTime or MaxCPUTime not defined, assuming no CPU time limit", t.ComputingEndpoint->URLString);
        }
        // There is no MinTotalCPUTime
        if (t.ComputingShare->MinCPUTime.GetPeriod() != -1) {
          const int slots = (j.Resources.SlotRequirement.NumberOfSlots > 0 ? j.Resources.SlotRequirement.NumberOfSlots : 1);
          if (t.ComputingShare->MinCPUTime.GetPeriod() > totalcputime/slots) {
            logger.msg(VERBOSE, "Matchmaking, MinCPUTime problem, ExecutionTarget: %d (MinCPUTime), JobDescription: %d (TotalCPUTime/NumberOfSlots)", t.ComputingShare->MinCPUTime.GetPeriod(), totalcputime/slots);
            return false;
          }
        }
        else {
          logger.msg(VERBOSE, "Matchmaking, ExecutionTarget:  %s, MinCPUTime not defined, assuming no CPU time limit", t.ComputingEndpoint->URLString);
        }
      }
    }

    if (j.Resources.IndividualPhysicalMemory != -1) {
      if (t.ExecutionEnvironment->MainMemorySize != -1) {     // Example: 678
        if (t.ExecutionEnvironment->MainMemorySize < j.Resources.IndividualPhysicalMemory) {
          logger.msg(VERBOSE, "Matchmaking, MainMemorySize problem, ExecutionTarget: %d (MainMemorySize), JobDescription: %d (IndividualPhysicalMemory)", t.ExecutionEnvironment->MainMemorySize, j.Resources.IndividualPhysicalMemory.max);
          return false;
        }
      }
      else if (t.ComputingShare->MaxMainMemory != -1) {     // Example: 678
        if (t.ComputingShare->MaxMainMemory < j.Resources.IndividualPhysicalMemory) {
          logger.msg(VERBOSE, "Matchmaking, MaxMainMemory problem, ExecutionTarget: %d (MaxMainMemory), JobDescription: %d (IndividualPhysicalMemory)", t.ComputingShare->MaxMainMemory, j.Resources.IndividualPhysicalMemory.max);
          return false;
        }
      }
      else {
        logger.msg(VERBOSE, "Matchmaking, ExecutionTarget: %s, MaxMainMemory and MainMemorySize are not defined", t.ComputingEndpoint->URLString);
        return false;
      }
    }

    if (j.Resources.IndividualVirtualMemory != -1) {
      if (t.ComputingShare->MaxVirtualMemory != -1) {     // Example: 678
        if (t.ComputingShare->MaxVirtualMemory < j.Resources.IndividualVirtualMemory) {
          logger.msg(VERBOSE, "Matchmaking, MaxVirtualMemory problem, ExecutionTarget: %d (MaxVirtualMemory), JobDescription: %d (IndividualVirtualMemory)", t.ComputingShare->MaxVirtualMemory, j.Resources.IndividualVirtualMemory.max);
          return false;
        }
      }
      else {
        logger.msg(VERBOSE, "Matchmaking, ExecutionTarget: %s, MaxVirtualMemory is not defined", t.ComputingEndpoint->URLString);
        return false;
      }
    }

    if (!j.Resources.Platform.empty()) {
      if (!t.ExecutionEnvironment->Platform.empty()) {    // Example: i386
        if (t.ExecutionEnvironment->Platform != j.Resources.Platform) {
          logger.msg(VERBOSE, "Matchmaking, Platform problem, ExecutionTarget: %s (Platform) JobDescription: %s (Platform)", t.ExecutionEnvironment->Platform, j.Resources.Platform);
          return false;
        }
      }
      else {
        logger.msg(VERBOSE, "Matchmaking, ExecutionTarget:  %s, Platform is not defined", t.ComputingEndpoint->URLString);
        return false;
      }
    }

    if (!j.Resources.OperatingSystem.empty()) {
      if (!t.ExecutionEnvironment->OperatingSystem.empty()) {
        if (!j.Resources.OperatingSystem.isSatisfied(t.ExecutionEnvironment->OperatingSystem)) {
          logger.msg(VERBOSE, "Matchmaking, ExecutionTarget: %s, OperatingSystem requirements not satisfied", t.ComputingEndpoint->URLString);
          return false;
        }
      }
      else {
        logger.msg(VERBOSE, "Matchmaking, ExecutionTarget:  %s,  OperatingSystem is not defined", t.ComputingEndpoint->URLString);
        return false;
      }
    }

    if (!j.Resources.RunTimeEnvironment.empty()) {
      if (!t.ApplicationEnvironments->empty()) {
        if (!j.Resources.RunTimeEnvironment.isSatisfied(*t.ApplicationEnvironments)) {
          logger.msg(VERBOSE, "Matchmaking, ExecutionTarget:  %s, RunTimeEnvironment requirements not satisfied", t.ComputingEndpoint->URLString);
          return false;
        }
      }
      else {
        logger.msg(VERBOSE, "Matchmaking, ExecutionTarget: %s, ApplicationEnvironments not defined", t.ComputingEndpoint->URLString);
        return false;
      }
    }

    if (!j.Resources.NetworkInfo.empty())
      if (!t.ComputingManager->NetworkInfo.empty()) {    // Example: infiniband
        if (std::find(t.ComputingManager->NetworkInfo.begin(), t.ComputingManager->NetworkInfo.end(),
                      j.Resources.NetworkInfo) == t.ComputingManager->NetworkInfo.end()) {
          logger.msg(VERBOSE, "Matchmaking, NetworkInfo demand not fulfilled, ExecutionTarget do not support %s, specified in the JobDescription.", j.Resources.NetworkInfo);
          return false;
        }
        else {
          logger.msg(VERBOSE, "Matchmaking, ExecutionTarget:  %s, NetworkInfo is not defined", t.ComputingEndpoint->URLString);
          return false;
        }
      }

    if (j.Resources.DiskSpaceRequirement.SessionDiskSpace > -1) {
      if (t.ComputingShare->MaxDiskSpace > -1) {     // Example: 5656
        if (t.ComputingShare->MaxDiskSpace*1024 < j.Resources.DiskSpaceRequirement.SessionDiskSpace) {
          logger.msg(VERBOSE, "Matchmaking, MaxDiskSpace problem, ExecutionTarget: %d MB (MaxDiskSpace); JobDescription: %d MB (SessionDiskSpace)", t.ComputingShare->MaxDiskSpace*1024, j.Resources.DiskSpaceRequirement.SessionDiskSpace);
          return false;
        }
      }

      if (t.ComputingManager->WorkingAreaFree > -1) {     // Example: 5656
        if (t.ComputingManager->WorkingAreaFree*1024 < j.Resources.DiskSpaceRequirement.SessionDiskSpace) {
          logger.msg(VERBOSE, "Matchmaking, WorkingAreaFree problem, ExecutionTarget: %d MB (WorkingAreaFree); JobDescription: %d MB (SessionDiskSpace)", t.ComputingManager->WorkingAreaFree*1024, j.Resources.DiskSpaceRequirement.SessionDiskSpace);
          return false;
        }
      }

      if (t.ComputingShare->MaxDiskSpace <= -1 && t.ComputingManager->WorkingAreaFree <= -1) {
        logger.msg(VERBOSE, "Matchmaking, ExecutionTarget:  %s, MaxDiskSpace and WorkingAreaFree are not defined", t.ComputingEndpoint->URLString);
        return false;
      }
    }

    if (j.Resources.DiskSpaceRequirement.DiskSpace.max > -1) {
      if (t.ComputingShare->MaxDiskSpace > -1) {     // Example: 5656
        if (t.ComputingShare->MaxDiskSpace*1024 < j.Resources.DiskSpaceRequirement.DiskSpace.max) {
          logger.msg(VERBOSE, "Matchmaking, MaxDiskSpace problem, ExecutionTarget: %d MB (MaxDiskSpace); JobDescription: %d MB (DiskSpace)", t.ComputingShare->MaxDiskSpace*1024, j.Resources.DiskSpaceRequirement.DiskSpace.max);
          return false;
        }
      }

      if (t.ComputingManager->WorkingAreaFree > -1) {     // Example: 5656
        if (t.ComputingManager->WorkingAreaFree*1024 < j.Resources.DiskSpaceRequirement.DiskSpace.max) {
          logger.msg(VERBOSE, "Matchmaking, WorkingAreaFree problem, ExecutionTarget: %d MB (WorkingAreaFree); JobDescription: %d MB (DiskSpace)", t.ComputingManager->WorkingAreaFree*1024, j.Resources.DiskSpaceRequirement.DiskSpace.max);
          return false;
        }
      }

      if (t.ComputingManager->WorkingAreaFree <= -1 && t.ComputingShare->MaxDiskSpace <= -1) {
        logger.msg(VERBOSE, "Matchmaking, ExecutionTarget:  %s, MaxDiskSpace and WorkingAreaFree are not defined", t.ComputingEndpoint->URLString);
        return false;
      }
    }

    if (j.Resources.DiskSpaceRequirement.CacheDiskSpace > -1) {
      if (t.ComputingManager->CacheTotal > -1) {     // Example: 5656
        if (t.ComputingManager->CacheTotal*1024 < j.Resources.DiskSpaceRequirement.CacheDiskSpace) {
          logger.msg(VERBOSE, "Matchmaking, CacheTotal problem, ExecutionTarget: %d MB (CacheTotal); JobDescription: %d MB (CacheDiskSpace)", t.ComputingManager->CacheTotal*1024, j.Resources.DiskSpaceRequirement.CacheDiskSpace);
          return false;
        }
      }
      else {
        logger.msg(VERBOSE, "Matchmaking, ExecutionTarget:  %s, CacheTotal is not defined", t.ComputingEndpoint->URLString);
        return false;
      }
    }

    if (j.Resources.SlotRequirement.NumberOfSlots != -1) {
      if (t.ComputingManager->TotalSlots != -1) {     // Example: 5656
        if (t.ComputingManager->TotalSlots < j.Resources.SlotRequirement.NumberOfSlots) {
          logger.msg(VERBOSE, "Matchmaking, TotalSlots problem, ExecutionTarget: %d (TotalSlots) JobDescription: %d (NumberOfProcesses)", t.ComputingManager->TotalSlots, j.Resources.SlotRequirement.NumberOfSlots);
          return false;
        }
      }
      if (t.ComputingShare->MaxSlotsPerJob != -1) {     // Example: 5656
        if (t.ComputingShare->MaxSlotsPerJob < j.Resources.SlotRequirement.NumberOfSlots) {
          logger.msg(VERBOSE, "Matchmaking, MaxSlotsPerJob problem, ExecutionTarget: %d (MaxSlotsPerJob) JobDescription: %d (NumberOfProcesses)", t.ComputingShare->MaxSlotsPerJob, j.Resources.SlotRequirement.NumberOfSlots);
          return false;
        }
      }

      if (t.ComputingManager->TotalSlots == -1 && t.ComputingShare->MaxSlotsPerJob == -1) {
        logger.msg(VERBOSE, "Matchmaking, ExecutionTarget:  %s, TotalSlots and MaxSlotsPerJob are not defined", t.ComputingEndpoint->URLString);
        return false;
      }
    }

    if ((int)j.Resources.SessionLifeTime.GetPeriod() != -1) {
      if ((int)t.ComputingManager->WorkingAreaLifeTime.GetPeriod() != -1) {     // Example: 123
        if (t.ComputingManager->WorkingAreaLifeTime < j.Resources.SessionLifeTime) {
          logger.msg(VERBOSE, "Matchmaking, WorkingAreaLifeTime problem, ExecutionTarget: %s (WorkingAreaLifeTime) JobDescription: %s (SessionLifeTime)", (std::string)t.ComputingManager->WorkingAreaLifeTime, (std::string)j.Resources.SessionLifeTime);
          return false;
        }
      }
      else {
        logger.msg(VERBOSE, "Matchmaking, ExecutionTarget:  %s, WorkingAreaLifeTime is not defined", t.ComputingEndpoint->URLString);
        return false;
      }
    }

    if ((j.Resources.NodeAccess == NAT_INBOUND ||
         j.Resources.NodeAccess == NAT_INOUTBOUND) &&
        !t.ExecutionEnvironment->ConnectivityIn) {     // Example: false (boolean)
      logger.msg(VERBOSE, "Matchmaking, ConnectivityIn problem, ExecutionTarget: %s (ConnectivityIn) JobDescription: %s (InBound)", (j.Resources.NodeAccess == NAT_INBOUND ? "INBOUND" : "INOUTBOUND"), (t.ExecutionEnvironment->ConnectivityIn ? "true" : "false"));
      return false;
    }

    if ((j.Resources.NodeAccess == NAT_OUTBOUND ||
         j.Resources.NodeAccess == NAT_INOUTBOUND) &&
        !t.ExecutionEnvironment->ConnectivityOut) {     // Example: false (boolean)
      logger.msg(VERBOSE, "Matchmaking, ConnectivityOut problem, ExecutionTarget: %s (ConnectivityOut) JobDescription: %s (OutBound)", (j.Resources.NodeAccess == NAT_OUTBOUND ? "OUTBOUND" : "INOUTBOUND"), (t.ExecutionEnvironment->ConnectivityIn ? "true" : "false"));
      return false;
    }

    return true;
  }

  void ExecutionTargetSorter::addEntities(const std::list<ComputingServiceType>& csList) {
    for (std::list<ComputingServiceType>::const_iterator it = csList.begin(); it != csList.end(); ++it) {
      addEntity(*it);
    }
  }

  void ExecutionTargetSorter::addEntity(const ComputingServiceType& cs) {
    /* Get ExecutionTarget objects with
     * ComputingServiceType::GetExecutionTargets method, but first save iterator
     * to element before end(). Check if the new ExecutionTarget objects matches
     * and if so insert them in the correct location.
     */
    std::list<ExecutionTarget>::iterator it = --targets.second.end();
    cs.GetExecutionTargets(targets.second); // Adds ExecutionTarget objects to end of targets.second list.

    if (b == NULL || !b->isValid()) {
      logger.msg(DEBUG, "Unable to sort added jobs. The BrokerPlugin plugin has not been loaded.");
      return;
    }

    for (++it; it != targets.second.end();) {
      if (!reject(*it) && b->match(*it)) {
        insert(*it);
        it = targets.second.erase(it);
      }
      else {
        ++it;
      }
    }
  }

  void ExecutionTargetSorter::addEntity(const ExecutionTarget& et) {
    if (b == NULL || !b->isValid()) {
      logger.msg(DEBUG, "Unable to match target, marking it as not matching. Broker not valid.");
      targets.second.push_back(et);
      return;
    }

    if (!reject(et) && b->match(et)) {
      insert(et);
    }
    else {
      targets.second.push_back(et);
    }
  }

  void ExecutionTargetSorter::insert(const ExecutionTarget& et) {
    std::list<ExecutionTarget>::iterator insertPosition = targets.first.begin();
    for (; insertPosition != targets.first.end(); ++insertPosition) {
      if ((*b)(et, *insertPosition)) {
        break;
      }
    }
    targets.first.insert(insertPosition, et);
  }
  
  bool ExecutionTargetSorter::reject(const ExecutionTarget& et) {
    for (std::list<URL>::const_iterator it = rejectEndpoints.begin(); it != rejectEndpoints.end(); ++it) {
      if (it->StringMatches(et.ComputingEndpoint->URLString)) return true;
    }
    return false;
  }

  void ExecutionTargetSorter::sort() {
    targets.second.insert(targets.second.end(), targets.first.begin(), targets.first.end());
    targets.first.clear();

    if (b == NULL || !b->isValid()) {
      reset();
      logger.msg(DEBUG, "Unable to sort ExecutionTarget objects - Invalid Broker object.");
      return;
    }
    
    for (std::list<ExecutionTarget>::iterator it = targets.second.begin();
         it != targets.second.end();) {
      if (!reject(*it) && b->match(*it)) {
        insert(*it);
        it = targets.second.erase(it);
      }
      else {
        ++it;
      }
    }

    reset();
  }

  void ExecutionTargetSorter::registerJobSubmission() {
    if (endOfList()) {
      return;
    }

    if (b == NULL || !b->isValid()) {
      logger.msg(DEBUG, "Unable to register job submission. Can't get JobDescription object from Broker, Broker is invalid.");
      return;
    }

    current->RegisterJobSubmission(b->getJobDescription());

    targets.second.push_back(*current);
    targets.first.erase(current);

    if (b->match(targets.second.back())) {
      insert(targets.second.back());
      targets.second.pop_back();
    }

    reset();
  }
} // namespace Arc
