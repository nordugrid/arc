#include <arc/client/TargetGenerator.h>
#include <arc/client/TargetRetriever.h>

namespace Arc {

  class ChainContext;
  class XMLNode;
  class URL;

  class TargetRetrieverCREAM : public TargetRetriever {
    TargetRetrieverCREAM(Config *cfg);
    ~TargetRetrieverCREAM();

  public:
    void GetTargets(TargetGenerator &mom, int TargetType, int DetailLevel);
    void InterrogateTarget(TargetGenerator &mom, URL url);
    static ACC* Instance(Config *cfg, ChainContext *ctx);
  };

} // namespace Arc
