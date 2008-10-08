#ifndef __ARC_SMARTBROKER_H__
#define __ARC_SMARTBROKER_H__
#include "Broker.h"

namespace Arc {

  class SmartBroker: public Broker {
        protected:
                      SmartBroker(Arc::TargetGenerator& targen);
                      virtual ~SmartBroker();
                      void sort_Targets() {};
  };

} // namespace Arc

#endif // __ARC_SMARTBROKER_H__
