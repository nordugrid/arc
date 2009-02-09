#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cstdlib>
#include <algorithm>
#include "FastestQueueBroker.h"

namespace Arc {
    
  bool CompareExecutionTarget(const ExecutionTarget& T1, const ExecutionTarget& T2){

    //Scale queue to become cluster size independent
    float T1queue = (float) T1.WaitingJobs / T1.TotalSlots;
    float T2queue = (float) T2.WaitingJobs / T2.TotalSlots;
    
    return T1queue< T2queue;
    
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

    //Remove clusters with incomplete information for target sorting
    std::vector<Arc::ExecutionTarget>::iterator iter = PossibleTargets.begin();
    while(iter != PossibleTargets.end()){
      if(!(iter->WaitingJobs != -1 && iter->TotalSlots != -1 && iter->FreeSlots != -1)){
	iter = PossibleTargets.erase(iter);
	continue;
      }
      iter++;
    }

    logger.msg(DEBUG, "Matching against job description, following targets possible for FastestQueueBroker: %d",PossibleTargets.size());

    iter = PossibleTargets.begin();
    
    for(int i=1; iter != PossibleTargets.end(); iter++, i++){
      logger.msg(DEBUG, "%d. Cluster: %s", i, iter->DomainName);
    }    

    //Sort the targets according to the number of waiting jobs (in % of the cluster size)
    std::sort( PossibleTargets.begin(), PossibleTargets.end(), CompareExecutionTarget);
    
    //Check is several clusters(queues) have 0 waiting jobs
    int ZeroQueueCluster = 0;
    int TotalFreeCPUs = 0;
    for(iter = PossibleTargets.begin(); iter != PossibleTargets.end(); iter++){
      if(iter->WaitingJobs == 0){
	ZeroQueueCluster++;
	TotalFreeCPUs += iter->FreeSlots / abs(jir.Slots);
      }
    }
    
    //If several clusters(queues) have free slots (CPUs) do basic load balancing
    if(ZeroQueueCluster > 1){
      for(int n = 0; n < ZeroQueueCluster-1; n++){
	double RandomCPU = rand()*TotalFreeCPUs;
	for(int j = n; j < ZeroQueueCluster; j++){
	  if((PossibleTargets[j].FreeSlots / abs(jir.Slots)) > RandomCPU){
	    ExecutionTarget temp = PossibleTargets[n];
	    PossibleTargets[n] = PossibleTargets[j];
	    PossibleTargets[j] = temp;	    
	    TotalFreeCPUs -= (PossibleTargets[n].FreeSlots / abs(jir.Slots));
	    break;
	  } else{
	    RandomCPU -= (PossibleTargets[j].FreeSlots / abs(jir.Slots));
	  }
	}
      }
    }

    logger.msg(DEBUG, "Best targets are: %d",PossibleTargets.size());

    iter = PossibleTargets.begin();
        
    for(int i=1; iter != PossibleTargets.end(); iter++, i++){
      logger.msg(DEBUG, "%d. Cluster: %s", i, iter->DomainName);
    }        
 
    TargetSortingDone = true;

  }

} // namespace Arc


