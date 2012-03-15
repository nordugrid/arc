#ifndef __ARC_SUBMITTERTESTACC_H__
#define __ARC_SUBMITTERTESTACC_H__

#include <arc/URL.h>
#include <arc/UserConfig.h>
#include <arc/client/Job.h>
#include <arc/client/Submitter.h>
#include <arc/client/TestACCControl.h>

namespace Arc {
class ExecutionTarget;
class Job;
class JobDescription;
class URL;

class SubmitterTestACC
  : public Submitter {
private:
  SubmitterTestACC(const UserConfig& usercfg, PluginArgument* parg)
    : Submitter(usercfg, "TEST", parg) {}

public:
  ~SubmitterTestACC() {}

  static Plugin* GetInstance(PluginArgument *arg);

  virtual bool Submit(const JobDescription& /*jobdesc*/, const ExecutionTarget& /*et*/, Job& job) { SubmitterTestACCControl::submitJob = &job; return SubmitterTestACCControl::submitStatus; }
  virtual bool Migrate(const URL& /*jobid*/, const JobDescription& /*jobdesc*/, const ExecutionTarget& /*et*/, bool /*forcemigration*/, Job& job) { SubmitterTestACCControl::migrateJob = &job; return SubmitterTestACCControl::migrateStatus; }
  virtual bool ModifyJobDescription(JobDescription& /*jobdesc*/, const ExecutionTarget& /*et*/) const { return SubmitterTestACCControl::modifyStatus; }
};

}

#endif // __ARC_SUBMITTERTESTACC_H__
