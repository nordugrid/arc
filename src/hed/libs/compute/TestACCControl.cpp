#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "TestACCControl.h"

namespace Arc {

bool BrokerPluginTestACCControl::match = false;
bool BrokerPluginTestACCControl::less = false;

bool JobDescriptionParserPluginTestACCControl::parseStatus = true;
std::list<JobDescription> JobDescriptionParserPluginTestACCControl::parsedJobDescriptions(1, JobDescription());
bool JobDescriptionParserPluginTestACCControl::unparseStatus = true;
std::string JobDescriptionParserPluginTestACCControl::unparsedString = "";

bool JobControllerPluginTestACCControl::cleanStatus = true;
bool JobControllerPluginTestACCControl::cancelStatus = true;
bool JobControllerPluginTestACCControl::renewStatus = true;
bool JobControllerPluginTestACCControl::resumeStatus = true;
bool JobControllerPluginTestACCControl::getJobDescriptionStatus = true;
std::string JobControllerPluginTestACCControl::getJobDescriptionString = "";
bool JobControllerPluginTestACCControl::resourceExist = true;
URL JobControllerPluginTestACCControl::resourceURL = URL();
URL JobControllerPluginTestACCControl::createURL = URL();

SubmissionStatus SubmitterPluginTestACCControl::submitStatus;
bool SubmitterPluginTestACCControl::modifyStatus = true;
Job SubmitterPluginTestACCControl::submitJob = Job();

float TargetInformationRetrieverPluginTESTControl::delay = 0;
std::list<ComputingServiceType> TargetInformationRetrieverPluginTESTControl::targets;
EndpointQueryingStatus TargetInformationRetrieverPluginTESTControl::status;

std::list<SimpleCondition*> ServiceEndpointRetrieverPluginTESTControl::condition;
std::list<EndpointQueryingStatus> ServiceEndpointRetrieverPluginTESTControl::status;
std::list< std::list<Endpoint> > ServiceEndpointRetrieverPluginTESTControl::endpoints;

float JobListRetrieverPluginTESTControl::delay = 0;
EndpointQueryingStatus JobListRetrieverPluginTESTControl::status;
std::list<Job> JobListRetrieverPluginTESTControl::jobs = std::list<Job>();

}
