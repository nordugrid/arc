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
  CPPUNIT_TEST(TestResubmit);
  CPPUNIT_TEST(TestCancel);
  CPPUNIT_TEST(TestClean);
  CPPUNIT_TEST_SUITE_END();

public:
  JobSupervisorTest();
  ~JobSupervisorTest() {}

  void setUp() {}
  void tearDown() { Arc::ThreadInitializer().waitExit(); }

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

void JobSupervisorTest::TestResubmit()
{
  std::list<Arc::Job> jobs;
  Arc::URL id1("http://test.nordugrid.org/1234567890test1"),
           id2("http://test.nordugrid.org/1234567890test2"),
           id3("http://test.nordugrid.org/1234567890test3");

  j.State = Arc::JobStateTEST(Arc::JobState::FAILED);
  j.JobID = id1;
  j.JobDescriptionDocument = "CONTENT";
  jobs.push_back(j);

  j.State = Arc::JobStateTEST(Arc::JobState::RUNNING);
  j.JobID = id1;
  j.JobDescriptionDocument = "CONTENT";
  jobs.push_back(j);

  usercfg.Broker("TEST");

  Arc::ComputingServiceType cs;

  Arc::ComputingEndpointType ce;
  ce->URLString = "http://test2.nordugrid.org";
  ce->InterfaceName = "org.nordugrid.test";
  ce->Capability.insert(Arc::Endpoint::GetStringForCapability(Arc::Endpoint::JOBSUBMIT));
  ce->HealthState = "ok";

  cs.ComputingEndpoint.insert(std::pair<int, Arc::ComputingEndpointType>(0, ce));
  cs.ComputingShare.insert(std::pair<int, Arc::ComputingShareType>(0, Arc::ComputingShareType()));
  Arc::ComputingManagerType cm;
  cm.ExecutionEnvironment.insert(std::pair<int, Arc::ExecutionEnvironmentType>(0, Arc::ExecutionEnvironmentType()));
  cs.ComputingManager.insert(std::pair<int, Arc::ComputingManagerType>(0, cm));

  Arc::TargetInformationRetrieverPluginTESTControl::targets.push_back(cs);
  Arc::TargetInformationRetrieverPluginTESTControl::status = Arc::EndpointQueryingStatus::SUCCESSFUL;

  Arc::BrokerPluginTestACCControl::match = true;

  js = new Arc::JobSupervisor(usercfg, jobs);

  std::list<Arc::Endpoint> services(1, Arc::Endpoint("http://test2.nordugrid.org",  Arc::Endpoint::COMPUTINGINFO, "org.nordugrid.tirtest"));
  std::list<Arc::Job> resubmitted;
  CPPUNIT_ASSERT(js->Resubmit(0, services, resubmitted));
  CPPUNIT_ASSERT_EQUAL(2, (int)resubmitted.size());

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

CPPUNIT_TEST_SUITE_REGISTRATION(JobSupervisorTest);
