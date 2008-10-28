#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <algorithm>

#include <arc/client/QueueBalanceBroker.h>

namespace Arc {
    
  bool CompareExecutionTarget(const ExecutionTarget& T1, const ExecutionTarget& T2){
    return T1.WaitingJobs < T2.WaitingJobs;
  }

  QueueBalanceBroker::QueueBalanceBroker(Config *cfg) : Broker(cfg){}

  QueueBalanceBroker::~QueueBalanceBroker(){}

  ACC* QueueBalanceBroker::Instance(Config *cfg, ChainContext*) {
    return new QueueBalanceBroker(cfg);
  }

  void QueueBalanceBroker::SortTargets() {
  	std::sort( PossibleTargets.begin(), PossibleTargets.end(), CompareExecutionTarget);

  }
} // namespace Arc


