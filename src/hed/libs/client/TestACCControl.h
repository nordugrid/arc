#ifndef __ARC_TESTACCCONTROL_H__
#define __ARC_TESTACCCONTROL_H__

#include <list>
#include <string>

#include <arc/client/Endpoint.h>
#include <arc/URL.h>
#include <arc/client/EndpointQueryingStatus.h>
#include <arc/client/ExecutionTarget.h>
#include <arc/client/Job.h>
#include <arc/client/JobState.h>


namespace Arc {

class BrokerPluginTestACCControl {
  public:
    static bool match;
    static bool less;
};

class JobDescriptionParserTestACCControl {
  public:
    static bool parseStatus;
    static bool unparseStatus;
    static std::list<JobDescription> parsedJobDescriptions;
    static std::string unparsedString;
};

class JobControllerTestACCControl {
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
    static bool submitStatus;
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
  static float delay;
  static EndpointQueryingStatus status;
  static std::list<Endpoint> endpoints;
};

class TargetInformationRetrieverPluginTESTControl {
public:
  static float delay;
  static std::list<ComputingServiceType> targets;
  static EndpointQueryingStatus status;
};

}

#endif // __ARC_TESTACCCONTROL_H__
