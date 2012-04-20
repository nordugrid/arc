// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_JOBCONTROLLEREMIES_H__
#define __ARC_JOBCONTROLLEREMIES_H__

#include <arc/client/JobController.h>

namespace Arc {

  class URL;

  class JobControllerEMIES : public JobController {
  public:
    JobControllerEMIES(const UserConfig& usercfg, PluginArgument* parg) : JobController(usercfg, parg) { supportedInterfaces.push_back("org.ogf.emies"); }
    ~JobControllerEMIES() {}

    static Plugin* Instance(PluginArgument *arg) {
      JobControllerPluginArgument *jcarg = dynamic_cast<JobControllerPluginArgument*>(arg);
      return jcarg ? new JobControllerEMIES(*jcarg, arg) : NULL;
    }
  
    virtual bool isEndpointNotSupported(const std::string& endpoint) const;
  
    virtual void UpdateJobs(std::list<Job*>& jobs, std::list<URL>& IDsProcessed, std::list<URL>& IDsNotProcessed, bool isGrouped = false) const;
    
    virtual bool CleanJobs(const std::list<Job*>& jobs, std::list<URL>& IDsProcessed, std::list<URL>& IDsNotProcessed, bool isGrouped = false) const;
    virtual bool CancelJobs(const std::list<Job*>& jobs, std::list<URL>& IDsProcessed, std::list<URL>& IDsNotProcessed, bool isGrouped = false) const;
    virtual bool RenewJobs(const std::list<Job*>& jobs, std::list<URL>& IDsProcessed, std::list<URL>& IDsNotProcessed, bool isGrouped = false) const;
    virtual bool ResumeJobs(const std::list<Job*>& jobs, std::list<URL>& IDsProcessed, std::list<URL>& IDsNotProcessed, bool isGrouped = false) const;
    
    virtual bool GetURLToJobResource(const Job& job, Job::ResourceType resource, URL& url) const;
    virtual bool GetJobDescription(const Job& job, std::string& desc_str) const;

  private:
    static Logger logger;
  };

} // namespace Arc

#endif // __ARC_JOBCONTROLLEREMIES_H__
