#ifndef __ARC_SUBMITTERCREAM_H__
#define __ARC_SUBMITTERCREAM_H__

#include <arc/client/Submitter.h>

namespace Arc {

  class ChainContext;
  class Config;

  class SubmitterCREAM
    : public Submitter {

  private:
    SubmitterCREAM(Config *cfg);
    ~SubmitterCREAM();

  public:
    static ACC *Instance(Config *cfg, ChainContext *cxt);
    std::pair<URL, URL> Submit(Arc::JobDescription& jobdesc);
  };

} // namespace Arc

#endif // __ARC_SUBMITTERCREAM_H__
