#ifndef __ARC_TESTACCCONTROL_H__
#define __ARC_TESTACCCONTROL_H__

#include <list>
#include <string>

#include <arc/compute/Endpoint.h>
#include <arc/Thread.h>
#include <arc/URL.h>
#include <arc/compute/EndpointQueryingStatus.h>
#include <arc/compute/ExecutionTarget.h>
#include <arc/compute/Job.h>
#include <arc/compute/JobState.h>
#include <arc/compute/SubmissionStatus.h>


namespace Arc {

class BrokerPluginTestACCControl {
  public:
    static bool match;
    static bool less;
};

class JobDescriptionParserPluginTestACCControl {
  public:
    static bool parseStatus;
    static bool unparseStatus;
    static std::list<JobDescription> parsedJobDescriptions;
    static std::string unparsedString;
};

class JobControllerPluginTestACCControl {
  public:
    static bool cleanStatus;
    static bool cancelStatus;
    static bool renewStatus;
    static bool resumeStatus;
    static bool getJobDescriptionStatus;
    static std::string getJobDescriptionString;
    static bool resourceExist;
    static URL resourceURL;
    static URL createURL;
};

class SubmitterPluginTestACCControl {
  public:
    static SubmissionStatus submitStatus;
    static bool migrateStatus;
    static bool modifyStatus;
    static Job submitJob;
    static Job migrateJob;
};

class JobStateTEST : public JobState {
  public:
    JobStateTEST(JobState::StateType type_, const std::string& state_ = "TestState") {
      type = type_;
      state = state_;
    }
};

class JobListRetrieverPluginTESTControl {
public:
  static float delay;
  static std::list<Job> jobs;
  static EndpointQueryingStatus status;
};

class ServiceEndpointRetrieverPluginTESTControl {
public:
  static std::list<SimpleCondition*> condition;
  static std::list<EndpointQueryingStatus> status;
  static std::list< std::list<Endpoint> > endpoints;
};

class TargetInformationRetrieverPluginTESTControl {
public:
  static float delay;
  static std::list<ComputingServiceType> targets;
  static EndpointQueryingStatus status;
};

}

#endif // __ARC_TESTACCCONTROL_H__
