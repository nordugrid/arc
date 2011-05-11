#ifndef __ARC_BROKERTESTACC_H__
#define __ARC_BROKERTESTACC_H__

#include <string>
#include <list>

#include <arc/Logger.h>
#include <arc/UserConfig.h>
#include <arc/client/Broker.h>

#include "TestACCControl.h"

class BrokerTestACC
  : public Arc::Broker {
private:
  BrokerTestACC(const Arc::UserConfig& usercfg)
    : Arc::Broker(usercfg) {
    BrokerTestACCControl::PossibleTargets = &PossibleTargets;
    BrokerTestACCControl::TargetSortingDone = &TargetSortingDone;
  }

public:
  ~BrokerTestACC() {}

  virtual void SortTargets() { if (BrokerTestACCControl::TargetSortingDone) TargetSortingDone = *BrokerTestACCControl::TargetSortingDone; }

  static Arc::Plugin* GetInstance(Arc::PluginArgument *arg);

private:

  static Arc::Logger logger;
};

#endif // __ARC_BROKERTESTACC_H__
