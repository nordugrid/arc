// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_RANDOMBROKER_H__
#define __ARC_RANDOMBROKER_H__

#include <arc/client/Broker.h>

namespace Arc {

  class RandomBroker
    : public Broker {
  public:
    RandomBroker(Config *cfg);
    ~RandomBroker();
    static Plugin* Instance(PluginArgument *arg);

  protected:
    virtual void SortTargets();
  };

} // namespace Arc

#endif // __ARC_RANDOMBROKER_H__
