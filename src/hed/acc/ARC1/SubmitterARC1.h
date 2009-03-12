#ifndef __ARC_SUBMITTERARC1_H__
#define __ARC_SUBMITTERARC1_H__

#include <arc/client/Submitter.h>
#include <arc/XMLNode.h>
#include <arc/URL.h>
#include <arc/loader/Loader.h>
#include <arc/client/ClientInterface.h>

namespace Arc {

  class Config;

  class SubmitterARC1
    : public Submitter {

  private:
    static Logger logger;

    SubmitterARC1(Config *cfg);
    ~SubmitterARC1();

  public:
    static Plugin* Instance(PluginArgument* arg);
    bool Submit(const JobDescription& jobdesc, XMLNode& info) const;
    bool Migrate(const URL& jobid, const JobDescription& jobdesc, bool forcemigration, XMLNode& info) const;
 };

} // namespace Arc

#endif // __ARC_SUBMITTERARC1_H__
