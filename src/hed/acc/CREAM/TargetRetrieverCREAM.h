#ifndef __ARC_TARGETRETRIEVERCREAM_H__
#define __ARC_TARGETRETRIEVERCREAM_H__

#include <arc/client/TargetRetriever.h>

namespace Arc {

  class ChainContext;
  class Logger;
  class TargetGenerator;
  class URL;
  class XMLNode;

  class TargetRetrieverCREAM
    : public TargetRetriever {
  private:
    TargetRetrieverCREAM(Config *cfg);
    ~TargetRetrieverCREAM();

  public:
    void GetTargets(TargetGenerator& mom, int targetType, int detailLevel);
    void InterrogateTarget(TargetGenerator& mom, URL& url,
			   int targetType, int detailLevel);
    static ACC *Instance(Config *cfg, ChainContext *ctx);

  private:
    static Logger logger;
  };

} // namespace Arc

#endif // __ARC_TARGETRETRIEVERCREAM_H__
