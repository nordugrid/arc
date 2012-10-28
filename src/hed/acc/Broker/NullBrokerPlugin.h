// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_NULLBROKERPLUGIN_H__
#define __ARC_NULLBROKERPLUGIN_H__

#include <cstdlib>

#include <arc/client/Broker.h>
#include <arc/client/ExecutionTarget.h>

namespace Arc {

  class NullBrokerPlugin : public BrokerPlugin {
  public:
    NullBrokerPlugin(BrokerPluginArgument* parg) : BrokerPlugin(parg) {}
    ~NullBrokerPlugin() {}
    static Plugin* Instance(PluginArgument *arg) {
      BrokerPluginArgument *brokerarg = dynamic_cast<BrokerPluginArgument*>(arg);
      return brokerarg ? new NullBrokerPlugin(brokerarg) : NULL;
    }

    virtual bool match(ExecutionTarget& et) const { return true; }
    
    virtual bool operator()(const ExecutionTarget&, const ExecutionTarget&) const { return true; }
  };

} // namespace Arc

#endif // __ARC_NULLBROKERPLUGIN_H__
