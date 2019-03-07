// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_JOBCONTROLLERGRIDFTPJOB_H__
#define __ARC_JOBCONTROLLERGRIDFTPJOB_H__

#include <arc/compute/JobControllerPlugin.h>

namespace Arc {

  class URL;

  class JobControllerPluginGRIDFTPJOB : public JobControllerPlugin {
  public:
    JobControllerPluginGRIDFTPJOB(const UserConfig& usercfg, PluginArgument* parg) : JobControllerPlugin(usercfg, parg) { supportedInterfaces.push_back("org.nordugrid.gridftpjob"); }
    ~JobControllerPluginGRIDFTPJOB() {}

    static Plugin* Instance(PluginArgument *arg);

    bool isEndpointNotSupported(const std::string& endpoint) const;

    virtual void UpdateJobs(std::list<Job*>& jobs, std::list<std::string>& IDsProcessed, std::list<std::string>& IDsNotProcessed, bool isGrouped = false) const;
    
    virtual bool CleanJobs(const std::list<Job*>& jobs, std::list<std::string>& IDsProcessed, std::list<std::string>& IDsNotProcessed, bool isGrouped = false) const;
    virtual bool CancelJobs(const std::list<Job*>& jobs, std::list<std::string>& IDsProcessed, std::list<std::string>& IDsNotProcessed, bool isGrouped = false) const;
    virtual bool RenewJobs(const std::list<Job*>& jobs, std::list<std::string>& IDsProcessed, std::list<std::string>& IDsNotProcessed, bool isGrouped = false) const;
    virtual bool ResumeJobs(const std::list<Job*>& jobs, std::list<std::string>& IDsProcessed, std::list<std::string>& IDsNotProcessed, bool isGrouped = false) const;
    
    virtual bool GetURLToJobResource(const Job& job, Job::ResourceType resource, URL& url) const;
    virtual bool GetJobDescription(const Job& job, std::string& desc_str) const;

  private:
    static Logger logger;
  };

} // namespace Arc

#endif // __ARC_JOBCONTROLLERGRIDFTPJOB_H__
