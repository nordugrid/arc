#ifndef __ARC_TARGETRETRIEVERARC1_H__
#define __ARC_TARGETRETRIEVERARC1_H__

#include <arc/client/TargetRetriever.h>

namespace Arc {

  class Logger;
  class TargetGenerator;
  class URL;
  class XMLNode;

  struct ThreadArg;

  class TargetRetrieverARC1
    : public TargetRetriever {
  private:
    TargetRetrieverARC1(Config *cfg);
  public:
    ~TargetRetrieverARC1();
    void GetTargets(TargetGenerator& mom, int targetType, int detailLevel);
    static Plugin *Instance(PluginArgument* arg);

  private:
    static void QueryIndex(void *arg);
    static void InterrogateTarget(void *arg);

    ThreadArg* CreateThreadArg(TargetGenerator& mom,
			       int targetType, int detailLevel);

    static Logger logger;
  };

} // namespace Arc

#endif // __ARC_TARGETRETRIEVERARC1_H__
