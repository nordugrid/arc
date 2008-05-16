#ifndef __ARC_JOBARC0__
#define __ARC_JOBARC0__

#include <arc/client/Job.h>

namespace Arc {

  class ChainContext;
  class Config;

  class JobARC0 : public Job {

   private:
    JobARC0(Config *cfg);
    ~JobARC0();

   public:
    static ACC* Instance(Config *cfg, ChainContext *cxt);
    void GetStatus();
    void Kill();
  };

} // namespace Arc

#endif // __ARC_JOBARC0__
