#include <cppunit/extensions/HelperMacros.h>

#include <stdlib.h>

#include <arc/URL.h>
#include <arc/UserConfig.h>
#include <arc/Utils.h>
#include <arc/client/Endpoint.h>
#include <arc/client/Job.h>
#include <arc/client/JobSupervisor.h>

#include <arc/client/TestACCControl.h>

class JobSupervisorTest
  : public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(JobSupervisorTest);
  CPPUNIT_TEST(TestConstructor);
  CPPUNIT_TEST(TestAddJob);
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

private:
  Arc::UserConfig usercfg;
  Arc::JobSupervisor *js;
  Arc::Job j;
};


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
  CPPUNIT_ASSERT(!js->GetAllJobs().empty());

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

  j.State = Arc::JobStateTEST(Arc::JobState::FAILED);
  j.IDFromEndpoint = id1;
  j.JobDescriptionDocument = "CONTENT";
  jobs.push_back(j);

  j.State = Arc::JobStateTEST(Arc::JobState::RUNNING);
  j.IDFromEndpoint = id1;
  j.JobDescriptionDocument = "CONTENT";
  jobs.push_back(j);

  usercfg.Broker("TEST");

  Arc::ComputingServiceType cs;

  Arc::ComputingEndpointType ce;
  ce->URLString = "http://test2.nordugrid.org";
  ce->InterfaceName = "org.nordugrid.test";
  ce->HealthState = "ok";

  cs.ComputingEndpoint.insert(std::pair<int, Arc::ComputingEndpointType>(0, ce));
  cs.ComputingShare.insert(std::pair<int, Arc::ComputingShareType>(0, Arc::ComputingShareType()));
  Arc::ComputingManagerType cm;
  cm.ExecutionEnvironment.insert(std::pair<int, Arc::ExecutionEnvironmentType>(0, Arc::ExecutionEnvironmentType()));
  cs.ComputingManager.insert(std::pair<int, Arc::ComputingManagerType>(0, cm));

  Arc::TargetInformationRetrieverPluginTESTControl::targets.push_back(cs);
  Arc::TargetInformationRetrieverPluginTESTControl::status = Arc::EndpointQueryingStatus::SUCCESSFUL;

  js = new Arc::JobSupervisor(usercfg, jobs);

  std::list<Arc::Endpoint> services(1, Arc::Endpoint("http://test2.nordugrid.org",  "org.nordugrid.tirtest", std::list<std::string>(1, Arc::Endpoint::GetStringForCapability(Arc::Endpoint::COMPUTINGINFO))));
  std::list<Arc::Job> resubmitted;
  CPPUNIT_ASSERT(js->Resubmit(0, services, resubmitted));
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

  j.State = Arc::JobStateTEST(Arc::JobState::RUNNING);
  j.IDFromEndpoint = id1;
  jobs.push_back(j);

  j.State = Arc::JobStateTEST(Arc::JobState::FINISHED);
  j.IDFromEndpoint = id2;
  jobs.push_back(j);

  j.State = Arc::JobStateTEST(Arc::JobState::UNDEFINED);
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

  j.State = Arc::JobStateTEST(Arc::JobState::ACCEPTED, "Accepted");
  j.IDFromEndpoint = id4;

  CPPUNIT_ASSERT(js->AddJob(j));

  std::list<std::string> status;
  status.push_back("Accepted");


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

  j.State = Arc::JobStateTEST(Arc::JobState::FINISHED, "Finished");
  j.IDFromEndpoint = id1;
  jobs.push_back(j);

  j.State = Arc::JobStateTEST(Arc::JobState::UNDEFINED);
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
  status.push_back("Finished");


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
