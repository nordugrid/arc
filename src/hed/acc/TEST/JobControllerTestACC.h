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
  JobControllerTestACC(const UserConfig& usercfg)
    : JobController(usercfg, "TEST") { JobControllerTestACCControl::jobs = &jobstore; }
  ~JobControllerTestACC() {}

  void UpdateJobs(std::list<Job>&) const {}
  virtual bool RetrieveJob(const Job& job, std::string& downloaddir, const bool usejobname, const bool force) const { return JobControllerTestACCControl::jobStatus; }
  virtual bool CleanJob(const Job& job) const { return JobControllerTestACCControl::cleanStatus; }
  virtual bool CancelJob(const Job& job) const { return JobControllerTestACCControl::cancelStatus; }
  virtual bool RenewJob(const Job& job) const { return JobControllerTestACCControl::renewStatus; }
  virtual bool ResumeJob(const Job& job) const { return JobControllerTestACCControl::resumeStatus; }
  virtual Arc::URL GetFileUrlForJob(const Job& job, const std::string& whichfile) const { return JobControllerTestACCControl::fileURL; }
  virtual bool GetJobDescription(const Job& job, std::string& desc_str) const { desc_str = JobControllerTestACCControl::getJobDescriptionString; return JobControllerTestACCControl::getJobDescriptionStatus; }
  virtual Arc::URL CreateURL(std::string service, ServiceType st) const { return JobControllerTestACCControl::createURL; }

  static Plugin* GetInstance(PluginArgument *arg);
};

}
#endif // __ARC_JOBCONTROLLERTESTACC_H__
