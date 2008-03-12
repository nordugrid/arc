#include <arc/client/TargetRetriever.h>

namespace Arc {

  class ChainContext;

  class TargetRetrieverARC0 : public Arc::TargetRetriever{
    TargetRetrieverARC0(Arc::Config *cfg);
    ~TargetRetrieverARC0();

  public:
    std::list<ACC*> getTargets(); 
    static ACC* Instance(Arc::Config *cfg, Arc::ChainContext*);

  };

} // namespace Arc
