// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_JOBCONTROLLERINTERNAL_H__
#define __ARC_JOBCONTROLLERINTERNAL_H__

#include <arc/compute/JobControllerPlugin.h>



using namespace Arc;

namespace Arc{
  class URL;

}

namespace ARex {
  class GMConfig;
}

namespace ARexINTERNAL {


  class INTERNALClient;
  class INTERNALClients;
  class JobStateINTERNAL;

  class JobControllerPluginINTERNAL : public Arc::JobControllerPlugin {
  public:
  JobControllerPluginINTERNAL(const UserConfig& usercfg, PluginArgument* parg) : JobControllerPlugin(usercfg, parg),clients(usercfg) {
      supportedInterfaces.push_back("org.nordugrid.internal");
    }
    ~JobControllerPluginINTERNAL() {}

    static Plugin* Instance(PluginArgument *arg) {
      JobControllerPluginArgument *jcarg = dynamic_cast<JobControllerPluginArgument*>(arg);
      return jcarg ? new JobControllerPluginINTERNAL(*jcarg, arg) : NULL;
    }

    virtual bool isEndpointNotSupported(const std::string& endpoint) const;

    virtual void UpdateJobs(std::list<Job*>& jobs, std::list<std::string>& IDsProcessed, std::list<std::string>& IDsNotProcessed, bool isGrouped = false) const;

    virtual bool CleanJobs(const std::list<Job*>& jobs, std::list<std::string>& IDsProcessed, std::list<std::string>& IDsNotProcessed, bool isGrouped = false) const;
    virtual bool CancelJobs(const std::list<Job*>& jobs, std::list<std::string>& IDsProcessed, std::list<std::string>& IDsNotProcessed, bool isGrouped = false) const;
    virtual bool RenewJobs(const std::list<Job*>& jobs, std::list<std::string>& IDsProcessed, std::list<std::string>& IDsNotProcessed, bool isGrouped = false) const;
    virtual bool ResumeJobs(const std::list<Job*>& jobs, std::list<std::string>& IDsProcessed, std::list<std::string>& IDsNotProcessed, bool isGrouped = false) const;

    virtual bool GetURLToJobResource(const Job& job, Job::ResourceType resource, URL& url) const;
    virtual bool GetJobDescription(const Job& job, std::string& desc_str) const;


  private:
    INTERNALClients clients;
    static Logger logger;
  };

} // namespace Arc

#endif // __ARC_JOBCONTROLLERINTERNAL_H__
