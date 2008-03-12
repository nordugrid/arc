#include <arc/client/ACC.h>
#include <list>
#include <arc/URL.h>

namespace Arc {

  class TargetRetriever : public ACC {
  protected:
    TargetRetriever(Arc::Config *cfg);
    URL m_url;
  public:
    virtual ~TargetRetriever();
    virtual std::list<ACC*> getTargets() = 0; 

  };

} // namespace Arc
