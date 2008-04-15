#include <arc/client/ACC.h>
#include <arc/client/ComputingService.h>
#include <list>
#include <arc/URL.h>

namespace Arc {

  class TargetRetriever : public ACC {
  protected:
    TargetRetriever(Arc::Config *cfg);
    URL m_url;
  public:
    virtual ~TargetRetriever();
    virtual std::list<Glue2::ComputingService_t> getTargets() = 0; 

  };

} // namespace Arc
