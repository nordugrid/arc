#ifndef __ARC_SUBMITTERARC0__
#define __ARC_SUBMITTERARC0__

#include <arc/client/Submitter.h>

namespace Arc {

  class ChainContext;
  class Config;

  class SubmitterARC0 : public Submitter {

   private:
    SubmitterARC0(Config *cfg);
    ~SubmitterARC0();

   public:
    static ACC* Instance(Config *cfg, ChainContext *cxt);
    std::pair<URL, URL> Submit(const std::string& jobdesc);
  };

} // namespace Arc

#endif // __ARC_SUBMITTERARC0__
