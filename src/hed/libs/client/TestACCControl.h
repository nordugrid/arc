#ifndef __ARC_TESTACCCONTROL_H__
#define __ARC_TESTACCCONTROL_H__

#include <list>
#include <string>

#include <arc/URL.h>
#include <arc/client/ExecutionTarget.h>
#include <arc/client/Job.h>
#include <arc/client/TargetGenerator.h>

namespace Arc {

class BrokerTestACCControl {
  public:
    static bool* TargetSortingDone;
    static bool* TargetSortingDoneSortTargets;
    static std::list<ExecutionTarget*>* PossibleTargets;
};

class JobDescriptionParserTestACCControl {
  public:
    static bool parseStatus;
    static bool unparseStatus;
    static std::list<JobDescription>* parsedJobDescriptions;
    static std::string* unparsedString;
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
    static URL fileURL;
    static URL createURL;
    static std::list<Job>* jobs;
};

class SubmitterTestACCControl {
  public:
    static bool submitStatus;
    static bool migrateStatus;
    static bool modifyStatus;
    static Job* submitJob;
    static Job* migrateJob;
};

class TargetRetrieverTestACCControl {
  public:
    static TargetGenerator* tg;
    static std::list<ExecutionTarget>* foundTargets;
    static std::list<Job>* foundJobs;
};

}
#endif // __ARC_TESTACCCONTROL_H__
