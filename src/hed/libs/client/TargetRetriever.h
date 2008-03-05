#include <arc/client/ACC.h>
#include <list>
#include <arc/URL.h>

namespace Arc {

  class TargetRetriever : public ACC {
  protected:
    TargetRetriever(Arc::Config *cfg, const URL &url);
  public:
    virtual ~TargetRetriever();
    virtual std::list<ACC*> getTargets() = 0; 

  private:
    URL m_url;

  };

} // namespace Arc
