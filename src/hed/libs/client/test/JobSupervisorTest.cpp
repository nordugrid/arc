#include <cppunit/extensions/HelperMacros.h>

#include <stdlib.h>

#include <arc/URL.h>
#include <arc/UserConfig.h>
#include <arc/Utils.h>
#include <arc/client/Job.h>
#include <arc/client/JobSupervisor.h>

#include "TestACCControl.h"

class JobSupervisorTest
  : public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(JobSupervisorTest);
  CPPUNIT_TEST(TestConstructor);
  CPPUNIT_TEST(TestAddJob);
  CPPUNIT_TEST(TestCancelByIDs);
  CPPUNIT_TEST(TestCancelByStatus);
  CPPUNIT_TEST(TestResubmit);
  CPPUNIT_TEST(TestCleanByIDs);
  CPPUNIT_TEST(TestCleanByStatus);
  CPPUNIT_TEST_SUITE_END();

public:
  JobSupervisorTest();
  ~JobSupervisorTest() {}

  void setUp() {}
  void tearDown() {}

  void TestConstructor();
  void TestAddJob();
  void TestResubmit();
  void TestCancelByIDs();
  void TestCancelByStatus();
  void TestCleanByIDs();
  void TestCleanByStatus();

  class JobStateTEST : public Arc::JobState {
  public:
    JobStateTEST(const std::string& state) : JobState(state, &StateMap) {}
    static JobState::StateType StateMap(const std::string& state) {
      return st;
    }
  };

private:
  Arc::UserConfig usercfg;
  Arc::JobSupervisor *js;
  Arc::Job j;
  static Arc::JobState::StateType st;
};

Arc::JobState::StateType JobSupervisorTest::st = Arc::JobState::UNDEFINED;

JobSupervisorTest::JobSupervisorTest() : usercfg(Arc::initializeCredentialsType(Arc::initializeCredentialsType::SkipCredentials)) {
  Arc::SetEnv("ARC_PLUGIN_PATH", ".libs");

  j.Flavour = "TEST";
  j.Cluster = Arc::URL("http://test.nordugrid.org");
  j.InfoEndpoint = Arc::URL("http://test.nordugrid.org");
}

void JobSupervisorTest::TestConstructor()
{
  std::list<Arc::Job> jobs;
  Arc::URL id1("http://test.nordugrid.org/1234567890test1"), id2("http://test.nordugrid.org/1234567890test2");
  j.IDFromEndpoint = id1;
  jobs.push_back(j);
  j.IDFromEndpoint = id2;
  jobs.push_back(j);

  js = new Arc::JobSupervisor(usercfg, jobs);
  CPPUNIT_ASSERT(js->JobsFound());

  // One and only one JobController should be loaded.
  CPPUNIT_ASSERT_EQUAL(1, (int)js->GetJobControllers().size());

  // JobController should contain 2 jobs.
  CPPUNIT_ASSERT_EQUAL(2, (int)js->GetJobControllers().front()->GetJobs().size());

  CPPUNIT_ASSERT_EQUAL(id1.str(), js->GetJobControllers().front()->GetJobs().front().IDFromEndpoint.str());
  CPPUNIT_ASSERT_EQUAL(id2.str(), js->GetJobControllers().front()->GetJobs().back().IDFromEndpoint.str());

  delete js;
}

void JobSupervisorTest::TestAddJob()
{
  js = new Arc::JobSupervisor(usercfg, std::list<Arc::Job>());
  CPPUNIT_ASSERT(!js->JobsFound());

  j.IDFromEndpoint = Arc::URL("http://test.nordugrid.org/1234567890test1");
  CPPUNIT_ASSERT(js->AddJob(j));
  CPPUNIT_ASSERT(js->JobsFound());

  j.Flavour = "";
  CPPUNIT_ASSERT(!js->AddJob(j));
  CPPUNIT_ASSERT(js->JobsFound());

  j.Flavour = "NON-EXISTENT";
  CPPUNIT_ASSERT(!js->AddJob(j));
  CPPUNIT_ASSERT(js->JobsFound());
  CPPUNIT_ASSERT(!js->AddJob(j));
  CPPUNIT_ASSERT(js->JobsFound());

  delete js;
}

