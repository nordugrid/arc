#ifndef __ARC_RANDOMBROKER_H__
#define __ARC_RANDOMBROKER_H__

#include <arc/client/Broker.h>

namespace Arc {
  
  class RandomBroker: public Broker {
  public:
    RandomBroker(Config *cfg);
    ~RandomBroker();
    static ACC* Instance(Config *cfg, ChainContext *ctx);
    
  protected:
    virtual void SortTargets();
  };

} // namespace Arc

#endif // __ARC_RANDOMBROKER_H__


