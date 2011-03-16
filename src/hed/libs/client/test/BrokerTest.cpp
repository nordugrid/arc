#include <iostream>

#include <cppunit/extensions/HelperMacros.h>

#include <arc/UserConfig.h>
#include <arc/Utils.h>
#include <arc/client/Broker.h>
#include <arc/client/ExecutionTarget.h>
#include <arc/client/JobDescription.h>

#include "TestACCControl.h"

#define CPPASSERT(n)\
  b->PreFilterTargets(etl, job);\
  CPPUNIT_ASSERT_EQUAL(n, (int)BrokerTestACCControl::PossibleTargets->size());


class BrokerTest
  : public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(BrokerTest);
  CPPUNIT_TEST(LoadTest);
  CPPUNIT_TEST(QueueTest);
  CPPUNIT_TEST(CPUWallTimeTest);
  CPPUNIT_TEST(BenckmarkCPUWallTimeTest);
  CPPUNIT_TEST(RegisterJobsubmissionTest);
  CPPUNIT_TEST(RegresssionTestMultipleDifferentJobDescriptions);
  CPPUNIT_TEST(RejectTargetsTest);
  CPPUNIT_TEST_SUITE_END();

public:
  BrokerTest();
  ~BrokerTest() { delete bl; }

  void setUp();
  void tearDown();

  void LoadTest();
  void QueueTest();
  void CPUWallTimeTest();
  void BenckmarkCPUWallTimeTest();
  void RegisterJobsubmissionTest();
  void RegresssionTestMultipleDifferentJobDescriptions();
  void RejectTargetsTest();

private:
  const Arc::UserConfig usercfg;
  Arc::BrokerLoader *bl;
  Arc::Broker *b;
  std::list<Arc::ExecutionTarget> etl;
  Arc::JobDescription job;
};

BrokerTest::BrokerTest()
  : usercfg(Arc::initializeCredentialsType(Arc::initializeCredentialsType::SkipCredentials)),
    etl(1, Arc::ExecutionTarget()) {
  Arc::SetEnv("ARC_PLUGIN_PATH", ".libs");

  bl = new Arc::BrokerLoader();
}

void BrokerTest::setUp() {
  etl.front().url = Arc::URL("http://localhost/test");
  etl.front().HealthState = "ok";
}

void BrokerTest::tearDown() {}

void BrokerTest::LoadTest() {
  b = bl->load("", usercfg);
  CPPUNIT_ASSERT(b == NULL);

  b = bl->load("NON-EXISTENT", usercfg);
  CPPUNIT_ASSERT(b == NULL);

  b = bl->load("TEST", usercfg);
  CPPUNIT_ASSERT(b != NULL);
}

void BrokerTest::QueueTest() {
  b = bl->load("TEST", usercfg);
  CPPUNIT_ASSERT(b != NULL);
  CPPUNIT_ASSERT(BrokerTestACCControl::PossibleTargets != NULL);

  job.Resources.QueueName = "q1"; CPPASSERT(0)
  etl.front().ComputingShareName = "q1"; CPPASSERT(1)
  job.Resources.QueueName = "q2"; CPPASSERT(0)
  job.Resources.QueueName = "";
  job.OtherAttributes["nordugrid:broker;reject_queue"] = "q1";  CPPASSERT(0)
  job.OtherAttributes["nordugrid:broker;reject_queue"] = "q2";  CPPASSERT(1)
  etl.front().ComputingShareName = ""; CPPASSERT(0)
  job.OtherAttributes.erase("nordugrid:broker;reject_queue");
}

void BrokerTest::CPUWallTimeTest() {
  b = bl->load("TEST", usercfg);
  CPPUNIT_ASSERT(b != NULL);
  CPPUNIT_ASSERT(BrokerTestACCControl::PossibleTargets != NULL);

  etl.front().MaxCPUTime = 100;
  job.Resources.TotalCPUTime.range.max = 110; CPPASSERT(0)
  job.Resources.TotalCPUTime.range.max = 100; CPPASSERT(1)
  job.Resources.TotalCPUTime.range.max = 90;  CPPASSERT(1)
  etl.front().MaxCPUTime = -1;
  job.Resources.TotalCPUTime.range.max = -1;

  etl.front().MinCPUTime = 10;
  job.Resources.TotalCPUTime.range.min = 5; CPPASSERT(0)
  job.Resources.TotalCPUTime.range.min = 10; CPPASSERT(1)
  job.Resources.TotalCPUTime.range.min = 15; CPPASSERT(1)
  etl.front().MinCPUTime = -1;
  job.Resources.TotalCPUTime.range.min = -1;

  etl.front().MaxWallTime = 100;
  job.Resources.TotalWallTime.range.max = 110; CPPASSERT(0)
  job.Resources.TotalWallTime.range.max = 100; CPPASSERT(1)
  job.Resources.TotalWallTime.range.max = 90;  CPPASSERT(1)
  etl.front().MaxWallTime = -1;
  job.Resources.TotalWallTime.range.max = -1;

  etl.front().MinWallTime = 10;
  job.Resources.TotalWallTime.range.min = 5;  CPPASSERT(0)
  job.Resources.TotalWallTime.range.min = 10; CPPASSERT(1)
  job.Resources.TotalWallTime.range.min = 15; CPPASSERT(1)
  etl.front().MinWallTime = -1;
  job.Resources.TotalWallTime.range.min = -1;
}

