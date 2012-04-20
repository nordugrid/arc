// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_JOBCONTROLLERBES_H__
#define __ARC_JOBCONTROLLERBES_H__

#include <arc/client/JobController.h>

namespace Arc {

  class JobControllerBES : public JobController {
  public:
    JobControllerBES(const UserConfig& usercfg, PluginArgument* parg) : JobController(usercfg, parg) { supportedInterfaces.push_back("org.ogf.bes"); }
    ~JobControllerBES() {}

    static Plugin* Instance(PluginArgument *arg) {
      JobControllerPluginArgument *jcarg = dynamic_cast<JobControllerPluginArgument*>(arg);
      return jcarg ? new JobControllerBES(*jcarg, arg) : NULL;
    }

    virtual bool isEndpointNotSupported(const std::string& endpoint) const;
    
    virtual void UpdateJobs(std::list<Job*>& jobs) const;
    virtual bool CleanJob(const Job& job) const;
    virtual bool CancelJob(const Job& job) const;
    virtual bool RenewJob(const Job& job) const;
    virtual bool ResumeJob(const Job& job) const;
    virtual bool GetURLToJobResource(const Job& job, Job::ResourceType resource, URL& url) const { return false; }
    virtual bool GetJobDescription(const Job& job, std::string& desc_str) const;
    virtual URL CreateURL(std::string service, ServiceType st) const;

  private:
    static Logger logger;
  };

} // namespace Arc

#endif // __ARC_JOBCONTROLLERBES_H__
