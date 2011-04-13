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
  CPPUNIT_TEST_SUITE_END();

public:
  JobSupervisorTest();
  ~JobSupervisorTest() { delete js; }

  void setUp() {}
  void tearDown() {}

  void TestConstructor();
  void TestAddJob();

private:
  Arc::UserConfig usercfg;
  Arc::JobSupervisor *js;
  Arc::Job j;
};

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

CPPUNIT_TEST_SUITE_REGISTRATION(JobSupervisorTest);
