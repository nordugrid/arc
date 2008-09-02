#ifndef __ARC_SUBMITTERARC1_H__
#define __ARC_SUBMITTERARC1_H__

#include <arc/client/Submitter.h>
#include <arc/XMLNode.h>
#include <arc/loader/Loader.h>
#include <arc/client/ClientInterface.h>

namespace Arc { 

  class ChainContext;
  class Config;

  class SubmitterARC1
    : public Submitter {

  private:
    static Logger logger;

    SubmitterARC1(Config *cfg);
    ~SubmitterARC1();

  public:
    static ACC *Instance(Config *cfg, ChainContext *cxt);
    bool Submit(JobDescription& jobdesc, XMLNode &info);
  };

} // namespace Arc

#endif // __ARC_SUBMITTERARC1_H__
