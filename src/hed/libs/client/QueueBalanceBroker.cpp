#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <algorithm>

#include <arc/client/QueueBalanceBroker.h>

namespace Arc {
    
  // A template for the list sorting function 
  template<class T> class CompareExecutionTarget {
	public:
    	CompareExecutionTarget(){};
        virtual ~CompareExecutionTarget(){};
        bool operator()(const T& T1, const T& T2) const {
		// Comparing
        	return T1.WaitingJobs < T2.WaitingJobs;
        }
  }; // End of template Compare
     
  void QueueBalanceBroker::sort_Targets() {
  	std::sort( found_Targets.begin(), found_Targets.end(), CompareExecutionTarget <Arc::ExecutionTarget>() );

  }
} // namespace Arc