void JobSupervisorTest::TestResubmit()
{
  std::list<Arc::Job> jobs;
  Arc::URL id1("http://test.nordugrid.org/1234567890test1"),
           id2("http://test.nordugrid.org/1234567890test2"),
           id3("http://test.nordugrid.org/1234567890test3");

  st = Arc::JobState::FAILED;
  j.State = JobStateTEST("E");
  j.IDFromEndpoint = id1;
  j.JobDescriptionDocument = "CONTENT";
  jobs.push_back(j);

  st = Arc::JobState::RUNNING;
  j.State = JobStateTEST("R");
  j.IDFromEndpoint = id1;
  j.JobDescriptionDocument = "CONTENT";
  jobs.push_back(j);

  usercfg.Broker("TEST");
  usercfg.AddServices(std::list<std::string>(1, "TEST:http://test2.nordugrid.org"), Arc::COMPUTING);

  js = new Arc::JobSupervisor(usercfg, jobs);

  std::list<Arc::ExecutionTarget> targets(1, Arc::ExecutionTarget());
  targets.back().url = Arc::URL("http://test2.nordugrid.org");
  targets.back().GridFlavour = "TEST";
  targets.back().HealthState = "ok";

  std::list<Arc::JobDescription> jobdescs(1, Arc::JobDescription());

  bool TargetSortingDone = true;
  BrokerTestACCControl::TargetSortingDoneSortTargets = &TargetSortingDone;
  TargetRetrieverTestACCControl::foundTargets = &targets;
  JobDescriptionParserTestACCControl::parseStatus = true;
  JobDescriptionParserTestACCControl::parsedJobDescriptions = &jobdescs;
  SubmitterTestACCControl::submitStatus = true;

  std::list<Arc::Job> resubmitted;
  std::list<Arc::URL> notResubmitted;
  CPPUNIT_ASSERT(js->Resubmit(std::list<std::string>(), 0, resubmitted, notResubmitted));
  CPPUNIT_ASSERT_EQUAL(2, (int)resubmitted.size());

  delete js;
}

void JobSupervisorTest::TestCancelByIDs()
{
  std::list<Arc::Job> jobs;
  Arc::URL id1("http://test.nordugrid.org/1234567890test1"),
           id2("http://test.nordugrid.org/1234567890test2"),
           id3("http://test.nordugrid.org/1234567890test3");

  st = Arc::JobState::RUNNING;
  j.State = JobStateTEST("R");
  j.IDFromEndpoint = id1;
  jobs.push_back(j);

  st = Arc::JobState::FINISHED;
  j.State = JobStateTEST("F");
  j.IDFromEndpoint = id2;
  jobs.push_back(j);

  st = Arc::JobState::UNDEFINED;
  j.State = JobStateTEST("U");
  j.IDFromEndpoint = id3;
  jobs.push_back(j);

  js = new Arc::JobSupervisor(usercfg, jobs);

  std::list<Arc::URL> jobsToBeCanceled;
  jobsToBeCanceled.push_back(id1);
  jobsToBeCanceled.push_back(id2);
  jobsToBeCanceled.push_back(id3);
  jobsToBeCanceled.push_back(Arc::URL("http://test.nordugrid.org/non-existent"));

  JobControllerTestACCControl::cancelStatus = true;

  std::list<Arc::URL> cancelled, notCancelled;
  CPPUNIT_ASSERT(js->CancelByIDs(jobsToBeCanceled, cancelled, notCancelled));

  CPPUNIT_ASSERT_EQUAL(2, (int)cancelled.size());
  CPPUNIT_ASSERT_EQUAL(id1.str(), cancelled.front().str());
  CPPUNIT_ASSERT_EQUAL(id2.str(), cancelled.back().str());

  CPPUNIT_ASSERT_EQUAL(1, (int)notCancelled.size());
  CPPUNIT_ASSERT_EQUAL(id3.str(), notCancelled.back().str());


  cancelled.clear();
  notCancelled.clear();
  JobControllerTestACCControl::cancelStatus = false;
  CPPUNIT_ASSERT(!js->CancelByIDs(jobsToBeCanceled, cancelled, notCancelled));

  CPPUNIT_ASSERT_EQUAL(1, (int)cancelled.size());
  CPPUNIT_ASSERT_EQUAL(id2.str(), cancelled.back().str());

  CPPUNIT_ASSERT_EQUAL(2, (int)notCancelled.size());
  CPPUNIT_ASSERT_EQUAL(id1.str(), notCancelled.front().str());
  CPPUNIT_ASSERT_EQUAL(id3.str(), notCancelled.back().str());

  delete js;
}

