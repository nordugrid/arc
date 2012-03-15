#ifndef __ARC_BROKERTESTACC_H__
#define __ARC_BROKERTESTACC_H__

#include <string>
#include <list>

#include <arc/UserConfig.h>
#include <arc/client/Broker.h>
#include <arc/client/TestACCControl.h>

namespace Arc {

class BrokerTestACC
  : public Broker {
private:
  BrokerTestACC(const UserConfig& usercfg, PluginArgument* parg) : Broker(usercfg, parg) {
    BrokerTestACCControl::PossibleTargets = &PossibleTargets;
    BrokerTestACCControl::TargetSortingDone = &TargetSortingDone;
  }

public:
  ~BrokerTestACC() {}

  virtual void SortTargets() { TargetSortingDone = true; }

  static Plugin* GetInstance(PluginArgument *arg);
};

}
#endif // __ARC_BROKERTESTACC_H__
