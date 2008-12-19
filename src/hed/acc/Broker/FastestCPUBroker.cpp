#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <algorithm>

#include "FastestCPUBroker.h"

namespace Arc {
    
  bool CheckCPUSpeeds(const ExecutionTarget& T1, const ExecutionTarget& T2){
    return T1.CPUClockSpeed > T2.CPUClockSpeed;
  }

  FastestCPUBroker::FastestCPUBroker(Config *cfg) : Broker(cfg){}

  FastestCPUBroker::~FastestCPUBroker(){}

  Plugin* FastestCPUBroker::Instance(PluginArgument* arg) {
    ACCPluginArgument* accarg =
            arg?dynamic_cast<ACCPluginArgument*>(arg):NULL;
    if(!accarg) return NULL;
    return new FastestCPUBroker((Arc::Config*)(*accarg));
  }

  void FastestCPUBroker::SortTargets() {
  	std::sort(PossibleTargets.begin(), PossibleTargets.end(), CheckCPUSpeeds);
	TargetSortingDone = true;
  }
} // namespace Arc


