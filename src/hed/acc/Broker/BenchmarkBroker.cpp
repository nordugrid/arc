// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <algorithm>

#include <arc/UserConfig.h>
#include <arc/StringConv.h>
#include <arc/client/ExecutionTarget.h>

#include "BenchmarkBroker.h"

namespace Arc {

  class cmp {
  public:
    cmp(const std::string benchmark)
      : benchmark(benchmark) {}
    bool ComparePerformance(const ExecutionTarget *T1,
                            const ExecutionTarget *T2);
  private:
    std::string benchmark;
  };

  bool cmp::ComparePerformance(const ExecutionTarget *T1,
                               const ExecutionTarget *T2) {
    double T1performance = 0;
    double T2performance = 0;
    std::map<std::string, double>::const_iterator iter;

    for (iter = T1->Benchmarks.begin(); iter != T1->Benchmarks.end(); iter++)
      if (lower(iter->first) == benchmark) {
        T1performance = iter->second;
        break;
      }

    for (iter = T2->Benchmarks.begin(); iter != T2->Benchmarks.end(); iter++)
      if (lower(iter->first) == benchmark) {
        T1performance = iter->second;
        break;
      }

    return T1performance > T2performance;
  }

  BenchmarkBroker::BenchmarkBroker(const UserConfig& usercfg)
    : Broker(usercfg) {
    benchmark = usercfg.Broker().second;
    if (benchmark.empty())
      benchmark = "specint2000";
  }

  BenchmarkBroker::~BenchmarkBroker() {}

  Plugin* BenchmarkBroker::Instance(PluginArgument *arg) {
    BrokerPluginArgument *brokerarg = dynamic_cast<BrokerPluginArgument*>(arg);
    if (!brokerarg)
      return NULL;
    return new BenchmarkBroker(*brokerarg);
  }

  void BenchmarkBroker::SortTargets() {

    //Remove clusters with incomplete information for target sorting
    std::list<const ExecutionTarget*>::iterator iter = PossibleTargets.begin();
    while (iter != PossibleTargets.end()) {
      if (((*iter)->Benchmarks).empty()) {
        iter = PossibleTargets.erase(iter);
        continue;
      }
      else {
        std::map<std::string, double>::const_iterator iter2;
        bool ok = false;
        for (iter2 = (*iter)->Benchmarks.begin();
             iter2 != (*iter)->Benchmarks.end(); iter2++)
          if (lower(iter2->first) == benchmark) {
            ok = true;
            break;
          }
        if (!ok) {
          iter = PossibleTargets.erase(iter);
          continue;
        }
      }
      iter++;
    }

    logger.msg(VERBOSE, "Matching against job description,"
               "following targets possible for BenchmarkBroker: %d", PossibleTargets.size());

    iter = PossibleTargets.begin();

    for (int i = 1; iter != PossibleTargets.end(); iter++, i++)
      logger.msg(VERBOSE, "%d. Resource: %s; Queue: %s", i, (*iter)->DomainName, (*iter)->ComputingShareName);

    cmp Cmp(benchmark);

    logger.msg(VERBOSE, "Resource will be ranked according to the %s benchmark scenario", benchmark);

    PossibleTargets.sort(sigc::mem_fun(Cmp, &cmp::ComparePerformance));

    logger.msg(VERBOSE, "Best targets are: %d", PossibleTargets.size());

    iter = PossibleTargets.begin();

    for (int i = 1; iter != PossibleTargets.end(); iter++, i++)
      logger.msg(VERBOSE, "%d. Resource: %s; Queue: %s", i, (*iter)->DomainName, (*iter)->ComputingShareName);

    TargetSortingDone = true;

  }

} // namespace Arc
