#include <arc/client/TargetGenerator.h>
#include <arc/client/TargetRetriever.h>

namespace Arc {

  class ChainContext;
  class XMLNode;
  class URL;

  class TargetRetrieverARC0 : public Arc::TargetRetriever{
    TargetRetrieverARC0(Arc::Config *cfg);
    ~TargetRetrieverARC0();

  public:
    void GetTargets(Arc::TargetGenerator &Mom, int TargetType, int DetailLevel);
    void InterrogateTarget(Arc::TargetGenerator &Mom, Arc::URL url);
    static ACC* Instance(Arc::Config *cfg, Arc::ChainContext*);
    std::list<std::string> getAttribute(std::string attr, Arc::XMLNode& node);

  }; //end class

} // namespace Arc
