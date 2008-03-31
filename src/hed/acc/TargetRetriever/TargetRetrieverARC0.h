#include <arc/client/TargetRetriever.h>

namespace Arc {

  class ChainContext;
  class XMLNode;

  class TargetRetrieverARC0 : public Arc::TargetRetriever{
    TargetRetrieverARC0(Arc::Config *cfg);
    ~TargetRetrieverARC0();

  private:
    std::list<std::string> getAttribute(std::string attr, Arc::XMLNode& node);
    
    
  public:
    std::list<ACC*> getTargets(); 
    static ACC* Instance(Arc::Config *cfg, Arc::ChainContext*);

  };

} // namespace Arc
