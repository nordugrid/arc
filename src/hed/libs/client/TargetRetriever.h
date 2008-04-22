/**
 * Base class for target retrievers
 */
#ifndef ARCLIB_TARGETRETRIEVER
#define ARCLIB_TARGETRETRIEVER

#include <arc/client/TargetGenerator.h>
#include <arc/client/ACC.h>
#include <arc/URL.h>

namespace Arc {

  class TargetRetriever : public ACC {
  protected:
    TargetRetriever(Arc::Config *cfg);
    URL m_url;
  public:
    virtual ~TargetRetriever();
    virtual void GetTargets(Arc::TargetGenerator &Mom, int TargetType, int DetailLevel) = 0;
    
  };
  
} // namespace Arc

#endif
