#ifndef __ARC_JOBCONTROLLERTESTACC_H__
#define __ARC_JOBCONTROLLERTESTACC_H__

#include <string>
#include <list>

#include <arc/URL.h>
#include <arc/UserConfig.h>
#include <arc/compute/Job.h>
#include <arc/compute/JobControllerPlugin.h>
#include <arc/compute/TestACCControl.h>

namespace Arc {

class JobControllerPluginTestACC
  : public JobControllerPlugin {
public:
  JobControllerPluginTestACC(const UserConfig& usercfg, PluginArgument* parg) : JobControllerPlugin(usercfg, parg) { supportedInterfaces.push_back("org.nordugrid.test"); }
  ~JobControllerPluginTestACC() {}

  virtual void UpdateJobs(std::list<Job*>&, std::list<std::string>& IDsProcessed, std::list<std::string>& IDsNotProcessed, bool isGrouped = false) const;
  
  virtual bool CleanJobs(const std::list<Job*>& jobs, std::list<std::string>& IDsProcessed, std::list<std::string>& IDsNotProcessed, bool isGrouped = false) const;
  virtual bool CancelJobs(const std::list<Job*>& jobs, std::list<std::string>& IDsProcessed, std::list<std::string>& IDsNotProcessed, bool isGrouped = false) const;
  virtual bool RenewJobs(const std::list<Job*>& jobs, std::list<std::string>& IDsProcessed, std::list<std::string>& IDsNotProcessed, bool isGrouped = false) const;
  virtual bool ResumeJobs(const std::list<Job*>& jobs, std::list<std::string>& IDsProcessed, std::list<std::string>& IDsNotProcessed, bool isGrouped = false) const;
  
  virtual bool GetURLToJobResource(const Job& job, Job::ResourceType resource, URL& url) const { url = JobControllerPluginTestACCControl::resourceURL; return JobControllerPluginTestACCControl::resourceExist; }
  virtual bool GetJobDescription(const Job& job, std::string& desc_str) const { desc_str = JobControllerPluginTestACCControl::getJobDescriptionString; return JobControllerPluginTestACCControl::getJobDescriptionStatus; }
  virtual URL CreateURL(std::string service, ServiceType st) const { return JobControllerPluginTestACCControl::createURL; }

  virtual bool isEndpointNotSupported(const std::string& endpoint) const { return endpoint.empty(); }
  
  static Plugin* GetInstance(PluginArgument *arg);
};

}
#endif // __ARC_JOBCONTROLLERTESTACC_H__
