#ifndef __ARC_SUBMITTERARC1_H__
#define __ARC_SUBMITTERARC1_H__

#include <arc/client/Submitter.h>
#include <stdexcept>
#include <arc/ArcConfig.h>
#include <arc/Logger.h>
#include <arc/XMLNode.h>
#include <arc/loader/Loader.h>
#include <arc/client/ClientInterface.h>
#include <arc/URL.h>

namespace Arc { 

  class ChainContext;
  class Config;

  class SubmitterARC1
    : public Submitter {

  private:
    static Arc::Logger logger;

    SubmitterARC1(Config *cfg);
    ~SubmitterARC1();

  public:
    static ACC *Instance(Config *cfg, ChainContext *cxt);
    std::pair<URL, URL> Submit(Arc::JobDescription& jobdesc);
  };

} // namespace Arc

#endif // __ARC_SUBMITTERARC1_H__
