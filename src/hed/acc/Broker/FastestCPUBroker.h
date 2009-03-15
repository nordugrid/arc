// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_FASTESTCPUBROKER_H__
#define __ARC_FASTESTCPUBROKER_H__

#include <arc/client/Broker.h>

namespace Arc {

  class FastestCPUBroker
    : public Broker {

  public:
    FastestCPUBroker(Config *cfg);
    ~FastestCPUBroker();
    static Plugin* Instance(PluginArgument *arg);

  protected:
    void SortTargets();
  };

} // namespace Arc

#endif // __ARC_FASTESTCPUBROKER_H__
