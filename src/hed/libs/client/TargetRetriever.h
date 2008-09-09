#ifndef __ARC_TARGETRETRIEVER_H__
#define __ARC_TARGETRETRIEVER_H__

#include <string>

#include <arc/URL.h>
#include <arc/client/ACC.h>

namespace Arc {

  class Config;
  class Logger;
  class TargetGenerator;

  class TargetRetriever
    : public ACC {
  protected:
    TargetRetriever(Config *cfg);
  public:
    virtual ~TargetRetriever();
    virtual void GetTargets(TargetGenerator& mom, int targetType,
			    int detailLevel) = 0;
  protected:
    URL url;
    std::string serviceType;

    static Logger logger;
  };

} // namespace Arc

#endif // __ARC_TARGETRETRIEVER_H__
