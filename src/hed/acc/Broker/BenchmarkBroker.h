// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_BENCHMARKBROKER_H__
#define __ARC_BENCHMARKBROKER_H__

#include <arc/client/Broker.h>

namespace Arc {

  class BenchmarkBroker
    : public Broker {

  public:
    BenchmarkBroker(const UserConfig& usercfg);
    ~BenchmarkBroker();
    static Plugin* Instance(PluginArgument *arg);

  protected:
    void SortTargets();
    std::string benchmark;
  };

} // namespace Arc

#endif // __ARC_FASTESTCPUBROKER_H__
