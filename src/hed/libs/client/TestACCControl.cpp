#include "TestACCControl.h"

namespace Arc {

bool* BrokerTestACCControl::TargetSortingDone = NULL;
std::list<ExecutionTarget*>* BrokerTestACCControl::PossibleTargets = NULL;

bool JobDescriptionParserTestACCControl::parseStatus = true;
std::list<JobDescription> JobDescriptionParserTestACCControl::parsedJobDescriptions(1, JobDescription());
bool JobDescriptionParserTestACCControl::unparseStatus = true;
std::string JobDescriptionParserTestACCControl::unparsedString = "";

bool JobControllerTestACCControl::jobStatus = true;
bool JobControllerTestACCControl::cleanStatus = true;
bool JobControllerTestACCControl::cancelStatus = true;
bool JobControllerTestACCControl::renewStatus = true;
bool JobControllerTestACCControl::resumeStatus = true;
bool JobControllerTestACCControl::getJobDescriptionStatus = true;
std::string JobControllerTestACCControl::getJobDescriptionString = "";
URL JobControllerTestACCControl::fileURL = URL();
URL JobControllerTestACCControl::createURL = URL();

bool SubmitterTestACCControl::submitStatus = true;
bool SubmitterTestACCControl::migrateStatus = true;
bool SubmitterTestACCControl::modifyStatus = true;
Job* SubmitterTestACCControl::submitJob = NULL;
Job* SubmitterTestACCControl::migrateJob = NULL;

float TargetInformationRetrieverPluginTESTControl::delay = 0;
std::list<ExecutionTarget> TargetInformationRetrieverPluginTESTControl::targets;
EndpointQueryingStatus TargetInformationRetrieverPluginTESTControl::status;

float ServiceEndpointRetrieverPluginTESTControl::delay = 0;
EndpointQueryingStatus ServiceEndpointRetrieverPluginTESTControl::status;
std::list<ServiceEndpoint> ServiceEndpointRetrieverPluginTESTControl::endpoints = std::list<ServiceEndpoint>();

float JobListRetrieverPluginTESTControl::delay = 0;
EndpointQueryingStatus JobListRetrieverPluginTESTControl::status;
std::list<Job> JobListRetrieverPluginTESTControl::jobs = std::list<Job>();

}