void BrokerTest::BenckmarkCPUWallTimeTest() {
  b = bl->load("TEST", usercfg);
  CPPUNIT_ASSERT(b != NULL);
  CPPUNIT_ASSERT(BrokerTestACCControl::PossibleTargets != NULL);

  etl.front().Benchmarks["TestBenchmark"] = 100.;

  job.Resources.TotalCPUTime.benchmark = std::pair<std::string, double>("TestBenchmark", 50.);
  etl.front().MaxCPUTime = 100;
  job.Resources.TotalCPUTime.range.max = 210; CPPASSERT(0)
  job.Resources.TotalCPUTime.range.max = 200; CPPASSERT(1)
  job.Resources.TotalCPUTime.range.max = 190;  CPPASSERT(1)
  etl.front().MaxCPUTime = -1;
  job.Resources.TotalCPUTime.range.max = -1;

  etl.front().MinCPUTime = 10;
  job.Resources.TotalCPUTime.range.min = 10; CPPASSERT(0)
  job.Resources.TotalCPUTime.range.min = 20; CPPASSERT(1)
  job.Resources.TotalCPUTime.range.min = 30; CPPASSERT(1)
  etl.front().MinCPUTime = -1;
  job.Resources.TotalCPUTime.range.min = -1;
  job.Resources.TotalCPUTime.benchmark = std::pair<std::string, double>("", -1.);

  job.Resources.TotalWallTime.benchmark = std::pair<std::string, double>("TestBenchmark", 50.);
  etl.front().MaxWallTime = 100;
  job.Resources.TotalWallTime.range.max = 210; CPPASSERT(0)
  job.Resources.TotalWallTime.range.max = 200; CPPASSERT(1)
  job.Resources.TotalWallTime.range.max = 190;  CPPASSERT(1)
  etl.front().MaxWallTime = -1;
  job.Resources.TotalWallTime.range.max = -1;

  etl.front().MinWallTime = 10;
  job.Resources.TotalWallTime.range.min = 10;  CPPASSERT(0)
  job.Resources.TotalWallTime.range.min = 20; CPPASSERT(1)
  job.Resources.TotalWallTime.range.min = 30; CPPASSERT(1)
  etl.front().MinWallTime = -1;
  job.Resources.TotalWallTime.range.min = -1;
  job.Resources.TotalWallTime.benchmark = std::pair<std::string, double>("", -1.);

  etl.front().CPUClockSpeed = 2500;
  job.Resources.TotalCPUTime.benchmark = std::pair<std::string, double>("clock rate", 1000.);
  etl.front().MaxCPUTime = 100;
  job.Resources.TotalCPUTime.range.max = 300; CPPASSERT(0)
  job.Resources.TotalCPUTime.range.max = 250; CPPASSERT(1)
  job.Resources.TotalCPUTime.range.max = 200;  CPPASSERT(1)
  etl.front().CPUClockSpeed = -1;
  etl.front().MaxCPUTime = -1;
  job.Resources.TotalCPUTime.range.max = -1;
  job.Resources.TotalCPUTime.benchmark = std::pair<std::string, double>("", -1.);
}

