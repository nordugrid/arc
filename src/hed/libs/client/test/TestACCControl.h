#ifndef __ARC_TESTACCCONTROL_H__
#define __ARC_TESTACCCONTROL_H__

#include <list>
#include <string>

#include <arc/URL.h>
#include <arc/client/ExecutionTarget.h>
#include <arc/client/Job.h>
#include <arc/client/TargetGenerator.h>

class BrokerTestACCControl {
  public:
    static bool* TargetSortingDone;
    static std::list<Arc::ExecutionTarget*>* PossibleTargets;
};

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

class SubmitterTestACCControl {
  public:
    static bool submitStatus;
    static bool migrateStatus;
    static bool modifyStatus;
    static Arc::Job* submitJob;
    static Arc::Job* migrateJob;
};

class TargetRetrieverTestACCControl {
  public:
    static Arc::TargetGenerator* tg;
};

#endif // __ARC_TESTACCCONTROL_H__
