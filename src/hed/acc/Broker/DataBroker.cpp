#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <algorithm>

#include "DataBroker.h"

namespace Arc {
   
  bool CacheCheck(std::string& filename, ExecutionTarget& target) {
     // TODO finish this method

    return true;
  }
 
  bool CheckCacheAndCompareExecutionTarget(const ExecutionTarget& T1, const ExecutionTarget& T2){
  //TODO finish this method
    return true;;
  }

  DataBroker::DataBroker(Config *cfg) : Broker(cfg){}

  DataBroker::~DataBroker(){}

  Plugin* DataBroker::Instance(PluginArgument* arg) {
    ACCPluginArgument* accarg =
            arg?dynamic_cast<ACCPluginArgument*>(arg):NULL;
    if(!accarg) return NULL;
    return new DataBroker((Arc::Config*)(*accarg));
  }

  void DataBroker::SortTargets() {
  	std::sort( PossibleTargets.begin(), PossibleTargets.end(), CheckCacheAndCompareExecutionTarget);
	TargetSortingDone = true;
  }
} // namespace Arc


