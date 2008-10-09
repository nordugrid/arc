#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/client/SmartBroker.h>

namespace Arc {
    
    QueueBalanceBroker::~QueueBalanceBroker() {}
    
      // A template for the list sorting function //
     template<class T> class CompareExecutionTarget {
         public:
             CompareExecutionTarget(){};
             virtual ~CompareExecutionTarget(){};
             bool operator()(const T& T1, const T& T2) const {
		 //TODO: here come the compare
                 return T1.MaxRunningJobs > T2.MaxRunningJobs;
             }
     }; // End of template Compare
     
  void QueueBalanceBroker::sort_Targets() {
          std::sort( found_Targets.begin(), found_Targets.end(), CompareExecutionTarget <Arc::ExecutionTarget>() );
  }
} // namespace Arc
