// -*- indent-tabs-mode: nil -*-

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

    SubmitterUNICORE(const Config& cfg, const UserConfig& usercfg);
    ~SubmitterUNICORE();

  public:
    static Plugin* Instance(PluginArgument *arg);
    URL Submit(const JobDescription& jobdesc,
               const std::string& joblistfile) const;
    URL Migrate(const URL& jobid, const JobDescription& jobdesc,
                bool forcemigration, const std::string& joblistfile) const;
  };

} // namespace Arc

#endif // __ARC_SUBMITTERUNICORE_H__
