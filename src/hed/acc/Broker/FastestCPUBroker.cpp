#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <algorithm>
#include "FastestCPUBroker.h"

namespace Arc {
    
  bool CheckCPUSpeeds(const ExecutionTarget& T1, const ExecutionTarget& T2){
    double T1performance = 0;
    double T2performance = 0;
    std::list<Benchmark>::const_iterator iter;

    for(iter = T1.Benchmarks.begin(); iter != T1.Benchmarks.end();iter++){
      if(strcasestr(iter->Type.c_str(), "specint2006")){
	T1performance = iter->Value;
	break;
      }
    }

    
    for(iter = T2.Benchmarks.begin(); iter != T2.Benchmarks.end();iter++){
      if(strcasestr(iter->Type.c_str(), "specint2006")){
	T1performance = iter->Value;
	break;
      }
    }

    return T1performance > T2performance;

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
    
    //Remove clusters with incomplete information for target sorting
    std::vector<Arc::ExecutionTarget>::iterator iter = PossibleTargets.begin();
    while(iter != PossibleTargets.end()){
      if(!(iter->WaitingJobs != -1 && iter->TotalSlots != -1 && iter->FreeSlots != -1)){
	iter = PossibleTargets.erase(iter);
	continue;
      }
      iter++;
    }
    
    std::sort(PossibleTargets.begin(), PossibleTargets.end(), CheckCPUSpeeds);
    TargetSortingDone = true;

  }
} // namespace Arc


