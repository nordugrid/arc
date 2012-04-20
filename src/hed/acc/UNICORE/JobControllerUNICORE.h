// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_JOBCONTROLLERUNICORE_H__
#define __ARC_JOBCONTROLLERUNICORE_H__

#include <arc/client/JobController.h>

namespace Arc {

  class URL;

  class JobControllerUNICORE : public JobController {
  public:
    JobControllerUNICORE(const UserConfig& usercfg, PluginArgument* parg) : JobController(usercfg, parg) { supportedInterfaces.push_back("org.unicore.xbes"); }
    ~JobControllerUNICORE() {}

    static Plugin* Instance(PluginArgument *arg) {
      JobControllerPluginArgument *jcarg = dynamic_cast<JobControllerPluginArgument*>(arg);
      return jcarg ? new JobControllerUNICORE(*jcarg, arg) : NULL;
    }

    virtual bool isEndpointNotSupported(const std::string& endpoint) const;

    virtual void UpdateJobs(std::list<Job*>& jobs) const;
    virtual bool RetrieveJob(const Job& job, std::string& downloaddir, bool usejobname, bool force) const;
    virtual bool CleanJob(const Job& job) const;
    virtual bool CancelJob(const Job& job) const;
    virtual bool RenewJob(const Job& job) const;
    virtual bool ResumeJob(const Job& job) const;
    virtual bool GetURLToJobResource(const Job& job, Job::ResourceType resource, URL& url) const { return false; }
    virtual bool GetJobDescription(const Job& job, std::string& desc_str) const;

  private:
    static Logger logger;
  };

} // namespace Arc

#endif // __ARC_JOBCONTROLLERUNICORE_H__
