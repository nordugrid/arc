#ifndef __ARC_SUBMITTERUNICORE_H__
#define __ARC_SUBMITTERUNICORE_H__

#include <arc/client/Submitter.h>
#include <arc/XMLNode.h>
#include <arc/loader/Loader.h>
#include <arc/client/ClientInterface.h>

namespace Arc {

  class ChainContext;
  class Config;

  class SubmitterUNICORE
    : public Submitter {

  private:
    static Logger logger;

    SubmitterUNICORE(Config *cfg);
    ~SubmitterUNICORE();

  public:
    static ACC* Instance(Config *cfg, ChainContext *cxt);
    bool Submit(JobDescription& jobdesc, XMLNode& info);
  };

} // namespace Arc

#endif // __ARC_SUBMITTERUNICORE_H__
