#ifndef __ARC_TARGETRETRIEVERARC1_H__
#define __ARC_TARGETRETRIEVERARC1_H__

#include <arc/client/TargetGenerator.h>
#include <arc/client/TargetRetriever.h>

namespace Arc {

  class ChainContext;
  class XMLNode;
  class URL;

  class TargetRetrieverARC1
    : public TargetRetriever {
    TargetRetrieverARC1(Config *cfg);
    ~TargetRetrieverARC1();

  public:
    void GetTargets(TargetGenerator& Mom, int TargetType, int DetailLevel);
    void InterrogateTarget(TargetGenerator& Mom, std::string url,
			   int TargetType, int DetailLevel);
    static ACC *Instance(Config *cfg, ChainContext *);
    std::list<std::string> getAttribute(std::string attr, XMLNode& node);

  }; //end class

} // namespace Arc

#endif // __ARC_TARGETRETRIEVERARC1_H__
