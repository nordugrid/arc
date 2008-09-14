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
    static ACC* Instance(Config *cfg, ChainContext *cxt);
    bool Submit(JobDescription& jobdesc, XMLNode& info);
  };

} // namespace Arc

#endif // __ARC_SUBMITTERCREAM_H__
