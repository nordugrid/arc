// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_FASTESTQUEUEBROKER_H__
#define __ARC_FASTESTQUEUEBROKER_H__

#include <arc/client/Broker.h>

namespace Arc {

  class FastestQueueBroker
    : public Broker {

  public:
    FastestQueueBroker(Config *cfg);
    ~FastestQueueBroker();
    static Plugin* Instance(PluginArgument *arg);

  protected:
    void SortTargets();
  };

} // namespace Arc

#endif // __ARC_FASTESTQUEUEBROKER_H__
