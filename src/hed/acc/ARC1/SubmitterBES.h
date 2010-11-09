// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_SUBMITTERBES_H__
#define __ARC_SUBMITTERBES_H__

#include <arc/client/Submitter.h>
#include <arc/XMLNode.h>
#include <arc/URL.h>
#include <arc/loader/Loader.h>
#include <arc/client/ClientInterface.h>

namespace Arc {

  class Config;

  class SubmitterBES
    : public Submitter {

  private:
    static Logger logger;

    SubmitterBES(const UserConfig& usercfg);
    ~SubmitterBES();

  public:
    static Plugin* Instance(PluginArgument *arg);
    virtual bool Submit(const JobDescription& jobdesc, const ExecutionTarget& et, Job& job) const;
    virtual bool Migrate(const URL& jobid, const JobDescription& jobdesc,
                        const ExecutionTarget& et, bool forcemigration,
                        Job& job) const;
    virtual bool ModifyJobDescription(JobDescription& jobdesc, const ExecutionTarget& et) const;
  };

} // namespace Arc

#endif // __ARC_SUBMITTERBES_H__
