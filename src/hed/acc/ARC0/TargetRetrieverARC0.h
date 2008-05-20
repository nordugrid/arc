#ifndef __ARC_TARGETRETRIEVERARC0_H__
#define __ARC_TARGETRETRIEVERARC0_H__

#include <arc/client/TargetGenerator.h>
#include <arc/client/TargetRetriever.h>

namespace Arc {

  class ChainContext;
  class XMLNode;
  class URL;

  class TargetRetrieverARC0
    : public TargetRetriever {
    TargetRetrieverARC0(Config *cfg);
    ~TargetRetrieverARC0();

  public:
    void GetTargets(TargetGenerator& Mom, int TargetType, int DetailLevel);
    void InterrogateTarget(TargetGenerator& Mom, std::string url,
			   int TargetType, int DetailLevel);
    static ACC *Instance(Config *cfg, ChainContext *);
    std::list<std::string> getAttribute(std::string attr, XMLNode& node);

  }; //end class

} // namespace Arc

#endif // __ARC_TARGETRETRIEVERARC0_H__
