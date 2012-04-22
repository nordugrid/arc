#ifndef __ARC_SUBMITTERPLUGINTESTACC_H__
#define __ARC_SUBMITTERPLUGINTESTACC_H__

#include <arc/URL.h>
#include <arc/UserConfig.h>
#include <arc/client/Job.h>
#include <arc/client/SubmitterPlugin.h>
#include <arc/client/TestACCControl.h>

namespace Arc {
  class ExecutionTarget;
  class Job;
  class JobDescription;
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

    virtual bool Submit(const std::list<JobDescription>& jobdescs, const ExecutionTarget& et, EntityConsumer<Job>& jc, std::list<const JobDescription*>& notSubmitted) { jc.addEntity(SubmitterPluginTestACCControl::submitJob); return SubmitterPluginTestACCControl::submitStatus; }
    virtual bool Migrate(const URL& /*jobid*/, const JobDescription& /*jobdesc*/, const ExecutionTarget& /*et*/, bool /*forcemigration*/, Job& job) { job = SubmitterPluginTestACCControl::migrateJob; return SubmitterPluginTestACCControl::migrateStatus; }
  };

}

#endif // __ARC_SUBMITTERPLUGINTESTACC_H__