void JobSupervisorTest::TestCancelByStatus()
{
  std::list<Arc::Job> jobs;
  Arc::URL id1("http://test.nordugrid.org/1234567890test1"),
           id2("http://test.nordugrid.org/1234567890test2"),
           id3("http://test.nordugrid.org/1234567890test3"),
           id4("http://test.nordugrid.org/1234567890test4");

  st = Arc::JobState::RUNNING;
  j.State = JobStateTEST("R");
  j.IDFromEndpoint = id1;
  jobs.push_back(j);

  st = Arc::JobState::ACCEPTED;
  j.State = JobStateTEST("A");
  j.IDFromEndpoint = id2;
  jobs.push_back(j);

  st = Arc::JobState::FINISHED;
  j.State = JobStateTEST("F");
  j.IDFromEndpoint = id3;
  jobs.push_back(j);

  st = Arc::JobState::UNDEFINED;
  j.State = JobStateTEST("U");
  j.IDFromEndpoint = id4;
  jobs.push_back(j);

  js = new Arc::JobSupervisor(usercfg, jobs);

  std::list<std::string> status;
  status.push_back("A");

  JobControllerTestACCControl::cancelStatus = true;

  std::list<Arc::URL> cancelled, notCancelled;
  CPPUNIT_ASSERT(js->CancelByStatus(status, cancelled, notCancelled));

  CPPUNIT_ASSERT_EQUAL(1, (int)cancelled.size());
  CPPUNIT_ASSERT_EQUAL(id2.str(), cancelled.front().str());
  CPPUNIT_ASSERT_EQUAL(0, (int)notCancelled.size());

  cancelled.clear();
  notCancelled.clear();
  JobControllerTestACCControl::cancelStatus = false;
  CPPUNIT_ASSERT(!js->CancelByStatus(status, cancelled, notCancelled));

  CPPUNIT_ASSERT_EQUAL(0, (int)cancelled.size());

  CPPUNIT_ASSERT_EQUAL(1, (int)notCancelled.size());
  CPPUNIT_ASSERT_EQUAL(id2.str(), notCancelled.front().str());

  status.push_back("R");

  cancelled.clear();
  notCancelled.clear();
  JobControllerTestACCControl::cancelStatus = true;
  CPPUNIT_ASSERT(js->CancelByStatus(status, cancelled, notCancelled));

  CPPUNIT_ASSERT_EQUAL(2, (int)cancelled.size());
  CPPUNIT_ASSERT_EQUAL(id1.str(), cancelled.front().str());
  CPPUNIT_ASSERT_EQUAL(id2.str(), cancelled.back().str());

  CPPUNIT_ASSERT_EQUAL(0, (int)notCancelled.size());

  status.clear();
  
  cancelled.clear();
  notCancelled.clear();
  JobControllerTestACCControl::cancelStatus = true;
  CPPUNIT_ASSERT(js->CancelByStatus(status, cancelled, notCancelled));

  CPPUNIT_ASSERT_EQUAL(2, (int)cancelled.size());
  CPPUNIT_ASSERT_EQUAL(id1.str(), cancelled.front().str());
  CPPUNIT_ASSERT_EQUAL(id2.str(), cancelled.back().str());

  CPPUNIT_ASSERT_EQUAL(0, (int)notCancelled.size());

  delete js;
}

