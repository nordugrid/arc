#ifndef __ARC_TARGETRETRIEVERCREAM_H__
#define __ARC_TARGETRETRIEVERCREAM_H__

#include <arc/client/TargetRetriever.h>

namespace Arc {

  class ChainContext;
  class Logger;
  class TargetGenerator;
  class URL;
  class XMLNode;

  struct ThreadArg;

  class TargetRetrieverCREAM
    : public TargetRetriever {
  private:
    TargetRetrieverCREAM(Config *cfg);
  public:
    ~TargetRetrieverCREAM();
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

#endif // __ARC_TARGETRETRIEVERCREAM_H__
