#ifndef __ARC_QUEUEBALANCEBROKER_H__
#define __ARC_QUEUEBALANCEBROKER_H__

#include <arc/client/Broker.h>

namespace Arc {
  
  class QueueBalanceBroker: public Broker {
    
  public:
    QueueBalanceBroker(Config *cfg);
    ~QueueBalanceBroker();
    static ACC* Instance(Config *cfg, ChainContext *ctx);
    
  protected:
    void SortTargets();
  };
  
} // namespace Arc

#endif // __ARC_QUEUEBALANCEBROKER_H__
