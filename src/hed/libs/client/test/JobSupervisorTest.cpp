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
  CPPUNIT_TEST(TestCancel);
  CPPUNIT_TEST(TestClean);
  CPPUNIT_TEST_SUITE_END();

public:
  JobSupervisorTest();
  ~JobSupervisorTest() { delete js; }

  void setUp() {}
  void tearDown() {}

  void TestConstructor();
  void TestAddJob();
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

void JobSupervisorTest::TestCancel()
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

  std::list<Arc::URL> notCanceled;
  std::list<Arc::URL> canceled = js->Cancel(jobsToBeCanceled, notCanceled);

  CPPUNIT_ASSERT_EQUAL(2, (int)canceled.size());
  CPPUNIT_ASSERT_EQUAL(id1.str(), canceled.front().str());
  CPPUNIT_ASSERT_EQUAL(id2.str(), canceled.back().str());

  CPPUNIT_ASSERT_EQUAL(1, (int)notCanceled.size());
  CPPUNIT_ASSERT_EQUAL(id3.str(), notCanceled.back().str());


  notCanceled.clear();
  JobControllerTestACCControl::cancelStatus = false;
  canceled = js->Cancel(jobsToBeCanceled, notCanceled);

  CPPUNIT_ASSERT_EQUAL(1, (int)canceled.size());
  CPPUNIT_ASSERT_EQUAL(id2.str(), canceled.back().str());

  CPPUNIT_ASSERT_EQUAL(2, (int)notCanceled.size());
  CPPUNIT_ASSERT_EQUAL(id1.str(), notCanceled.front().str());
  CPPUNIT_ASSERT_EQUAL(id3.str(), notCanceled.back().str());

  delete js;
}

void JobSupervisorTest::TestClean()
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

  std::list<Arc::URL> notCleaned;
  std::list<Arc::URL> cleaned = js->Clean(jobsToBeCleaned, notCleaned);

  CPPUNIT_ASSERT_EQUAL(1, (int)cleaned.size());
  CPPUNIT_ASSERT_EQUAL(id1.str(), cleaned.front().str());

  CPPUNIT_ASSERT_EQUAL(1, (int)notCleaned.size());
  CPPUNIT_ASSERT_EQUAL(id2.str(), notCleaned.back().str());


  notCleaned.clear();
  JobControllerTestACCControl::cleanStatus = false;
  cleaned = js->Clean(jobsToBeCleaned, notCleaned);

  CPPUNIT_ASSERT_EQUAL(0, (int)cleaned.size());

  CPPUNIT_ASSERT_EQUAL(2, (int)notCleaned.size());
  CPPUNIT_ASSERT_EQUAL(id1.str(), notCleaned.front().str());
  CPPUNIT_ASSERT_EQUAL(id2.str(), notCleaned.back().str());

  delete js;
}

CPPUNIT_TEST_SUITE_REGISTRATION(JobSupervisorTest);
