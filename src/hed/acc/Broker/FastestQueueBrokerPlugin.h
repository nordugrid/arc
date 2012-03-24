// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_FASTESTQUEUEBROKERPLUGIN_H__
#define __ARC_FASTESTQUEUEBROKERPLUGIN_H__

#include <arc/client/Broker.h>

namespace Arc {

  class FastestQueueBrokerPlugin : public BrokerPlugin {
  public:
    FastestQueueBrokerPlugin(BrokerPluginArgument* parg) : BrokerPlugin(parg) {}
    ~FastestQueueBrokerPlugin() {}
    static Plugin* Instance(PluginArgument *arg) {
      BrokerPluginArgument *brokerarg = dynamic_cast<BrokerPluginArgument*>(arg);
      return brokerarg ? new FastestQueueBrokerPlugin(brokerarg) : NULL;
    }
    
    virtual bool operator()(const ExecutionTarget&, const ExecutionTarget&) const;
    virtual bool match(const ExecutionTarget&) const;

  };

} // namespace Arc

#endif // __ARC_FASTESTQUEUEBROKERPLUGIN_H__
