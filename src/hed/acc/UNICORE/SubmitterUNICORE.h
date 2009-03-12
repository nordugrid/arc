#ifndef __ARC_SUBMITTERUNICORE_H__
#define __ARC_SUBMITTERUNICORE_H__

#include <arc/client/Submitter.h>
#include <arc/XMLNode.h>
#include <arc/loader/Loader.h>
#include <arc/client/ClientInterface.h>

namespace Arc {

  class Config;

  class SubmitterUNICORE
    : public Submitter {

  private:
    static Logger logger;

    SubmitterUNICORE(Config *cfg);
    ~SubmitterUNICORE();

  public:
    static Plugin* Instance(PluginArgument* arg);
    bool Submit(const JobDescription& jobdesc, XMLNode& info) const;
    bool Migrate(const URL& jobid, const JobDescription& jobdesc, bool forcemigration, XMLNode& info) const;
  };

} // namespace Arc

#endif // __ARC_SUBMITTERUNICORE_H__
