#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cppunit/extensions/HelperMacros.h>

#include <stdlib.h>

#include <arc/compute/Submitter.h>
#include <arc/Thread.h>

#include <arc/compute/TestACCControl.h>


class SubmitterTest : public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(SubmitterTest);
  CPPUNIT_TEST(SubmissionToExecutionTargetTest);
  CPPUNIT_TEST(SubmissionToExecutionTargetWithConsumerTest);
  CPPUNIT_TEST_SUITE_END();

public:
  SubmitterTest();
  
  void setUp() {}
  void tearDown() { Arc::ThreadInitializer().waitExit(); }
  
  void SubmissionToExecutionTargetTest();
  void SubmissionToExecutionTargetWithConsumerTest();
  
private:
  Arc::UserConfig usercfg;
};

SubmitterTest::SubmitterTest() : usercfg(Arc::initializeCredentialsType(Arc::initializeCredentialsType::SkipCredentials)) {}

void SubmitterTest::SubmissionToExecutionTargetTest() {
    
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
  
  CPPUNIT_ASSERT(job == testJob);
}

void SubmitterTest::SubmissionToExecutionTargetWithConsumerTest() {
    
  Arc::Submitter submitter(usercfg);
  // Prepare to job which will be returned by the test ACC
  Arc::Job testJob;
  testJob.JobID = Arc::URL("http://test.nordugrid.org/testjob");
  Arc::SubmitterPluginTestACCControl::submitJob = testJob;
  
  Arc::JobDescription desc;
  Arc::ExecutionTarget et;
  et.ComputingEndpoint->InterfaceName = "org.nordugrid.test";
  
  Arc::EntityContainer<Arc::Job> container;
  submitter.addConsumer(container);
  submitter.Submit(et, desc);
  
  CPPUNIT_ASSERT_EQUAL(1, (int)container.size());
  CPPUNIT_ASSERT(container.front() == testJob);
}


CPPUNIT_TEST_SUITE_REGISTRATION(SubmitterTest);
