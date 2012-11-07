// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_BENCHMARKBROKERPLUGIN_H__
#define __ARC_BENCHMARKBROKERPLUGIN_H__

#include <arc/StringConv.h>
#include <arc/UserConfig.h>
#include <arc/compute/Broker.h>

namespace Arc {

  class BenchmarkBrokerPlugin : public BrokerPlugin {
  public:
    BenchmarkBrokerPlugin(BrokerPluginArgument* parg) : BrokerPlugin(parg), benchmark(!uc.Broker().second.empty() ? lower(uc.Broker().second) : "specint2000") {}
    ~BenchmarkBrokerPlugin() {}
    static Plugin* Instance(PluginArgument *arg) {
      BrokerPluginArgument *brokerarg = dynamic_cast<BrokerPluginArgument*>(arg);
      return brokerarg ? new BenchmarkBrokerPlugin(brokerarg) : NULL;
    }
  
    virtual bool operator()(const ExecutionTarget&, const ExecutionTarget&) const;
    virtual bool match(const ExecutionTarget&) const;

  private:
    std::string benchmark;
  };

} // namespace Arc

#endif // __ARC_BENCHMARKBROKERPLUGIN_H__
