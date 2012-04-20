#ifndef __ARC_JOBCONTROLLERTESTACC_H__
#define __ARC_JOBCONTROLLERTESTACC_H__

#include <string>
#include <list>

#include <arc/URL.h>
#include <arc/UserConfig.h>
#include <arc/client/Job.h>
#include <arc/client/JobController.h>
#include <arc/client/TestACCControl.h>

namespace Arc {

class JobControllerTestACC
  : public JobController {
public:
  JobControllerTestACC(const UserConfig& usercfg, PluginArgument* parg) : JobController(usercfg, parg) { supportedInterfaces.push_back("org.nordugrid.test"); }
  ~JobControllerTestACC() {}

  virtual void UpdateJobs(std::list<Job*>&, std::list<URL>& IDsProcessed, std::list<URL>& IDsNotProcessed, bool isGrouped = false) const;
  
  virtual bool CleanJobs(const std::list<Job*>& jobs, std::list<URL>& IDsProcessed, std::list<URL>& IDsNotProcessed, bool isGrouped = false) const;
  virtual bool CancelJobs(const std::list<Job*>& jobs, std::list<URL>& IDsProcessed, std::list<URL>& IDsNotProcessed, bool isGrouped = false) const;
  virtual bool RenewJobs(const std::list<Job*>& jobs, std::list<URL>& IDsProcessed, std::list<URL>& IDsNotProcessed, bool isGrouped = false) const;
  virtual bool ResumeJobs(const std::list<Job*>& jobs, std::list<URL>& IDsProcessed, std::list<URL>& IDsNotProcessed, bool isGrouped = false) const;
  
  virtual bool GetURLToJobResource(const Job& job, Job::ResourceType resource, URL& url) const { url = JobControllerTestACCControl::resourceURL; return JobControllerTestACCControl::resourceExist; }
  virtual bool GetJobDescription(const Job& job, std::string& desc_str) const { desc_str = JobControllerTestACCControl::getJobDescriptionString; return JobControllerTestACCControl::getJobDescriptionStatus; }
  virtual URL CreateURL(std::string service, ServiceType st) const { return JobControllerTestACCControl::createURL; }

  virtual bool isEndpointNotSupported(const std::string& endpoint) const { return endpoint.empty(); }
  
  static Plugin* GetInstance(PluginArgument *arg);
};

}
#endif // __ARC_JOBCONTROLLERTESTACC_H__