void JobSupervisorTest::TestCleanByIDs()
{
  std::list<Arc::Job> jobs;
  Arc::URL id1("http://test.nordugrid.org/1234567890test1"),
           id2("http://test.nordugrid.org/1234567890test2");

  st = Arc::JobState::FINISHED;
  j.State = JobStateTEST("R");
  j.IDFromEndpoint = id1;
  jobs.push_back(j);

  st = Arc::JobState::UNDEFINED;
  j.State = JobStateTEST("U");
  j.IDFromEndpoint = id2;
  jobs.push_back(j);

  js = new Arc::JobSupervisor(usercfg, jobs);

  std::list<Arc::URL> jobsToBeCleaned;
  jobsToBeCleaned.push_back(id1);
  jobsToBeCleaned.push_back(id2);
  jobsToBeCleaned.push_back(Arc::URL("http://test.nordugrid.org/non-existent"));

  JobControllerTestACCControl::cleanStatus = true;

  std::list<Arc::URL> cleaned, notCleaned;
  CPPUNIT_ASSERT(js->CleanByIDs(jobsToBeCleaned, cleaned, notCleaned));

  CPPUNIT_ASSERT_EQUAL(1, (int)cleaned.size());
  CPPUNIT_ASSERT_EQUAL(id1.str(), cleaned.front().str());
  CPPUNIT_ASSERT_EQUAL(1, (int)notCleaned.size());
  CPPUNIT_ASSERT_EQUAL(id2.str(), notCleaned.back().str());

  cleaned.clear();
  notCleaned.clear();
  JobControllerTestACCControl::cleanStatus = false;
  CPPUNIT_ASSERT(!js->CleanByIDs(jobsToBeCleaned, cleaned, notCleaned));

  CPPUNIT_ASSERT_EQUAL(0, (int)cleaned.size());
  CPPUNIT_ASSERT_EQUAL(2, (int)notCleaned.size());
  CPPUNIT_ASSERT_EQUAL(id1.str(), notCleaned.front().str());
  CPPUNIT_ASSERT_EQUAL(id2.str(), notCleaned.back().str());

  delete js;
}

void JobSupervisorTest::TestCleanByStatus()
{
  std::list<Arc::Job> jobs;
  Arc::URL id1("http://test.nordugrid.org/1234567890test1"),
           id2("http://test.nordugrid.org/1234567890test2");

  st = Arc::JobState::FINISHED;
  j.State = JobStateTEST("R");
  j.IDFromEndpoint = id1;
  jobs.push_back(j);

  st = Arc::JobState::UNDEFINED;
  j.State = JobStateTEST("U");
  j.IDFromEndpoint = id2;
  jobs.push_back(j);

  js = new Arc::JobSupervisor(usercfg, jobs);

  std::list<std::string> status;
  status.push_back("R");

  JobControllerTestACCControl::cleanStatus = true;

  std::list<Arc::URL> cleaned, notCleaned;
  CPPUNIT_ASSERT(js->CleanByStatus(status, cleaned, notCleaned));

  CPPUNIT_ASSERT_EQUAL(1, (int)cleaned.size());
  CPPUNIT_ASSERT_EQUAL(id1.str(), cleaned.front().str());
  CPPUNIT_ASSERT_EQUAL(0, (int)notCleaned.size());

  cleaned.clear();
  notCleaned.clear();
  JobControllerTestACCControl::cleanStatus = false;
  CPPUNIT_ASSERT(!js->CleanByStatus(status, cleaned, notCleaned));

  CPPUNIT_ASSERT_EQUAL(0, (int)cleaned.size());
  CPPUNIT_ASSERT_EQUAL(1, (int)notCleaned.size());
  CPPUNIT_ASSERT_EQUAL(id1.str(), notCleaned.front().str());

  delete js;
}

CPPUNIT_TEST_SUITE_REGISTRATION(JobSupervisorTest);
