#ifndef __ARC_TARGETRETRIEVERARC0_H__
#define __ARC_TARGETRETRIEVERARC0_H__

#include <arc/client/TargetRetriever.h>

namespace Arc {

  class ChainContext;
  class Logger;
  class TargetGenerator;
  class URL;
  class XMLNode;

  struct ThreadArg;

  class TargetRetrieverARC0
    : public TargetRetriever {
  private:
    TargetRetrieverARC0(Config *cfg);
  public:
    ~TargetRetrieverARC0();
    void GetTargets(TargetGenerator& mom, int targetType, int detailLevel);
    static ACC* Instance(Config *cfg, ChainContext *ctx);

  private:
    static void QueryIndex(void *arg);
    static void InterrogateTarget(void *arg);

    ThreadArg* CreateThreadArg(TargetGenerator& mom,
			       int targetType, int detailLevel);

    static Logger logger;
  };

} // namespace Arc

#endif // __ARC_TARGETRETRIEVERARC0_H__
