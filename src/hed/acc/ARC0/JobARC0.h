#ifndef __ARC_JOBARC0_H__
#define __ARC_JOBARC0_H__

#include <arc/client/Job.h>

namespace Arc {

  class ChainContext;
  class Config;

  class JobARC0
    : public Job {

  private:
    JobARC0(Config *cfg);
    ~JobARC0();

  public:
    static ACC *Instance(Config *cfg, ChainContext *cxt);
    void GetStatus();
    void Kill();
  };

} // namespace Arc

#endif // __ARC_JOBARC0_H__
