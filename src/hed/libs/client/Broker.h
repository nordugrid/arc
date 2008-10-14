#ifndef __ARC_BROKER_H__
#define __ARC_BROKER_H__

#include <vector>

#include <arc/Logger.h>
#include <arc/client/TargetGenerator.h>
#include <arc/client/JobDescription.h>
#include <arc/client/ExecutionTarget.h>

namespace Arc {

  class Broker {

       public:
			ExecutionTarget& get_Target();

       protected:
			Broker(Arc::TargetGenerator& targen, Arc::JobDescription jd);
			virtual ~Broker();
			virtual void sort_Targets() = 0;
			std::vector<Arc::ExecutionTarget> found_Targets;

       private:	
			//current Target for a Job 
			std::vector<Arc::ExecutionTarget>::iterator current;

  };
  
} // namespace Arc

#endif // __ARC_BROKER_H__



