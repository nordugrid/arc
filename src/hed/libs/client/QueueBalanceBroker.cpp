#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <algorithm>

#include <arc/client/QueueBalanceBroker.h>

namespace Arc {
    
  bool CompareExecutionTarget(const ExecutionTarget& T1, const ExecutionTarget& T2){
    return T1.WaitingJobs < T2.WaitingJobs;
  }

  void QueueBalanceBroker::sort_Targets() {
  	std::sort( found_Targets.begin(), found_Targets.end(), CompareExecutionTarget);

  }
} // namespace Arc


