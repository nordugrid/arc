#ifndef __ARC_QUEUEBALANCEBROKER_H__
#define __ARC_QUEUEBALANCEBROKER_H__
#include <arc/client/Broker.h>

namespace Arc {

  class QueueBalanceBroker: public Broker {
        public:
                      QueueBalanceBroker(Arc::TargetGenerator& targen,  Arc::JobDescription jobd): Broker( targen, jobd ) {}
                      virtual ~QueueBalanceBroker();
        protected:
	    void sort_Targets();
  };

} // namespace Arc

#endif // __ARC_QUEUEBALANCEBROKER_H__
