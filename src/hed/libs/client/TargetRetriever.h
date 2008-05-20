#ifndef __ARC_TARGETRETRIEVER_H__
#define __ARC_TARGETRETRIEVER_H__

#include <string>

#include <arc/URL.h>
#include <arc/client/ACC.h>
#include <arc/client/TargetGenerator.h>

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

#endif // __ARC_TARGETRETRIEVER_H__