void BrokerTest::RegisterJobsubmissionTest() {
  b = bl->load("TEST", usercfg);
  CPPUNIT_ASSERT(b != NULL);
  CPPUNIT_ASSERT(BrokerTestACCControl::PossibleTargets != NULL);
  CPPUNIT_ASSERT(BrokerTestACCControl::TargetSortingDone != NULL);

  job.Resources.SlotRequirement.NumberOfSlots = 4;
  etl.front().TotalSlots = 100;
  etl.front().MaxSlotsPerJob = 5;
  etl.front().FreeSlots = 7;
  etl.front().UsedSlots = 10;
  etl.front().WaitingJobs = 0;

  b->PreFilterTargets(etl, job);
  b->GetBestTarget();
  *BrokerTestACCControl::TargetSortingDone = true;
  b->RegisterJobsubmission();

  CPPUNIT_ASSERT_EQUAL(1, (int)BrokerTestACCControl::PossibleTargets->size());
  CPPUNIT_ASSERT_EQUAL(3, BrokerTestACCControl::PossibleTargets->front()->FreeSlots);
  CPPUNIT_ASSERT_EQUAL(14, BrokerTestACCControl::PossibleTargets->front()->UsedSlots);
  CPPUNIT_ASSERT_EQUAL(0, BrokerTestACCControl::PossibleTargets->front()->WaitingJobs);

  b->RegisterJobsubmission();

  CPPUNIT_ASSERT_EQUAL(1, (int)BrokerTestACCControl::PossibleTargets->size());
  CPPUNIT_ASSERT_EQUAL(3, BrokerTestACCControl::PossibleTargets->front()->FreeSlots);
  CPPUNIT_ASSERT_EQUAL(14, BrokerTestACCControl::PossibleTargets->front()->UsedSlots);
  CPPUNIT_ASSERT_EQUAL(4, BrokerTestACCControl::PossibleTargets->front()->WaitingJobs);
}

void BrokerTest::RegresssionTestMultipleDifferentJobDescriptions() {
  b = bl->load("TEST", usercfg);
  CPPUNIT_ASSERT(b != NULL);
  CPPUNIT_ASSERT(BrokerTestACCControl::PossibleTargets != NULL);

  /* When prefiltered by the broker, each JobDescription object "correspond" to
   * a (list of) ExecutionTarget object(s).
   */

  Arc::JobDescription frontJD, backJD;
  frontJD.Resources.QueueName = "front";
  backJD.Resources.QueueName = "back";

  std::list<Arc::ExecutionTarget> targets;
  targets.push_back(Arc::ExecutionTarget());
  targets.push_back(Arc::ExecutionTarget());
  targets.front().url = Arc::URL("http://localhost/test");
  targets.front().HealthState = "ok";
  targets.back().url = Arc::URL("http://localhost/test");
  targets.back().HealthState = "ok";

  targets.front().ComputingShareName = "front";
  targets.back().ComputingShareName = "back";

  b->PreFilterTargets(targets, frontJD);
  CPPUNIT_ASSERT_EQUAL(1, (int)BrokerTestACCControl::PossibleTargets->size());
  CPPUNIT_ASSERT_EQUAL((std::string)"front", BrokerTestACCControl::PossibleTargets->front()->ComputingShareName);
  b->PreFilterTargets(targets, backJD);
  CPPUNIT_ASSERT_EQUAL(1, (int)BrokerTestACCControl::PossibleTargets->size());
  CPPUNIT_ASSERT_EQUAL((std::string)"back", BrokerTestACCControl::PossibleTargets->front()->ComputingShareName);
}

void BrokerTest::RejectTargetsTest() {
  b = bl->load("TEST", usercfg);
  CPPUNIT_ASSERT(b != NULL);
  CPPUNIT_ASSERT(BrokerTestACCControl::PossibleTargets != NULL);

  Arc::JobDescription j;
  j.Application.Executable.Name = "executable";

  std::list<Arc::ExecutionTarget> targets;
  targets.push_back(Arc::ExecutionTarget());
  targets.push_back(Arc::ExecutionTarget());
  targets.front().url = Arc::URL("http://localhost/test1");
  targets.front().HealthState = "ok";
  targets.back().url = Arc::URL("http://localhost/test2");
  targets.back().HealthState = "ok";

  // Rejecting no targets.
  b->PreFilterTargets(targets, j);
  CPPUNIT_ASSERT_EQUAL(2, (int)BrokerTestACCControl::PossibleTargets->size());

  // Reject test1 target.
  std::list<Arc::URL> rejectTargets;
  rejectTargets.push_back(targets.front().url);
  b->PreFilterTargets(targets, j, rejectTargets);
  CPPUNIT_ASSERT_EQUAL(1, (int)BrokerTestACCControl::PossibleTargets->size());
  CPPUNIT_ASSERT_EQUAL(targets.back().url, BrokerTestACCControl::PossibleTargets->front()->url);

  // Reject both targets.
  rejectTargets.push_back(targets.back().url);
  b->PreFilterTargets(targets, j, rejectTargets);
  CPPUNIT_ASSERT_EQUAL(0, (int)BrokerTestACCControl::PossibleTargets->size());
}

CPPUNIT_TEST_SUITE_REGISTRATION(BrokerTest);
