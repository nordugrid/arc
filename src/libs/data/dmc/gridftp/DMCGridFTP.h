#ifndef __ARC_DMCGRIDFTP_H__
#define __ARC_DMCGRIDFTP_H__

#include "data/DMC.h"

namespace Arc {

  class DMCGridFTP : public DMC {
   public:
    DMCGridFTP(Config *cfg);
    ~DMCGridFTP();
    static DMC* Instance(Config *cfg, ChainContext *ctx);
    DataPoint* iGetDataPoint(const URL& url);
   protected:
    static Logger logger;
  };

} // namespace Arc

#endif // __ARC_DMCGRIDFTP_H__
