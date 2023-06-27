#ifndef __ARC_TESTACCCONTROL_H__
#define __ARC_TESTACCCONTROL_H__

/** \file
 * \brief Classes for controlling output of compute test plugins.
 */

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

/**
 * \defgroup testacccontrol Classes for controlling output of compute test plugins
 * The listed classes are used for controlling the behaviour of the test
 * plugins. A test plugin can be used for simulating, testing and checking how
 * the compute library behaves and react to different inputs from plugins. Also
 * the test plugins doesn't require a network connection in order to function.
 * 
 * Compute test plugins are available for the following plugin types:
 * \li BrokerPlugin 
 * \li JobControllerPlugin
 * \li JobDescriptionParserPlugin
 * \li SubmitterPlugin
 * \li ServiceEndpointRetrieverPlugin
 * \li TargetInformationRetrieverPlugin
 * \li JobListRetrieverPlugin
 * 
 * They can be loaded by using the associated plugin loader class.
 * 
 * \ingroup compute
 */

/**
 * \ingroup testacccontrol
 * \headerfile TestACCControl.h arc/compute/TestACCControl.h
 */
class BrokerPluginTestACCControl {
  public:
    static bool match;
    static bool less;
};

/**
 * \ingroup testacccontrol
 * \headerfile TestACCControl.h arc/compute/TestACCControl.h
 */
class JobDescriptionParserPluginTestACCControl {
  public:
    static bool parseStatus;
    static bool unparseStatus;
    static std::list<JobDescription> parsedJobDescriptions;
    static std::string unparsedString;
};

/**
 * \ingroup testacccontrol
 * \headerfile TestACCControl.h arc/compute/TestACCControl.h
 */
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

/**
 * \ingroup testacccontrol
 * \headerfile TestACCControl.h arc/compute/TestACCControl.h
 */
class SubmitterPluginTestACCControl {
  public:
    static SubmissionStatus submitStatus;
    static bool modifyStatus;
    static Job submitJob;
};

/**
 * \ingroup testacccontrol
 * \headerfile TestACCControl.h arc/compute/TestACCControl.h
 */
class JobStateTEST : public JobState {
  public:
    JobStateTEST(JobState::StateType type_, const std::string& state_ = "TestState") {
      type = type_;
      state = state_;
    }
};

/**
 * \ingroup testacccontrol
 * \headerfile TestACCControl.h arc/compute/TestACCControl.h
 */
class JobListRetrieverPluginTESTControl {
public:
  static float delay;
  static std::list<Job> jobs;
  static EndpointQueryingStatus status;
};

/**
 * \ingroup testacccontrol
 * \headerfile TestACCControl.h arc/compute/TestACCControl.h
 */
class ServiceEndpointRetrieverPluginTESTControl {
public:
  static std::list<SimpleCondition*> condition;
  static std::list<EndpointQueryingStatus> status;
  static std::list< std::list<Endpoint> > endpoints;
};

/**
 * \ingroup testacccontrol
 * \headerfile TestACCControl.h arc/compute/TestACCControl.h
 */
class TargetInformationRetrieverPluginTESTControl {
public:
  static float delay;
  static std::list<ComputingServiceType> targets;
  static EndpointQueryingStatus status;
};

}

#endif // __ARC_TESTACCCONTROL_H__
