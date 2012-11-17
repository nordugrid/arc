// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "JobControllerPluginTestACC.h"

namespace Arc {

  Plugin* JobControllerPluginTestACC::GetInstance(PluginArgument *arg) {
    JobControllerPluginArgument *jcarg = dynamic_cast<JobControllerPluginArgument*>(arg);
    if (!jcarg) {
      return NULL;
    }
    return new JobControllerPluginTestACC(*jcarg,arg);
  }

  void JobControllerPluginTestACC::UpdateJobs(std::list<Job*>& jobs, std::list<std::string>& IDsProcessed, std::list<std::string>& IDsNotProcessed, bool isGrouped) const {
    for (std::list<Job*>::const_iterator it = jobs.begin(); it != jobs.end(); ++it) {
      IDsProcessed.push_back((*it)->JobID);
    }
  }

  bool JobControllerPluginTestACC::CleanJobs(const std::list<Job*>& jobs, std::list<std::string>& IDsProcessed, std::list<std::string>& IDsNotProcessed, bool isGrouped) const {
    for (std::list<Job*>::const_iterator it = jobs.begin(); it != jobs.end(); ++it) {
      if (JobControllerPluginTestACCControl::cleanStatus) {
        IDsProcessed.push_back((*it)->JobID);
      } else {
        IDsNotProcessed.push_back((*it)->JobID);
      }
    }
    return JobControllerPluginTestACCControl::cleanStatus;
  }
  
  bool JobControllerPluginTestACC::CancelJobs(const std::list<Job*>& jobs, std::list<std::string>& IDsProcessed, std::list<std::string>& IDsNotProcessed, bool isGrouped) const {
    for (std::list<Job*>::const_iterator it = jobs.begin(); it != jobs.end(); ++it) {
      if (JobControllerPluginTestACCControl::cancelStatus) {
        IDsProcessed.push_back((*it)->JobID);
      } else {
        IDsNotProcessed.push_back((*it)->JobID);
      }
    }
    return JobControllerPluginTestACCControl::cancelStatus;
  }
  
  bool JobControllerPluginTestACC::RenewJobs(const std::list<Job*>& jobs, std::list<std::string>& IDsProcessed, std::list<std::string>& IDsNotProcessed, bool isGrouped) const {
    for (std::list<Job*>::const_iterator it = jobs.begin(); it != jobs.end(); ++it) {
      if (JobControllerPluginTestACCControl::renewStatus) {
        IDsProcessed.push_back((*it)->JobID);
      } else {
        IDsNotProcessed.push_back((*it)->JobID);
      }
    }
    return JobControllerPluginTestACCControl::renewStatus;
  }
  
  bool JobControllerPluginTestACC::ResumeJobs(const std::list<Job*>& jobs, std::list<std::string>& IDsProcessed, std::list<std::string>& IDsNotProcessed, bool isGrouped) const {
    for (std::list<Job*>::const_iterator it = jobs.begin(); it != jobs.end(); ++it) {
      if (JobControllerPluginTestACCControl::resumeStatus) {
        IDsProcessed.push_back((*it)->JobID);
      } else {
        IDsNotProcessed.push_back((*it)->JobID);
      }
    }
    return JobControllerPluginTestACCControl::resumeStatus;
  }

}
