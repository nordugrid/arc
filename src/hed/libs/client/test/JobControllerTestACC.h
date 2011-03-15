#ifndef __ARC_JOBCONTROLLERTESTACC_H__
#define __ARC_JOBCONTROLLERTESTACC_H__

#include <string>
#include <list>

#include <arc/Logger.h>
#include <arc/URL.h>
#include <arc/UserConfig.h>
#include <arc/client/Job.h>
#include <arc/client/JobController.h>

#include "JobControllerTestACCControl.h"

class JobControllerTestACC
  : public Arc::JobController {
private:
  JobControllerTestACC(const Arc::UserConfig& usercfg)
    : JobController(usercfg, "TEST") { JobControllerTestACCControl::jobs = &jobstore; }

public:
  ~JobControllerTestACC() {}

  void GetJobInformation() {}
  static Arc::Plugin* GetInstance(Arc::PluginArgument *arg);

private:
  bool GetJob(const Arc::Job& job, const std::string& downloaddir, const bool usejobname, const bool force) { return JobControllerTestACCControl::jobStatus; }
  bool CleanJob(const Arc::Job& job) { return JobControllerTestACCControl::cleanStatus; }
  bool CancelJob(const Arc::Job& job) { return JobControllerTestACCControl::cancelStatus; }
  bool RenewJob(const Arc::Job& job) { return JobControllerTestACCControl::renewStatus; }
  bool ResumeJob(const Arc::Job& job) { return JobControllerTestACCControl::resumeStatus; }
  Arc::URL GetFileUrlForJob(const Arc::Job& job, const std::string& whichfile) { return JobControllerTestACCControl::fileURL; }
  bool GetJobDescription(const Arc::Job& job, std::string& desc_str) { desc_str = JobControllerTestACCControl::getJobDescriptionString; return JobControllerTestACCControl::getJobDescriptionStatus; }
  Arc::URL CreateURL(std::string service, Arc::ServiceType st) { return JobControllerTestACCControl::createURL; }

  static Arc::Logger logger;
};

#endif // __ARC_JOBCONTROLLERTESTACC_H__
