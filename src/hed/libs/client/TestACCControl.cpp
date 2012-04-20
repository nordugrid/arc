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

bool JobControllerTestACCControl::cleanStatus = true;
bool JobControllerTestACCControl::cancelStatus = true;
bool JobControllerTestACCControl::renewStatus = true;
bool JobControllerTestACCControl::resumeStatus = true;
bool JobControllerTestACCControl::getJobDescriptionStatus = true;
std::string JobControllerTestACCControl::getJobDescriptionString = "";
bool JobControllerTestACCControl::resourceExist = true;
URL JobControllerTestACCControl::resourceURL = URL();
URL JobControllerTestACCControl::createURL = URL();

bool SubmitterPluginTestACCControl::submitStatus = true;
bool SubmitterPluginTestACCControl::migrateStatus = true;
bool SubmitterPluginTestACCControl::modifyStatus = true;
Job SubmitterPluginTestACCControl::submitJob = Job();
Job SubmitterPluginTestACCControl::migrateJob = Job();

float TargetInformationRetrieverPluginTESTControl::delay = 0;
std::list<ComputingServiceType> TargetInformationRetrieverPluginTESTControl::targets;
EndpointQueryingStatus TargetInformationRetrieverPluginTESTControl::status;

float ServiceEndpointRetrieverPluginTESTControl::delay = 0;
EndpointQueryingStatus ServiceEndpointRetrieverPluginTESTControl::status;
std::list<Endpoint> ServiceEndpointRetrieverPluginTESTControl::endpoints = std::list<Endpoint>();

float JobListRetrieverPluginTESTControl::delay = 0;
EndpointQueryingStatus JobListRetrieverPluginTESTControl::status;
std::list<Job> JobListRetrieverPluginTESTControl::jobs = std::list<Job>();

}
