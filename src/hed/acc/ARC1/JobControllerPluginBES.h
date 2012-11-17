// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_JOBCONTROLLERBES_H__
#define __ARC_JOBCONTROLLERBES_H__

#include <arc/compute/JobControllerPlugin.h>

namespace Arc {

  class JobControllerPluginBES : public JobControllerPlugin {
  public:
    JobControllerPluginBES(const UserConfig& usercfg, PluginArgument* parg) : JobControllerPlugin(usercfg, parg) { supportedInterfaces.push_back("org.ogf.bes"); }
    ~JobControllerPluginBES() {}

    static Plugin* Instance(PluginArgument *arg) {
      JobControllerPluginArgument *jcarg = dynamic_cast<JobControllerPluginArgument*>(arg);
      return jcarg ? new JobControllerPluginBES(*jcarg, arg) : NULL;
    }

    virtual bool isEndpointNotSupported(const std::string& endpoint) const;
    
    virtual void UpdateJobs(std::list<Job*>& jobs, std::list<std::string>& IDsProcessed, std::list<std::string>& IDsNotProcessed, bool isGrouped = false) const;
    
    virtual bool CleanJobs(const std::list<Job*>& jobs, std::list<std::string>& IDsProcessed, std::list<std::string>& IDsNotProcessed, bool isGrouped = false) const;
    virtual bool CancelJobs(const std::list<Job*>& jobs, std::list<std::string>& IDsProcessed, std::list<std::string>& IDsNotProcessed, bool isGrouped = false) const;
    virtual bool RenewJobs(const std::list<Job*>& jobs, std::list<std::string>& IDsProcessed, std::list<std::string>& IDsNotProcessed, bool isGrouped = false) const;
    virtual bool ResumeJobs(const std::list<Job*>& jobs, std::list<std::string>& IDsProcessed, std::list<std::string>& IDsNotProcessed, bool isGrouped = false) const;
    
    virtual bool GetURLToJobResource(const Job& job, Job::ResourceType resource, URL& url) const { return false; }
    virtual bool GetJobDescription(const Job& job, std::string& desc_str) const;
    virtual URL CreateURL(std::string service, ServiceType st) const;

  private:
    static Logger logger;
  };

} // namespace Arc

#endif // __ARC_JOBCONTROLLERBES_H__
