#include "TestACCControl.h"

namespace Arc {

bool* BrokerTestACCControl::TargetSortingDone = NULL;
bool* BrokerTestACCControl::TargetSortingDoneSortTargets = NULL;
std::list<ExecutionTarget*>* BrokerTestACCControl::PossibleTargets = NULL;

bool JobDescriptionParserTestACCControl::parseStatus = false;
bool JobDescriptionParserTestACCControl::unparseStatus = false;
std::list<JobDescription>* JobDescriptionParserTestACCControl::parsedJobDescriptions = NULL;
std::string* JobDescriptionParserTestACCControl::unparsedString = NULL;

bool JobControllerTestACCControl::jobStatus = false;
bool JobControllerTestACCControl::cleanStatus = false;
bool JobControllerTestACCControl::cancelStatus = false;
bool JobControllerTestACCControl::renewStatus = false;
bool JobControllerTestACCControl::resumeStatus = false;
bool JobControllerTestACCControl::getJobDescriptionStatus = false;
std::string JobControllerTestACCControl::getJobDescriptionString = "";
URL JobControllerTestACCControl::fileURL = URL();
URL JobControllerTestACCControl::createURL = URL();

bool SubmitterTestACCControl::submitStatus = false;
bool SubmitterTestACCControl::migrateStatus = false;
bool SubmitterTestACCControl::modifyStatus = false;
Job* SubmitterTestACCControl::submitJob = NULL;
Job* SubmitterTestACCControl::migrateJob = NULL;

TargetGenerator* TargetRetrieverTestACCControl::tg = NULL;
std::list<ExecutionTarget>* TargetRetrieverTestACCControl::foundTargets = NULL;
std::list<Job>* TargetRetrieverTestACCControl::foundJobs = NULL;

}
