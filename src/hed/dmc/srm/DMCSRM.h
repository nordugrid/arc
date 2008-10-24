#ifndef __ARC_DMCSRM_H__
#define __ARC_DMCSRM_H__

#include <arc/data/DMC.h>

namespace Arc {

  class DMCSRM
    : public DMC {
  public:
    DMCSRM(Config *cfg);
    ~DMCSRM();
    static DMC *Instance(Config *cfg, ChainContext *ctx);
    DataPoint *iGetDataPoint(const URL& url);
  protected:
    static Logger logger;
  };

} // namespace Arc

#endif // __ARC_DMCSRM_H__
