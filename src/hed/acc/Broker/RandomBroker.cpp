#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cstdlib>
#include <algorithm>

#include "RandomBroker.h"
	
namespace Arc {

  RandomBroker::RandomBroker(Config *cfg) : Broker(cfg){}

  RandomBroker::~RandomBroker(){}
  
  ACC* RandomBroker::Instance(Config *cfg, ChainContext*) {
    return new RandomBroker(cfg);
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


