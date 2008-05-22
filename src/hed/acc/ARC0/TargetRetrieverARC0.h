#ifndef __ARC_TARGETRETRIEVERARC0_H__
#define __ARC_TARGETRETRIEVERARC0_H__

#include <arc/client/TargetRetriever.h>

namespace Arc {

  class ChainContext;
  class Logger;
  class TargetGenerator;
  class URL;
  class XMLNode;

  class TargetRetrieverARC0
    : public TargetRetriever {
  private:
    TargetRetrieverARC0(Config *cfg);
    ~TargetRetrieverARC0();

  public:
    void GetTargets(TargetGenerator& mom, int targetType, int detailLevel);
    void InterrogateTarget(TargetGenerator& mom, URL& url,
			   int targetType, int detailLevel);
    static ACC *Instance(Config *cfg, ChainContext *ctx);

  private:
    static Logger logger;
  };

} // namespace Arc

#endif // __ARC_TARGETRETRIEVERARC0_H__
