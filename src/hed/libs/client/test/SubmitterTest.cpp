#include <cppunit/extensions/HelperMacros.h>

#include <stdlib.h>

#include <arc/client/Submitter.h>
#include <arc/Thread.h>

#include <arc/client/TestACCControl.h>


class SubmitterTest : public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(SubmitterTest);
  CPPUNIT_TEST(SubmissionToExecutionTargetTest);
  CPPUNIT_TEST_SUITE_END();

public:
  SubmitterTest();
  
  void setUp() {}
  void tearDown() { Arc::ThreadInitializer().waitExit(); }
  
  void SubmissionToExecutionTargetTest();
  
private:
  Arc::UserConfig usercfg;
};

SubmitterTest::SubmitterTest() : usercfg(Arc::initializeCredentialsType(Arc::initializeCredentialsType::SkipCredentials)) {}

void SubmitterTest::SubmissionToExecutionTargetTest() {
  
  Arc::LogStream logcerr(std::cerr);
  logcerr.setFormat(Arc::ShortFormat);
  Arc::Logger::getRootLogger().addDestination(logcerr);
  Arc::Logger::getRootLogger().setThreshold(Arc::DEBUG);
  
  Arc::Submitter submitter(usercfg);
  // Prepare to job which will be returned by the test ACC
  Arc::Job testJob;
  testJob.JobID = Arc::URL("http://test.nordugrid.org/testjob");
  Arc::SubmitterPluginTestACCControl::submitJob = testJob;
  
  Arc::JobDescription desc;
  Arc::ExecutionTarget et;
  et.ComputingEndpoint->InterfaceName = "org.nordugrid.test";
  
  Arc::Job job;
  submitter.Submit(et, desc, job);
  
  CPPUNIT_ASSERT(testJob == job);
}

CPPUNIT_TEST_SUITE_REGISTRATION(SubmitterTest);
