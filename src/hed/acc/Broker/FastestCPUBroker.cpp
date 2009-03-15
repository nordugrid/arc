// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <algorithm>

#include <arc/StringConv.h>

#include "FastestCPUBroker.h"

namespace Arc {

  bool CheckCPUSpeeds(const ExecutionTarget& T1, const ExecutionTarget& T2) {
    double T1performance = 0;
    double T2performance = 0;
    std::map<std::string, double>::const_iterator iter;

    for (iter = T1.Benchmarks.begin(); iter != T1.Benchmarks.end(); iter++)
      if (lower(iter->first) == "specint2000") {
        T1performance = iter->second;
        break;
      }

    for (iter = T2.Benchmarks.begin(); iter != T2.Benchmarks.end(); iter++)
      if (lower(iter->first) == "specint2000") {
        T1performance = iter->second;
        break;
      }

    return T1performance > T2performance;

  }

  FastestCPUBroker::FastestCPUBroker(Config *cfg)
    : Broker(cfg) {}

  FastestCPUBroker::~FastestCPUBroker() {}

  Plugin* FastestCPUBroker::Instance(PluginArgument *arg) {
    ACCPluginArgument *accarg =
      arg ? dynamic_cast<ACCPluginArgument*>(arg) : NULL;
    if (!accarg)
      return NULL;
    return new FastestCPUBroker((Arc::Config*)(*accarg));
  }

  void FastestCPUBroker::SortTargets() {

    //Remove clusters with incomplete information for target sorting
    std::vector<Arc::ExecutionTarget>::iterator iter = PossibleTargets.begin();
    while (iter != PossibleTargets.end()) {
      if ((iter->Benchmarks).empty()) {
        iter = PossibleTargets.erase(iter);
        continue;
      }
      else {
        std::map<std::string, double>::const_iterator iter2;
        bool ok = false;
        for (iter2 = iter->Benchmarks.begin();
             iter2 != iter->Benchmarks.end(); iter2++)
          if (lower(iter2->first) == "specint2000") {
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

    logger.msg(DEBUG, "Matching against job description, following targets possible for FastestCPUBroker: %d", PossibleTargets.size());

    iter = PossibleTargets.begin();

    for (int i = 1; iter != PossibleTargets.end(); iter++, i++)
      logger.msg(DEBUG, "%d. Cluster: %s", i, iter->DomainName);

    std::sort(PossibleTargets.begin(), PossibleTargets.end(), CheckCPUSpeeds);

    logger.msg(DEBUG, "Best targets are: %d", PossibleTargets.size());

    iter = PossibleTargets.begin();

    for (int i = 1; iter != PossibleTargets.end(); iter++, i++)
      logger.msg(DEBUG, "%d. Cluster: %s", i, iter->DomainName);

    TargetSortingDone = true;

  }
} // namespace Arc
