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
