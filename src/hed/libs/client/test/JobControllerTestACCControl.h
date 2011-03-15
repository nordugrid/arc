#ifndef __ARC_JOBCONTROLLERTESTACCCONTROL_H__
#define __ARC_JOBCONTROLLERTESTACCCONTROL_H__

#include <string>
#include <list>

#include <arc/URL.h>
#include <arc/client/Job.h>

class JobControllerTestACCControl {
  public:
    static bool jobStatus;
    static bool cleanStatus;
    static bool cancelStatus;
    static bool renewStatus;
    static bool resumeStatus;
    static bool getJobDescriptionStatus;
    static std::string getJobDescriptionString;
    static Arc::URL fileURL;
    static Arc::URL createURL;
    static std::list<Arc::Job>* jobs;
};

bool JobControllerTestACCControl::jobStatus = false;
bool JobControllerTestACCControl::cleanStatus = false;
bool JobControllerTestACCControl::cancelStatus = false;
bool JobControllerTestACCControl::renewStatus = false;
bool JobControllerTestACCControl::resumeStatus = false;
bool JobControllerTestACCControl::getJobDescriptionStatus = false;
std::string JobControllerTestACCControl::getJobDescriptionString = "";
Arc::URL JobControllerTestACCControl::fileURL = Arc::URL();
Arc::URL JobControllerTestACCControl::createURL = Arc::URL();
std::list<Arc::Job>* JobControllerTestACCControl::jobs = NULL;

#endif // __ARC_JOBCONTROLLERTESTACCCONTROL_H__
