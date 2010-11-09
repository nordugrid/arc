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

    SubmitterUNICORE(const UserConfig& usercfg);
    ~SubmitterUNICORE();

  public:
    static Plugin* Instance(PluginArgument *arg);
    virtual bool Submit(const JobDescription& jobdesc, const ExecutionTarget& et, Job& job) const;
    virtual bool Migrate(const URL& jobid, const JobDescription& jobdesc,
                         const ExecutionTarget& et, bool forcemigration,
                         Job& job) const;
    virtual bool ModifyJobDescription(JobDescription& jobdesc, const ExecutionTarget& et) const;
  };

} // namespace Arc

#endif // __ARC_SUBMITTERUNICORE_H__
