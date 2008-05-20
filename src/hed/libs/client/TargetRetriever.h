/**
 * Base class for target retrievers
 */
#ifndef ARCLIB_TARGETRETRIEVER
#define ARCLIB_TARGETRETRIEVER

#include <arc/client/TargetGenerator.h>
#include <arc/client/ACC.h>
#include <arc/URL.h>
#include <string>

namespace Arc {

  class TargetRetriever
    : public ACC {
  protected:
    TargetRetriever(Config *cfg);
    std::string m_url;
    std::string ServiceType;
  public:
    virtual ~TargetRetriever();
    virtual void GetTargets(TargetGenerator& Mom, int TargetType,
			    int DetailLevel) = 0;
  };

} // namespace Arc

#endif
