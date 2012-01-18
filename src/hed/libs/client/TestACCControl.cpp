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

TargetGenerator* TargetRetrieverTestACCControl::tg = NULL;
std::list<ExecutionTarget> TargetRetrieverTestACCControl::foundTargets = std::list<ExecutionTarget>();
std::list<Job> TargetRetrieverTestACCControl::foundJobs = std::list<Job>();

void TargetRetrieverTestACCControl::addTarget(ExecutionTarget target) {
  TargetRetrieverTestACCControl::foundTargets.push_back(target);
}

void TargetRetrieverTestACCControl::addJob(Job job) {
  TargetRetrieverTestACCControl::foundJobs.push_back(job);
}


}
