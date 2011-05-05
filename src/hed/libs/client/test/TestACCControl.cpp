#include "TestACCControl.h"

bool* BrokerTestACCControl::TargetSortingDone = NULL;
bool* BrokerTestACCControl::TargetSortingDoneSortTargets = NULL;
std::list<Arc::ExecutionTarget*>* BrokerTestACCControl::PossibleTargets = NULL;

bool JobDescriptionParserTestACCControl::parseStatus = false;
bool JobDescriptionParserTestACCControl::unparseStatus = false;
std::list<Arc::JobDescription>* JobDescriptionParserTestACCControl::parsedJobDescriptions = NULL;
std::string* JobDescriptionParserTestACCControl::unparsedString = NULL;

bool JobControllerTestACCControl::jobStatus = false;
bool JobControllerTestACCControl::cleanStatus = false;
bool JobControllerTestACCControl::cancelStatus = false;
bool JobControllerTestACCControl::renewStatus = false;
bool JobControllerTestACCControl::resumeStatus = false;
bool JobControllerTestACCControl::getJobDescriptionStatus = false;
std::string JobControllerTestACCControl::getJobDescriptionString = "";
Arc::URL JobControllerTestACCControl::fileURL = Arc::URL();
Arc::URL JobControllerTestACCControl::createURL = Arc::URL();
std::list<Arc::Job>* JobControllerTestACCControl::jobs = NULL;

bool SubmitterTestACCControl::submitStatus = false;
bool SubmitterTestACCControl::migrateStatus = false;
bool SubmitterTestACCControl::modifyStatus = false;
Arc::Job* SubmitterTestACCControl::submitJob = NULL;
Arc::Job* SubmitterTestACCControl::migrateJob = NULL;

Arc::TargetGenerator* TargetRetrieverTestACCControl::tg = NULL;
std::list<Arc::ExecutionTarget>* TargetRetrieverTestACCControl::foundTargets = NULL;
std::list<Arc::Job>* TargetRetrieverTestACCControl::foundJobs = NULL;
