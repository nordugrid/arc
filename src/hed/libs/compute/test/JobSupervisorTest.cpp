#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cppunit/extensions/HelperMacros.h>

#include <stdlib.h>

#include <arc/URL.h>
#include <arc/UserConfig.h>
#include <arc/Utils.h>
#include <arc/compute/Endpoint.h>
#include <arc/compute/Job.h>
#include <arc/compute/JobSupervisor.h>
#include <arc/Thread.h>

#include <arc/compute/TestACCControl.h>

class JobSupervisorTest
  : public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(JobSupervisorTest);
  CPPUNIT_TEST(TestConstructor);
  CPPUNIT_TEST(TestAddJob);
  CPPUNIT_TEST(TestCancel);
  CPPUNIT_TEST(TestClean);
  CPPUNIT_TEST(TestSelector);
  CPPUNIT_TEST_SUITE_END();

public:
  JobSupervisorTest();
  ~JobSupervisorTest() {}

  void setUp() {}
  void tearDown() { Arc::ThreadInitializer().waitExit(); }

  void TestConstructor();
  void TestAddJob();
  void TestCancel();
  void TestClean();
  void TestSelector();

private:
  Arc::UserConfig usercfg;
  Arc::JobSupervisor *js;
  Arc::Job j;
};

class ThreeDaysOldJobSelector : public Arc::JobSelector {
public:
  ThreeDaysOldJobSelector() {
    now = Arc::Time();
    three_days = Arc::Period(60*60*24*3);
  }

  bool Select(const Arc::Job& job) const {
    return (now - job.EndTime) >= three_days;
  }

private:
  Arc::Time now;
  Arc::Period three_days;
};

JobSupervisorTest::JobSupervisorTest() : usercfg(Arc::initializeCredentialsType(Arc::initializeCredentialsType::SkipCredentials)), js(NULL) {
  j.JobStatusURL = Arc::URL("http://test.nordugrid.org");
  j.JobStatusInterfaceName = "org.nordugrid.test";
  j.JobManagementURL = Arc::URL("http://test.nordugrid.org");
  j.JobManagementInterfaceName = "org.nordugrid.test";
}

void JobSupervisorTest::TestConstructor()
{
  std::list<Arc::Job> jobs;
  std::string id1 = "http://test.nordugrid.org/1234567890test1";
  std::string id2 = "http://test.nordugrid.org/1234567890test2";
  j.JobID = id1;
  jobs.push_back(j);
  j.JobID = id2;
  jobs.push_back(j);

  js = new Arc::JobSupervisor(usercfg, jobs);
  CPPUNIT_ASSERT(!js->GetAllJobs().empty());

  jobs = js->GetAllJobs();

  // JobSupervisor should contain 2 jobs.
  CPPUNIT_ASSERT_EQUAL(2, (int)jobs.size());

  CPPUNIT_ASSERT_EQUAL(id1, jobs.front().JobID);
  CPPUNIT_ASSERT_EQUAL(id2, jobs.back().JobID);

  delete js;
}

void JobSupervisorTest::TestAddJob()
{
  js = new Arc::JobSupervisor(usercfg, std::list<Arc::Job>());
  CPPUNIT_ASSERT(js->GetAllJobs().empty());

  j.JobID = "http://test.nordugrid.org/1234567890test1";
  CPPUNIT_ASSERT(js->AddJob(j));
  CPPUNIT_ASSERT(!js->GetAllJobs().empty());

  j.JobManagementInterfaceName = "";
  CPPUNIT_ASSERT(!js->AddJob(j));
  CPPUNIT_ASSERT_EQUAL(1, (int)js->GetAllJobs().size());

  j.JobManagementInterfaceName = "non.existent.interface";
  CPPUNIT_ASSERT(!js->AddJob(j));
  CPPUNIT_ASSERT_EQUAL(1, (int)js->GetAllJobs().size());

  delete js;
}

