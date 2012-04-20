// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "JobControllerTestACC.h"

namespace Arc {

  Plugin* JobControllerTestACC::GetInstance(PluginArgument *arg) {
    JobControllerPluginArgument *jcarg = dynamic_cast<JobControllerPluginArgument*>(arg);
    if (!jcarg) {
      return NULL;
    }
    return new JobControllerTestACC(*jcarg,arg);
  }

  void JobControllerTestACC::UpdateJobs(std::list<Job*>& jobs, std::list<URL>& IDsProcessed, std::list<URL>& IDsNotProcessed, bool isGrouped) const {
    for (std::list<Job*>::const_iterator it = jobs.begin(); it != jobs.end(); ++it) {
      IDsProcessed.push_back((*it)->JobID);
    }
  }

  bool JobControllerTestACC::CleanJobs(const std::list<Job*>& jobs, std::list<URL>& IDsProcessed, std::list<URL>& IDsNotProcessed, bool isGrouped) const {
    for (std::list<Job*>::const_iterator it = jobs.begin(); it != jobs.end(); ++it) {
      if (JobControllerTestACCControl::cleanStatus) {
        IDsProcessed.push_back((*it)->JobID);
      } else {
        IDsNotProcessed.push_back((*it)->JobID);
      }
    }
    return JobControllerTestACCControl::cleanStatus;
  }
  
  bool JobControllerTestACC::CancelJobs(const std::list<Job*>& jobs, std::list<URL>& IDsProcessed, std::list<URL>& IDsNotProcessed, bool isGrouped) const {
    for (std::list<Job*>::const_iterator it = jobs.begin(); it != jobs.end(); ++it) {
      if (JobControllerTestACCControl::cancelStatus) {
        IDsProcessed.push_back((*it)->JobID);
      } else {
        IDsNotProcessed.push_back((*it)->JobID);
      }
    }
    return JobControllerTestACCControl::cancelStatus;
  }
  
  bool JobControllerTestACC::RenewJobs(const std::list<Job*>& jobs, std::list<URL>& IDsProcessed, std::list<URL>& IDsNotProcessed, bool isGrouped) const {
    for (std::list<Job*>::const_iterator it = jobs.begin(); it != jobs.end(); ++it) {
      if (JobControllerTestACCControl::renewStatus) {
        IDsProcessed.push_back((*it)->JobID);
      } else {
        IDsNotProcessed.push_back((*it)->JobID);
      }
    }
    return JobControllerTestACCControl::renewStatus;
  }
  
  bool JobControllerTestACC::ResumeJobs(const std::list<Job*>& jobs, std::list<URL>& IDsProcessed, std::list<URL>& IDsNotProcessed, bool isGrouped) const {
    for (std::list<Job*>::const_iterator it = jobs.begin(); it != jobs.end(); ++it) {
      if (JobControllerTestACCControl::resumeStatus) {
        IDsProcessed.push_back((*it)->JobID);
      } else {
        IDsNotProcessed.push_back((*it)->JobID);
      }
    }
    return JobControllerTestACCControl::resumeStatus;
  }

}
