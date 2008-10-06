#ifndef __ARC_BROKER_H__
#define __ARC_BROKER_H__

#include <list>

#include <arc/client/JobDescription.h>
#include <arc/client/ExecutionTarget.h>

namespace Arc {

  class Broker {
       public:
                 ExecutionTarget get_Target();
       protected:
	 Broker();
	 Broker( Arc::JobDescription jd);
                 virtual ~Broker();
	 virtual void sort_Targets()=0;
       private:
	 std::list<Arc::ExecutionTarget> found_Targets;
	 std::list<Arc::ExecutionTarget>::iterator current;
  };

} // namespace Arc

#endif // __ARC_BROKER_H__
