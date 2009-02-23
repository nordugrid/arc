#ifndef __ARC_DMCARC_H__
#define __ARC_DMCARC_H__

#include <arc/data/DMC.h>

namespace Arc {

  class DMCARC
    : public DMC {
  public:
    DMCARC(Config *cfg);
    ~DMCARC();
    static DMC *Instance(Config *cfg);
    DataPoint *iGetDataPoint(const URL& url);
  protected:
    static Logger logger;
  };

} // namespace Arc

#endif // __ARC_DMCARC_H__