void JobSupervisorTest::TestCancel()
{
  std::list<Arc::Job> jobs;
  std::string id1 = "http://test.nordugrid.org/1234567890test1";
  std::string id2 = "http://test.nordugrid.org/1234567890test2";
  std::string id3 = "http://test.nordugrid.org/1234567890test3";
  std::string id4 = "http://test.nordugrid.org/1234567890test4";

  j.State = Arc::JobStateTEST(Arc::JobState::RUNNING);
  j.JobID = id1;
  jobs.push_back(j);

  j.State = Arc::JobStateTEST(Arc::JobState::FINISHED);
  j.JobID = id2;
  jobs.push_back(j);

  j.State = Arc::JobStateTEST(Arc::JobState::UNDEFINED);
  j.JobID = id3;
  jobs.push_back(j);

  js = new Arc::JobSupervisor(usercfg, jobs);

  Arc::JobControllerPluginTestACCControl::cancelStatus = true;

  CPPUNIT_ASSERT(js->Cancel());

  CPPUNIT_ASSERT_EQUAL(1, (int)js->GetIDsProcessed().size());
  CPPUNIT_ASSERT_EQUAL(id1, js->GetIDsProcessed().front());

  CPPUNIT_ASSERT_EQUAL(2, (int)js->GetIDsNotProcessed().size());
  CPPUNIT_ASSERT_EQUAL(id2, js->GetIDsNotProcessed().front());
  CPPUNIT_ASSERT_EQUAL(id3, js->GetIDsNotProcessed().back());
  js->ClearSelection();

  Arc::JobControllerPluginTestACCControl::cancelStatus = false;
  CPPUNIT_ASSERT(!js->Cancel());

  CPPUNIT_ASSERT_EQUAL(0, (int)js->GetIDsProcessed().size());

  CPPUNIT_ASSERT_EQUAL(3, (int)js->GetIDsNotProcessed().size());
  CPPUNIT_ASSERT_EQUAL(id1, js->GetIDsNotProcessed().front());
  CPPUNIT_ASSERT_EQUAL(id3, js->GetIDsNotProcessed().back());
  js->ClearSelection();

  j.State = Arc::JobStateTEST(Arc::JobState::ACCEPTED, "Accepted");
  j.JobID = id4;

  CPPUNIT_ASSERT(js->AddJob(j));

  std::list<std::string> status;
  status.push_back("Accepted");


  Arc::JobControllerPluginTestACCControl::cancelStatus = true;

  js->SelectByStatus(status);
  CPPUNIT_ASSERT(js->Cancel());

  CPPUNIT_ASSERT_EQUAL(1, (int)js->GetIDsProcessed().size());
  CPPUNIT_ASSERT_EQUAL(id4, js->GetIDsProcessed().front());
  CPPUNIT_ASSERT_EQUAL(0, (int)js->GetIDsNotProcessed().size());
  js->ClearSelection();


  Arc::JobControllerPluginTestACCControl::cancelStatus = false;

  js->SelectByStatus(status);
  CPPUNIT_ASSERT(!js->Cancel());

  CPPUNIT_ASSERT_EQUAL(0, (int)js->GetIDsProcessed().size());

  CPPUNIT_ASSERT_EQUAL(1, (int)js->GetIDsNotProcessed().size());
  CPPUNIT_ASSERT_EQUAL(id4, js->GetIDsNotProcessed().front());
  js->ClearSelection();

  delete js;
}

