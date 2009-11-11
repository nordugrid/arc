// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cstdlib>
#include <algorithm>

#include <arc/client/ExecutionTarget.h>

#include "RandomBroker.h"

namespace Arc {

  RandomBroker::RandomBroker(const UserConfig& usercfg)
    : Broker(usercfg) {}

  RandomBroker::~RandomBroker() {}

  Plugin* RandomBroker::Instance(PluginArgument *arg) {
    BrokerPluginArgument *brokerarg = dynamic_cast<BrokerPluginArgument*>(arg);
    if (!brokerarg)
      return NULL;
    return new RandomBroker(*brokerarg);
  }

  void RandomBroker::SortTargets() {

    std::list<ExecutionTarget*>::iterator iter = PossibleTargets.begin();

    logger.msg(VERBOSE, "Matching against job description, following targets possible for RandomBroker: %d", PossibleTargets.size());

    for (int i = 1; iter != PossibleTargets.end(); iter++, i++)
      logger.msg(VERBOSE, "%d. Cluster: %s", i, (*iter)->DomainName);

    int i, j;
    std::srand(time(NULL));

    for (unsigned int k = 1; k < 2 * (std::rand() % PossibleTargets.size()) + 1; k++) {
      std::list<ExecutionTarget*>::iterator itI = PossibleTargets.begin();
      std::list<ExecutionTarget*>::iterator itJ = PossibleTargets.begin();
      for (int i = rand() % PossibleTargets.size(); i > 0; i--)
        itI++;
      for (int i = rand() % PossibleTargets.size(); i > 0; i--)
        itJ++;
      std::iter_swap(itI, itJ);
    }

    logger.msg(VERBOSE, "Best targets are: %d", PossibleTargets.size());

    iter = PossibleTargets.begin();

    for (int i = 1; iter != PossibleTargets.end(); iter++, i++)
      logger.msg(VERBOSE, "%d. Cluster: %s", i, (*iter)->DomainName);
    TargetSortingDone = true;
  }

} // namespace Arc
