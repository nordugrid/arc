#ifndef __ARC_BROKERPLUGINTESTACC_H__
#define __ARC_BROKERPLUGINTESTACC_H__

#include <string>
#include <list>

#include <arc/UserConfig.h>
#include <arc/client/Broker.h>
#include <arc/client/TestACCControl.h>

namespace Arc {

class BrokerPluginTestACC : public BrokerPlugin {
private:
  BrokerPluginTestACC(BrokerPluginArgument* parg) : BrokerPlugin(parg) {}

public:
  ~BrokerPluginTestACC() {}

  virtual bool operator()(const ExecutionTarget&, const ExecutionTarget&) const { return BrokerPluginTestACCControl::less; }
  virtual bool match(const ExecutionTarget&) const { return BrokerPluginTestACCControl::match; }
  static Plugin* GetInstance(PluginArgument *arg) { 
    BrokerPluginArgument *bparg = dynamic_cast<BrokerPluginArgument*>(arg);
    return bparg ? new BrokerPluginTestACC(bparg) : NULL;
  }

};

}
#endif // __ARC_BROKERPLUGINTESTACC_H__
