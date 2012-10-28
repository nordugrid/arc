// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_RANDOMBROKERPLUGIN_H__
#define __ARC_RANDOMBROKERPLUGIN_H__

#include <cstdlib>

#include <arc/client/Broker.h>
#include <arc/client/ExecutionTarget.h>

namespace Arc {

  class RandomBrokerPlugin : public BrokerPlugin {
  public:
    RandomBrokerPlugin(BrokerPluginArgument* parg) : BrokerPlugin(parg) { std::srand(time(NULL)); }
    ~RandomBrokerPlugin() {};
    static Plugin* Instance(PluginArgument *arg) {
      BrokerPluginArgument *brokerarg = dynamic_cast<BrokerPluginArgument*>(arg);
      return brokerarg ? new RandomBrokerPlugin(brokerarg) : NULL;
    }

    virtual bool match(ExecutionTarget& et) const { return BrokerPlugin::match(et); }
    
    virtual bool operator()(const ExecutionTarget&, const ExecutionTarget&) const { return (bool)(std::rand()%2); }
  };

} // namespace Arc

#endif // __ARC_RANDOMBROKERPLUGIN_H__
