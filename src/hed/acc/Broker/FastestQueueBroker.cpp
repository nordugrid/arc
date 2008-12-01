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

  Plugin* FastestQueueBroker::Instance(PluginArgument* arg) {
    ACCPluginArgument* accarg =
            arg?dynamic_cast<ACCPluginArgument*>(arg):NULL;
    if(!accarg) return NULL;
    return new FastestQueueBroker((Arc::Config*)(*accarg));
  }

  void FastestQueueBroker::SortTargets() {
  	std::sort( PossibleTargets.begin(), PossibleTargets.end(), CompareExecutionTarget);
	TargetSortingDone = true;
  }
} // namespace Arc


