#include <cppunit/extensions/HelperMacros.h>

#include <stdlib.h>

#include <arc/URL.h>
#include <arc/UserConfig.h>
#include <arc/Utils.h>
#include <arc/client/Job.h>
#include <arc/client/JobSupervisor.h>

#include <arc/client/TestACCControl.h>

class JobSupervisorTest
  : public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(JobSupervisorTest);
  //CPPUNIT_TEST(TestConstructor);
  //CPPUNIT_TEST(TestAddJob);
  CPPUNIT_TEST(TestResubmit);
  CPPUNIT_TEST(TestCancel);
  CPPUNIT_TEST(TestClean);
  CPPUNIT_TEST_SUITE_END();

public:
  JobSupervisorTest();
  ~JobSupervisorTest() {}

  void setUp() {}
  void tearDown() {}

  void TestConstructor();
  void TestAddJob();
  void TestResubmit();
  void TestCancel();
  void TestClean();

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
  CPPUNIT_ASSERT(js->GetAllJobs().empty());

  // One and only one JobController should be loaded.
  CPPUNIT_ASSERT_EQUAL(1, (int)js->GetJobControllers().size());

  jobs = js->GetJobs();

  // JobController should contain 2 jobs.
  CPPUNIT_ASSERT_EQUAL(2, (int)jobs.size());

  CPPUNIT_ASSERT_EQUAL(id1.str(), jobs.front().IDFromEndpoint.str());
  CPPUNIT_ASSERT_EQUAL(id2.str(), jobs.back().IDFromEndpoint.str());

  delete js;
}

void JobSupervisorTest::TestAddJob()
{
  js = new Arc::JobSupervisor(usercfg, std::list<Arc::Job>());
  CPPUNIT_ASSERT(js->GetAllJobs().empty());

  j.IDFromEndpoint = Arc::URL("http://test.nordugrid.org/1234567890test1");
  CPPUNIT_ASSERT(js->AddJob(j));
  CPPUNIT_ASSERT(!js->GetAllJobs().empty());

  j.Flavour = "";
  CPPUNIT_ASSERT(!js->AddJob(j));
  CPPUNIT_ASSERT_EQUAL(1, (int)js->GetAllJobs().size());

  j.Flavour = "NON-EXISTENT";
  CPPUNIT_ASSERT(!js->AddJob(j));
  CPPUNIT_ASSERT_EQUAL(1, (int)js->GetAllJobs().size());

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

  std::list<Arc::ExecutionTarget> targets(1, Arc::ExecutionTarget());
  targets.back().url = Arc::URL("http://test2.nordugrid.org");
  targets.back().GridFlavour = "TEST";
  targets.back().HealthState = "ok";

  std::list<Arc::JobDescription> jobdescs(1, Arc::JobDescription());

  bool TargetSortingDone = true;
  Arc::BrokerTestACCControl::TargetSortingDoneSortTargets = &TargetSortingDone;
  Arc::TargetRetrieverTestACCControl::foundTargets = &targets;
  Arc::JobDescriptionParserTestACCControl::parseStatus = true;
  Arc::JobDescriptionParserTestACCControl::parsedJobDescriptions = &jobdescs;
  Arc::SubmitterTestACCControl::submitStatus = true;

  js = new Arc::JobSupervisor(usercfg, jobs);

  std::list<Arc::Job> resubmitted;
  CPPUNIT_ASSERT(js->Resubmit(0, resubmitted));
  CPPUNIT_ASSERT_EQUAL(2, (int)resubmitted.size());

  delete js;
}

