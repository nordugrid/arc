// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_SUBMITTERARC0_H__
#define __ARC_SUBMITTERARC0_H__

#ifdef WIN32
#include <fcntl.h>
#endif

#include <arc/client/Submitter.h>

namespace Arc {

  class Config;

  class SubmitterARC0
    : public Submitter {

  private:
    SubmitterARC0(const UserConfig& usercfg);
    ~SubmitterARC0();

    static Logger logger;

  public:
    static Plugin* Instance(PluginArgument *arg);
    virtual bool Submit(const JobDescription& jobdesc, const ExecutionTarget& et, Job& job);
    virtual bool Migrate(const URL& jobid, const JobDescription& jobdesc,
                         const ExecutionTarget& et, bool forcemigration,
                         Job& job);
    virtual bool ModifyJobDescription(JobDescription& jobdesc, const ExecutionTarget& et) const;
  };

} // namespace Arc

#endif // __ARC_SUBMITTERARC0_H__
