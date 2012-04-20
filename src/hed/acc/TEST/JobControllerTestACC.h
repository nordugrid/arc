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

  void UpdateJobs(std::list<Job*>&) const {}
  virtual bool CleanJob(const Job& job) const { return JobControllerTestACCControl::cleanStatus; }
  virtual bool CancelJob(const Job& job) const { return JobControllerTestACCControl::cancelStatus; }
  virtual bool RenewJob(const Job& job) const { return JobControllerTestACCControl::renewStatus; }
  virtual bool ResumeJob(const Job& job) const { return JobControllerTestACCControl::resumeStatus; }
  virtual bool GetURLToJobResource(const Job& job, Job::ResourceType resource, URL& url) const { url = JobControllerTestACCControl::resourceURL; return JobControllerTestACCControl::resourceExist; }
  virtual bool GetJobDescription(const Job& job, std::string& desc_str) const { desc_str = JobControllerTestACCControl::getJobDescriptionString; return JobControllerTestACCControl::getJobDescriptionStatus; }
  virtual Arc::URL CreateURL(std::string service, ServiceType st) const { return JobControllerTestACCControl::createURL; }

  virtual bool isEndpointNotSupported(const std::string& endpoint) const { return endpoint.empty(); }
  
  static Plugin* GetInstance(PluginArgument *arg);
};

}
#endif // __ARC_JOBCONTROLLERTESTACC_H__
