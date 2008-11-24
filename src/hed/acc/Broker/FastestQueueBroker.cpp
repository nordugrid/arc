#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <algorithm>

#include "FastestQueueBroker.h"

namespace Arc {
    
  bool CompareExecutionTarget(const ExecutionTarget& T1, const ExecutionTarget& T2){
    return T1.WaitingJobs < T2.WaitingJobs;
  }

  FastestQueueBroker::FastestQueueBroker(Config *cfg) : Broker(cfg){}

  FastestQueueBroker::~FastestQueueBroker(){}

  ACC* FastestQueueBroker::Instance(Config *cfg, ChainContext*) {
    return new FastestQueueBroker(cfg);
  }

  void FastestQueueBroker::SortTargets() {
  	std::sort( PossibleTargets.begin(), PossibleTargets.end(), CompareExecutionTarget);
	TargetSortingDone = true;
  }
} // namespace Arc


