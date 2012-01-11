#ifndef __ARC_JOBCONTROLLERTESTACC_H__
#define __ARC_JOBCONTROLLERTESTACC_H__

#include <string>
#include <list>

#include <arc/Logger.h>
#include <arc/URL.h>
#include <arc/UserConfig.h>
#include <arc/client/Job.h>
#include <arc/client/JobController.h>

#include "TestACCControl.h"

class JobControllerTestACC
  : public Arc::JobController {
public:
  JobControllerTestACC(const Arc::UserConfig& usercfg)
    : JobController(usercfg, "TEST") { JobControllerTestACCControl::jobs = &jobstore; }
  ~JobControllerTestACC() {}

  void UpdateJobs(std::list<Arc::Job>&) const {}
  virtual bool RetrieveJob(const Arc::Job& job, std::string& downloaddir, const bool usejobname, const bool force) const { return JobControllerTestACCControl::jobStatus; }
  virtual bool CleanJob(const Arc::Job& job) const { return JobControllerTestACCControl::cleanStatus; }
  virtual bool CancelJob(const Arc::Job& job) const { return JobControllerTestACCControl::cancelStatus; }
  virtual bool RenewJob(const Arc::Job& job) const { return JobControllerTestACCControl::renewStatus; }
  virtual bool ResumeJob(const Arc::Job& job) const { return JobControllerTestACCControl::resumeStatus; }
  virtual Arc::URL GetFileUrlForJob(const Arc::Job& job, const std::string& whichfile) const { return JobControllerTestACCControl::fileURL; }
  virtual bool GetJobDescription(const Arc::Job& job, std::string& desc_str) const { desc_str = JobControllerTestACCControl::getJobDescriptionString; return JobControllerTestACCControl::getJobDescriptionStatus; }
  virtual Arc::URL CreateURL(std::string service, Arc::ServiceType st) const { return JobControllerTestACCControl::createURL; }

  static Arc::Plugin* GetInstance(Arc::PluginArgument *arg);

private:
  static Arc::Logger logger;
};

#endif // __ARC_JOBCONTROLLERTESTACC_H__
