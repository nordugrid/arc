#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cstdlib>
#include <algorithm>

#include "RandomBroker.h"
	
namespace Arc {

  RandomBroker::RandomBroker(Config *cfg) : Broker(cfg){}

  RandomBroker::~RandomBroker(){}
  
  Plugin* RandomBroker::Instance(PluginArgument* arg) {
    ACCPluginArgument* accarg =
            arg?dynamic_cast<ACCPluginArgument*>(arg):NULL;
    if(!accarg) return NULL;
    return new RandomBroker((Arc::Config*)(*accarg));
  }

  void RandomBroker::SortTargets() {
    
    int i, j;    
    std::srand(time(NULL));
    
    for (unsigned int k = 1; k < 2 * (std::rand() % PossibleTargets.size()) + 1; k++){
      i = rand() % PossibleTargets.size();
      j = rand() % PossibleTargets.size();
      std::iter_swap(PossibleTargets.begin() + i, PossibleTargets.begin() + j);
    }      
    TargetSortingDone = true;
  }

} // namespace Arc


