// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_TARGETRETRIEVERARC0_H__
#define __ARC_TARGETRETRIEVERARC0_H__

#include <arc/client/TargetRetriever.h>

namespace Arc {

  class Logger;

  struct ThreadArg;

  class TargetRetrieverARC0
    : public TargetRetriever {
  private:
    TargetRetrieverARC0(const UserConfig& usercfg,
                        const URL& url, ServiceType st);
  public:
    ~TargetRetrieverARC0();
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

#endif // __ARC_TARGETRETRIEVERARC0_H__
