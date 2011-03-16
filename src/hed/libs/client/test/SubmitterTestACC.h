#ifndef __ARC_SUBMITTERTESTACC_H__
#define __ARC_SUBMITTERTESTACC_H__

#include <arc/Logger.h>
#include <arc/URL.h>
#include <arc/UserConfig.h>
#include <arc/client/Job.h>
#include <arc/client/Submitter.h>

#include "TestACCControl.h"

namespace Arc {
class ExecutionTarget;
class Job;
class JobDescription;
class URL;
}

class SubmitterTestACC
  : public Arc::Submitter {
private:
  SubmitterTestACC(const Arc::UserConfig& usercfg)
    : Submitter(usercfg, "TEST") {}

public:
  ~SubmitterTestACC() {}

  static Arc::Plugin* GetInstance(Arc::PluginArgument *arg);

  bool Submit(const Arc::JobDescription& /*jobdesc*/, const Arc::ExecutionTarget& /*et*/, Arc::Job& job) { SubmitterTestACCControl::submitJob = &job; return SubmitterTestACCControl::submitStatus; }
  bool Migrate(const Arc::URL& /*jobid*/, const Arc::JobDescription& /*jobdesc*/, const Arc::ExecutionTarget& /*et*/, bool /*forcemigration*/, Arc::Job& job) { SubmitterTestACCControl::migrateJob = &job; SubmitterTestACCControl::migrateStatus; }
  bool ModifyJobDescription(Arc::JobDescription& /*jobdesc*/, const Arc::ExecutionTarget& /*et*/) const { return SubmitterTestACCControl::modifyStatus; }

  static Arc::Logger logger;
};

#endif // __ARC_SUBMITTERTESTACC_H__
