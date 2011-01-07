// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_SUBMITTERCREAM_H__
#define __ARC_SUBMITTERCREAM_H__

#include <arc/client/Submitter.h>

namespace Arc {

  class Config;

  class SubmitterCREAM
    : public Submitter {

  private:
    SubmitterCREAM(const UserConfig& usercfg);
    ~SubmitterCREAM();

  public:
    static Plugin* Instance(PluginArgument *arg);
    virtual bool Submit(const JobDescription& jobdesc, const ExecutionTarget& et, Job& job);
    virtual bool Migrate(const URL& jobid, const JobDescription& jobdesc,
                         const ExecutionTarget& et, bool forcemigration,
                         Job& job);
    virtual bool ModifyJobDescription(JobDescription& jobdesc, const ExecutionTarget& et) const;
  };

} // namespace Arc

#endif // __ARC_SUBMITTERCREAM_H__
