// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_TARGETRETRIEVERUNICORE_H__
#define __ARC_TARGETRETRIEVERUNICORE_H__

#include <arc/client/TargetRetriever.h>

namespace Arc {

  class Logger;

  struct ThreadArg;

  class TargetRetrieverUNICORE
    : public TargetRetriever {
  private:
    TargetRetrieverUNICORE(const UserConfig& usercfg,
                           const URL& url, ServiceType st);
  public:
    ~TargetRetrieverUNICORE();
    void GetTargets(TargetGenerator& mom, int targetType, int detailLevel);
    static Plugin* Instance(PluginArgument *arg);

  private:
    static void QueryIndex(void *arg);
    static void InterrogateTarget(void *arg);

    ThreadArg* CreateThreadArg(TargetGenerator& mom,
                               int targetType, int detailLevel);

    static Logger logger;
  };

} // namespace Arc

#endif // __ARC_TARGETRETRIEVERUNICORE_H__
