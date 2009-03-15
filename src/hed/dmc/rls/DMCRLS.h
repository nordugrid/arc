// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_DMCRLS_H__
#define __ARC_DMCRLS_H__

#include <arc/data/DMC.h>

namespace Arc {

  class DMCRLS
    : public DMC {
  public:
    DMCRLS(Config *cfg);
    virtual ~DMCRLS();
    static Plugin* Instance(PluginArgument *arg);
    virtual DataPoint* iGetDataPoint(const URL& url);
  protected:
    static Logger logger;
  };

} // namespace Arc

#endif // __ARC_DMCRLS_H__