void JobSupervisorTest::TestCancel()
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

  st = Arc::JobState::FINISHED;
  j.State = JobStateTEST("F");
  j.IDFromEndpoint = id2;
  jobs.push_back(j);

  st = Arc::JobState::UNDEFINED;
  j.State = JobStateTEST("U");
  j.IDFromEndpoint = id3;
  jobs.push_back(j);

  js = new Arc::JobSupervisor(usercfg, jobs);

  Arc::JobControllerTestACCControl::cancelStatus = true;

  CPPUNIT_ASSERT(js->Cancel());

  CPPUNIT_ASSERT_EQUAL(1, (int)js->GetIDsProcessed().size());
  CPPUNIT_ASSERT_EQUAL(id1.str(), js->GetIDsProcessed().front().str());

  CPPUNIT_ASSERT_EQUAL(2, (int)js->GetIDsNotProcessed().size());
  CPPUNIT_ASSERT_EQUAL(id2.str(), js->GetIDsNotProcessed().front().str());
  CPPUNIT_ASSERT_EQUAL(id3.str(), js->GetIDsNotProcessed().back().str());
  js->ClearSelection();

  Arc::JobControllerTestACCControl::cancelStatus = false;
  CPPUNIT_ASSERT(!js->Cancel());

  CPPUNIT_ASSERT_EQUAL(0, (int)js->GetIDsProcessed().size());

  CPPUNIT_ASSERT_EQUAL(3, (int)js->GetIDsNotProcessed().size());
  CPPUNIT_ASSERT_EQUAL(id1.str(), js->GetIDsNotProcessed().front().str());
  CPPUNIT_ASSERT_EQUAL(id3.str(), js->GetIDsNotProcessed().back().str());
  js->ClearSelection();

  st = Arc::JobState::ACCEPTED;
  j.State = JobStateTEST("A");
  j.IDFromEndpoint = id4;

  CPPUNIT_ASSERT(js->AddJob(j));

  std::list<std::string> status;
  status.push_back("A");


  Arc::JobControllerTestACCControl::cancelStatus = true;

  js->SelectByStatus(status);
  CPPUNIT_ASSERT(js->Cancel());

  CPPUNIT_ASSERT_EQUAL(1, (int)js->GetIDsProcessed().size());
  CPPUNIT_ASSERT_EQUAL(id4.str(), js->GetIDsProcessed().front().str());
  CPPUNIT_ASSERT_EQUAL(0, (int)js->GetIDsNotProcessed().size());
  js->ClearSelection();


  Arc::JobControllerTestACCControl::cancelStatus = false;

  js->SelectByStatus(status);
  CPPUNIT_ASSERT(!js->Cancel());

  CPPUNIT_ASSERT_EQUAL(0, (int)js->GetIDsProcessed().size());

  CPPUNIT_ASSERT_EQUAL(1, (int)js->GetIDsNotProcessed().size());
  CPPUNIT_ASSERT_EQUAL(id4.str(), js->GetIDsNotProcessed().front().str());
  js->ClearSelection();

  delete js;
}

void JobSupervisorTest::TestClean()
{
  std::list<Arc::Job> jobs;
  Arc::URL id1("http://test.nordugrid.org/1234567890test1"),
           id2("http://test.nordugrid.org/1234567890test2");

  st = Arc::JobState::FINISHED;
  j.State = JobStateTEST("F");
  j.IDFromEndpoint = id1;
  jobs.push_back(j);

  st = Arc::JobState::UNDEFINED;
  j.State = JobStateTEST("U");
  j.IDFromEndpoint = id2;
  jobs.push_back(j);

  js = new Arc::JobSupervisor(usercfg, jobs);
  CPPUNIT_ASSERT_EQUAL(2, (int)js->GetAllJobs().size());

  Arc::JobControllerTestACCControl::cleanStatus = true;

  CPPUNIT_ASSERT(js->Clean());

  CPPUNIT_ASSERT_EQUAL(1, (int)js->GetIDsProcessed().size());
  CPPUNIT_ASSERT_EQUAL(id1.str(), js->GetIDsProcessed().front().str());
  CPPUNIT_ASSERT_EQUAL(1, (int)js->GetIDsNotProcessed().size());
  CPPUNIT_ASSERT_EQUAL(id2.str(), js->GetIDsNotProcessed().back().str());
  js->ClearSelection();


  Arc::JobControllerTestACCControl::cleanStatus = false;
  CPPUNIT_ASSERT(!js->Clean());

  CPPUNIT_ASSERT_EQUAL(0, (int)js->GetIDsProcessed().size());
  CPPUNIT_ASSERT_EQUAL(2, (int)js->GetIDsNotProcessed().size());
  CPPUNIT_ASSERT_EQUAL(id1.str(), js->GetIDsNotProcessed().front().str());
  CPPUNIT_ASSERT_EQUAL(id2.str(), js->GetIDsNotProcessed().back().str());
  js->ClearSelection();

  std::list<std::string> status;
  status.push_back("F");


  Arc::JobControllerTestACCControl::cleanStatus = true;

  js->SelectByStatus(status);
  CPPUNIT_ASSERT(js->Clean());

  CPPUNIT_ASSERT_EQUAL(1, (int)js->GetIDsProcessed().size());
  CPPUNIT_ASSERT_EQUAL(id1.str(), js->GetIDsProcessed().front().str());
  CPPUNIT_ASSERT_EQUAL(0, (int)js->GetIDsNotProcessed().size());
  js->ClearSelection();


  Arc::JobControllerTestACCControl::cleanStatus = false;

  js->SelectByStatus(status);
  CPPUNIT_ASSERT(!js->Clean());

  CPPUNIT_ASSERT_EQUAL(0, (int)js->GetIDsProcessed().size());
  CPPUNIT_ASSERT_EQUAL(1, (int)js->GetIDsNotProcessed().size());
  CPPUNIT_ASSERT_EQUAL(id1.str(), js->GetIDsNotProcessed().front().str());

  delete js;
}

CPPUNIT_TEST_SUITE_REGISTRATION(JobSupervisorTest);
