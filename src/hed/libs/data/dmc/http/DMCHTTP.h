#ifndef __ARC_DMCHTTP_H__
#define __ARC_DMCHTTP_H__

#include "data/DMC.h"

namespace Arc {

  class DMCHTTP : public DMC {
   public:
    DMCHTTP(Config *cfg);
    ~DMCHTTP();
    static DMC* Instance(Config *cfg, ChainContext *ctx);
    DataPoint* iGetDataPoint(const URL& url);
   protected:
    static Logger logger;
  };

} // namespace Arc

#endif // __ARC_DMCHTTP_H__
