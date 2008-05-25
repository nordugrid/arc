#ifndef __ARC_SUBMITTERARC1_H__
#define __ARC_SUBMITTERARC1_H__

#include <arc/client/Submitter.h>

namespace Arc {

  class ChainContext;
  class Config;

  class SubmitterARC1
    : public Submitter {

  private:
    SubmitterARC1(Config *cfg);
    ~SubmitterARC1();

  public:
    static ACC *Instance(Config *cfg, ChainContext *cxt);
    std::pair<URL, URL> Submit(const std::string& jobdesc);
  };

} // namespace Arc

#endif // __ARC_SUBMITTERARC1_H__
