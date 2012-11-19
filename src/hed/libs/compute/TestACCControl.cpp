#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "TestACCControl.h"

namespace Arc {

bool BrokerPluginTestACCControl::match = false;
bool BrokerPluginTestACCControl::less = false;

bool JobDescriptionParserTestACCControl::parseStatus = true;
std::list<JobDescription> JobDescriptionParserTestACCControl::parsedJobDescriptions(1, JobDescription());
bool JobDescriptionParserTestACCControl::unparseStatus = true;
std::string JobDescriptionParserTestACCControl::unparsedString = "";

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
bool SubmitterPluginTestACCControl::migrateStatus = true;
bool SubmitterPluginTestACCControl::modifyStatus = true;
Job SubmitterPluginTestACCControl::submitJob = Job();
Job SubmitterPluginTestACCControl::migrateJob = Job();

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
