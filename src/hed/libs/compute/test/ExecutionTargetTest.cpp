#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>

#include <cppunit/extensions/HelperMacros.h>

#include <arc/compute/ExecutionTarget.h>
#include <arc/compute/JobDescription.h>
#include <arc/Thread.h>

static Arc::Logger testLogger(Arc::Logger::getRootLogger(), "ExecutionTargetTest");

class ExecutionTargetTest
  : public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(ExecutionTargetTest);
  CPPUNIT_TEST(RegisterJobsubmissionTest);
  CPPUNIT_TEST_SUITE_END();

public:
  ExecutionTargetTest() {}

  void setUp();
  void tearDown() {}

  void RegisterJobsubmissionTest();

private:
  Arc::ExecutionTarget et;
  Arc::JobDescription job;
};

void ExecutionTargetTest::setUp() {
  et.ComputingEndpoint->URLString = "http://localhost/test";
  et.ComputingEndpoint->HealthState = "ok";
}

void ExecutionTargetTest::RegisterJobsubmissionTest() {
  job.Resources.SlotRequirement.NumberOfSlots = 4;
  et.ComputingManager->TotalSlots = 100;
  et.ComputingShare->MaxSlotsPerJob = 5;
  et.ComputingShare->FreeSlots = 7;
  et.ComputingShare->UsedSlots = 10;
  et.ComputingShare->WaitingJobs = 0;

  et.RegisterJobSubmission(job);

  CPPUNIT_ASSERT_EQUAL(3, et.ComputingShare->FreeSlots);
  CPPUNIT_ASSERT_EQUAL(14, et.ComputingShare->UsedSlots);
  CPPUNIT_ASSERT_EQUAL(0, et.ComputingShare->WaitingJobs);

  et.RegisterJobSubmission(job);

  CPPUNIT_ASSERT_EQUAL(3, et.ComputingShare->FreeSlots);
  CPPUNIT_ASSERT_EQUAL(14, et.ComputingShare->UsedSlots);
  CPPUNIT_ASSERT_EQUAL(4, et.ComputingShare->WaitingJobs);
}

CPPUNIT_TEST_SUITE_REGISTRATION(ExecutionTargetTest);
