#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cstdlib>
#include <algorithm>
#include "FastestQueueBroker.h"

namespace Arc {
    

  Arc::Logger logger(Arc::Logger::getRootLogger(), "broker");
  Arc::LogStream logcerr(std::cerr);

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

    //Sort the targets according to the number of waiting jobs (in % of the cluster size)
    std::sort( PossibleTargets.begin(), PossibleTargets.end(), CompareExecutionTarget);
    
    //Check is several clusters(queues) have 0 waiting jobs
    int ZeroQueueCluster = 0;
    int TotalFreeCPUs = 0;
    for(iter = PossibleTargets.begin(); iter != PossibleTargets.end(); iter++){
      if(iter->WaitingJobs == 0){
	ZeroQueueCluster++;
	TotalFreeCPUs += iter->FreeSlots;
      }
    }
    
    //If several clusters(queues) have free slots (CPUs) do basic load balancing
    if(ZeroQueueCluster > 1){
      for(int n = 0; n < ZeroQueueCluster-1; n++){
	double RandomCPU = rand()*TotalFreeCPUs;
	for(int j = n; j < ZeroQueueCluster; j++){
	  if(PossibleTargets[j].FreeSlots > RandomCPU){
	    ExecutionTarget temp = PossibleTargets[n];
	    PossibleTargets[n] = PossibleTargets[j];
	    PossibleTargets[j] = temp;	    
	    TotalFreeCPUs -= PossibleTargets[n].FreeSlots;
	    break;
	  } else{
	    RandomCPU -= PossibleTargets[j].FreeSlots;
	  }
	}
      }
    }
 
      TargetSortingDone = true;

  }

} // namespace Arc