void JobSupervisorTest::TestClean()
{
  std::list<Arc::Job> jobs;
  std::string id1 = "http://test.nordugrid.org/1234567890test1";
  std::string id2 = "http://test.nordugrid.org/1234567890test2";

  j.State = Arc::JobStateTEST(Arc::JobState::FINISHED, "Finished");
  j.JobID = id1;
  jobs.push_back(j);

  j.State = Arc::JobStateTEST(Arc::JobState::UNDEFINED);
  j.JobID = id2;
  jobs.push_back(j);

  js = new Arc::JobSupervisor(usercfg, jobs);
  CPPUNIT_ASSERT_EQUAL(2, (int)js->GetAllJobs().size());

  Arc::JobControllerPluginTestACCControl::cleanStatus = true;

  CPPUNIT_ASSERT(js->Clean());

  CPPUNIT_ASSERT_EQUAL(1, (int)js->GetIDsProcessed().size());
  CPPUNIT_ASSERT_EQUAL(id1, js->GetIDsProcessed().front());
  CPPUNIT_ASSERT_EQUAL(1, (int)js->GetIDsNotProcessed().size());
  CPPUNIT_ASSERT_EQUAL(id2, js->GetIDsNotProcessed().back());
  js->ClearSelection();


  Arc::JobControllerPluginTestACCControl::cleanStatus = false;
  CPPUNIT_ASSERT(!js->Clean());

  CPPUNIT_ASSERT_EQUAL(0, (int)js->GetIDsProcessed().size());
  CPPUNIT_ASSERT_EQUAL(2, (int)js->GetIDsNotProcessed().size());
  CPPUNIT_ASSERT_EQUAL(id1, js->GetIDsNotProcessed().front());
  CPPUNIT_ASSERT_EQUAL(id2, js->GetIDsNotProcessed().back());
  js->ClearSelection();

  std::list<std::string> status;
  status.push_back("Finished");


  Arc::JobControllerPluginTestACCControl::cleanStatus = true;

  js->SelectByStatus(status);
  CPPUNIT_ASSERT(js->Clean());

  CPPUNIT_ASSERT_EQUAL(1, (int)js->GetIDsProcessed().size());
  CPPUNIT_ASSERT_EQUAL(id1, js->GetIDsProcessed().front());
  CPPUNIT_ASSERT_EQUAL(0, (int)js->GetIDsNotProcessed().size());
  js->ClearSelection();


  Arc::JobControllerPluginTestACCControl::cleanStatus = false;

  js->SelectByStatus(status);
  CPPUNIT_ASSERT(!js->Clean());

  CPPUNIT_ASSERT_EQUAL(0, (int)js->GetIDsProcessed().size());
  CPPUNIT_ASSERT_EQUAL(1, (int)js->GetIDsNotProcessed().size());
  CPPUNIT_ASSERT_EQUAL(id1, js->GetIDsNotProcessed().front());

  delete js;
}

void JobSupervisorTest::TestSelector()
{
  js = new Arc::JobSupervisor(usercfg);

  j.JobID = "test-job-1-day-old";
  j.EndTime = Arc::Time()-Arc::Period("P1D");
  js->AddJob(j);

  j.JobID = "test-job-2-days-old";
  j.EndTime = Arc::Time()-Arc::Period("P2D");
  js->AddJob(j);

  j.JobID = "test-job-3-days-old";
  j.EndTime = Arc::Time()-Arc::Period("P3D");
  js->AddJob(j);

  j.JobID = "test-job-4-days-old";
  j.EndTime = Arc::Time()-Arc::Period("P4D");
  js->AddJob(j);

  CPPUNIT_ASSERT_EQUAL(4, (int)js->GetAllJobs().size());

  ThreeDaysOldJobSelector selector;
  js->Select(selector);

  std::list<Arc::Job> selectedJobs = js->GetSelectedJobs();
  CPPUNIT_ASSERT_EQUAL(2, (int)js->GetSelectedJobs().size());


  for (std::list<Arc::Job>::iterator itJ = selectedJobs.begin();
       itJ != selectedJobs.end(); ++itJ) {
    CPPUNIT_ASSERT(itJ->JobID == "test-job-3-days-old" || itJ->JobID == "test-job-4-days-old");
    CPPUNIT_ASSERT(itJ->EndTime <= (Arc::Time()-Arc::Period("P3D")));
  }

  delete js;
}

CPPUNIT_TEST_SUITE_REGISTRATION(JobSupervisorTest);
