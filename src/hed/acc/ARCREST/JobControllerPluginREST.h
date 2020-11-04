// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_JOBCONTROLLERREST_H__
#define __ARC_JOBCONTROLLERREST_H__

#include <arc/compute/JobControllerPlugin.h>

namespace Arc {

  class JobControllerPluginREST : public JobControllerPlugin {
  public:
    JobControllerPluginREST(const UserConfig& usercfg, PluginArgument* parg) : JobControllerPlugin(usercfg, parg) { supportedInterfaces.push_back("org.nordugrid.arcrest"); }
    ~JobControllerPluginREST() {}

    static Plugin* Instance(PluginArgument *arg) {
      JobControllerPluginArgument *jcarg = dynamic_cast<JobControllerPluginArgument*>(arg);
      return jcarg ? new JobControllerPluginREST(*jcarg, arg) : NULL;
    }

    bool isEndpointNotSupported(const std::string& endpoint) const;

    virtual void UpdateJobs(std::list<Job*>& jobs, std::list<std::string>& IDsProcessed, std::list<std::string>& IDsNotProcessed, bool isGrouped = false) const;

    virtual bool CleanJobs(const std::list<Job*>& jobs, std::list<std::string>& IDsProcessed, std::list<std::string>& IDsNotProcessed, bool isGrouped = false) const;
    virtual bool CancelJobs(const std::list<Job*>& jobs, std::list<std::string>& IDsProcessed, std::list<std::string>& IDsNotProcessed, bool isGrouped = false) const;
    virtual bool RenewJobs(const std::list<Job*>& jobs, std::list<std::string>& IDsProcessed, std::list<std::string>& IDsNotProcessed, bool isGrouped = false) const;
    virtual bool ResumeJobs(const std::list<Job*>& jobs, std::list<std::string>& IDsProcessed, std::list<std::string>& IDsNotProcessed, bool isGrouped = false) const;

    virtual bool GetURLToJobResource(const Job& job, Job::ResourceType resource, URL& url) const;
    virtual bool GetJobDescription(const Job& job, std::string& desc_str) const;

  private:
    static URL GetAddressOfResource(const Job& job);
    static Logger logger;

    class InfoNodeProcessor {
     public:
      virtual void operator()(std::string const& job_id, XMLNode info_node) {};
    };

    bool ProcessJobs(Arc::URL const & resourceUrl, std::string const & action, int successCode,
          std::list<std::string>& IDs, std::list<std::string>& IDsProcessed, std::list<std::string>& IDsNotProcessed,
          InfoNodeProcessor& infoNodeProcessor) const;

  };

} // namespace Arc

#endif // __ARC_JOBCONTROLLERREST_H__
