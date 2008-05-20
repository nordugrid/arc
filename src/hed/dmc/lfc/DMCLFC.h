#ifndef __ARC_DMCLFC_H__
#define __ARC_DMCLFC_H__

#include <arc/data/DMC.h>

namespace Arc {

  class DMCLFC
    : public DMC {
  public:
    DMCLFC(Config *cfg);
    ~DMCLFC();
    static DMC *Instance(Config *cfg, ChainContext *ctx);
    DataPoint *iGetDataPoint(const URL& url);
  protected:
    static Logger logger;
  };

} // namespace Arc

#endif // __ARC_DMCLFC_H__
