#ifndef __ARC_SUBMITTERPLUGINTESTACC_H__
#define __ARC_SUBMITTERPLUGINTESTACC_H__

#include <arc/URL.h>
#include <arc/UserConfig.h>
#include <arc/compute/Job.h>
#include <arc/compute/SubmitterPlugin.h>
#include <arc/compute/TestACCControl.h>

namespace Arc {
  class ExecutionTarget;
  class Job;
  class JobDescription;
  class SubmissionStatus;
  class URL;
  
  class SubmitterPluginTestACC : public SubmitterPlugin {
  public:
    SubmitterPluginTestACC(const UserConfig& usercfg, PluginArgument* parg) : SubmitterPlugin(usercfg, parg) { supportedInterfaces.push_back("org.nordugrid.test"); }
    ~SubmitterPluginTestACC() {}
  
    static Plugin* GetInstance(PluginArgument *arg) {
      SubmitterPluginArgument *jcarg = dynamic_cast<SubmitterPluginArgument*>(arg);
      return jcarg ? new SubmitterPluginTestACC(*jcarg,arg) : NULL;
    }
  
    virtual bool isEndpointNotSupported(const std::string& endpoint) const { return endpoint.empty(); }

    virtual SubmissionStatus Submit(const std::list<JobDescription>& jobdescs, const std::string& endpoint, EntityConsumer<Job>& jc, std::list<const JobDescription*>& notSubmitted);
    virtual SubmissionStatus Submit(const std::list<JobDescription>& jobdescs, const ExecutionTarget& et, EntityConsumer<Job>& jc, std::list<const JobDescription*>& notSubmitted);
    virtual bool Migrate(const std::string& /*jobid*/, const JobDescription& /*jobdesc*/, const ExecutionTarget& /*et*/, bool /*forcemigration*/, Job& job) { job = SubmitterPluginTestACCControl::migrateJob; return SubmitterPluginTestACCControl::migrateStatus; }
  };

}

#endif // __ARC_SUBMITTERPLUGINTESTACC_H__
