#include <iostream>

#include <cppunit/extensions/HelperMacros.h>

#include <arc/UserConfig.h>
#include <arc/client/Broker.h>
#include <arc/client/ExecutionTarget.h>
#include <arc/client/JobDescription.h>

#define CPPASSERT(n)\
  tb.PreFilterTargets(etl, job);\
  CPPUNIT_ASSERT_EQUAL(n, (int)tb.GetTargets().size());\
  tb.clear();


class BrokerTest
  : public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(BrokerTest);
  CPPUNIT_TEST(QueueTest);
  CPPUNIT_TEST(CPUWallTimeTest);
  CPPUNIT_TEST(BenckmarkCPUWallTimeTest);
  CPPUNIT_TEST(RegisterJobsubmissionTest);
  CPPUNIT_TEST(RegresssionTestMultipleDifferentJobDescriptions);
  CPPUNIT_TEST_SUITE_END();

public:
  BrokerTest();
  void setUp();
  void tearDown();
  void QueueTest();
  void CPUWallTimeTest();
  void BenckmarkCPUWallTimeTest();
  void RegisterJobsubmissionTest();
  void RegresssionTestMultipleDifferentJobDescriptions();

  class TestBroker
    : public Arc::Broker {
  public:
    TestBroker(const Arc::UserConfig& usercfg) : Arc::Broker(usercfg) {}
    void SortTargets() { TargetSortingDone = true; }
    void clear() { PossibleTargets.clear(); }
    const std::list<Arc::ExecutionTarget*>& GetTargets() { return PossibleTargets; }
  };

private:
  const Arc::UserConfig usercfg;
  TestBroker tb;
  std::list<Arc::ExecutionTarget> etl;
  Arc::JobDescription job;
};

BrokerTest::BrokerTest()
  : usercfg(Arc::initializeCredentialsType(Arc::initializeCredentialsType::SkipCredentials)),
    tb(usercfg),
    etl(1, Arc::ExecutionTarget()) {}

void BrokerTest::setUp() {
  etl.front().url = Arc::URL("http://localhost/test");
  etl.front().HealthState = "ok";
}

void BrokerTest::tearDown() {}

void BrokerTest::QueueTest() {
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
  job.Resources.SlotRequirement.NumberOfSlots = 4;
  etl.front().TotalSlots = 100;
  etl.front().MaxSlotsPerJob = 5;
  etl.front().FreeSlots = 7;
  etl.front().UsedSlots = 10;
  etl.front().WaitingJobs = 0;

  tb.PreFilterTargets(etl, job);
  tb.GetBestTarget();
  tb.RegisterJobsubmission();

  CPPUNIT_ASSERT_EQUAL(1, (int)tb.GetTargets().size());
  CPPUNIT_ASSERT_EQUAL(3, tb.GetTargets().front()->FreeSlots);
  CPPUNIT_ASSERT_EQUAL(14, tb.GetTargets().front()->UsedSlots);
  CPPUNIT_ASSERT_EQUAL(0, tb.GetTargets().front()->WaitingJobs);

  tb.RegisterJobsubmission();

  CPPUNIT_ASSERT_EQUAL(1, (int)tb.GetTargets().size());
  CPPUNIT_ASSERT_EQUAL(3, tb.GetTargets().front()->FreeSlots);
  CPPUNIT_ASSERT_EQUAL(14, tb.GetTargets().front()->UsedSlots);
  CPPUNIT_ASSERT_EQUAL(4, tb.GetTargets().front()->WaitingJobs);
}

void BrokerTest::RegresssionTestMultipleDifferentJobDescriptions() {
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

  tb.PreFilterTargets(targets, frontJD);
  CPPUNIT_ASSERT_EQUAL(1, (int)tb.GetTargets().size());
  CPPUNIT_ASSERT_EQUAL((std::string)"front", tb.GetTargets().front()->ComputingShareName);
  tb.PreFilterTargets(targets, backJD);
  CPPUNIT_ASSERT_EQUAL(1, (int)tb.GetTargets().size());
  CPPUNIT_ASSERT_EQUAL((std::string)"back", tb.GetTargets().front()->ComputingShareName);
}

CPPUNIT_TEST_SUITE_REGISTRATION(BrokerTest);
