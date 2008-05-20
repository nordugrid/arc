#ifndef __ARC_DMCLDAP_H__
#define __ARC_DMCLDAP_H__

#include <arc/data/DMC.h>

namespace Arc {

  class DMCLDAP
    : public DMC {
  public:
    DMCLDAP(Config *cfg);
    ~DMCLDAP();
    static DMC *Instance(Config *cfg, ChainContext *ctx);
    DataPoint *iGetDataPoint(const URL& url);
  protected:
    static Logger logger;
  };

} // namespace Arc

#endif // __ARC_DMCLDAP_H__
