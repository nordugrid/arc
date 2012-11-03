#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>

#include <cppunit/extensions/HelperMacros.h>

#include <arc/UserConfig.h>
#include <arc/Utils.h>
#include <arc/compute/Broker.h>
#include <arc/compute/ExecutionTarget.h>
#include <arc/compute/JobDescription.h>
#include <arc/Thread.h>

#include <arc/compute/TestACCControl.h>

static Arc::Logger testLogger(Arc::Logger::getRootLogger(), "BrokerTest");

class BrokerTest
  : public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(BrokerTest);
  CPPUNIT_TEST(LoadTest);
  CPPUNIT_TEST(QueueTest);
  CPPUNIT_TEST(CPUWallTimeTest);
  CPPUNIT_TEST(BenckmarkCPUWallTimeTest);
  CPPUNIT_TEST(RegresssionTestMultipleDifferentJobDescriptions);
  CPPUNIT_TEST(RejectTargetsTest);
  CPPUNIT_TEST_SUITE_END();

public:
  BrokerTest();

  void setUp();
  void tearDown();

  void LoadTest();
  void QueueTest();
  void CPUWallTimeTest();
  void BenckmarkCPUWallTimeTest();
  void RegresssionTestMultipleDifferentJobDescriptions();
  void RejectTargetsTest();

private:
  const Arc::UserConfig usercfg;
  std::list<Arc::ExecutionTarget> etl;
  Arc::JobDescription job;
};

BrokerTest::BrokerTest()
  : usercfg(Arc::initializeCredentialsType(Arc::initializeCredentialsType::SkipCredentials)),
    etl(1, Arc::ExecutionTarget()) {
}

void BrokerTest::setUp() {
  Arc::BrokerPluginTestACCControl::match = true;
  etl.front().ComputingEndpoint->URLString = "http://localhost/test";
  etl.front().ComputingEndpoint->HealthState = "ok";
}

void BrokerTest::tearDown() { Arc::ThreadInitializer().waitExit(); }

void BrokerTest::LoadTest() {
  Arc::BrokerPluginLoader bpl;
  Arc::BrokerPlugin *b = bpl.load(usercfg, "NON-EXISTENT");
  CPPUNIT_ASSERT(b == NULL);

  b = bpl.load(usercfg, "TEST");
  CPPUNIT_ASSERT(b != NULL);
}

void BrokerTest::QueueTest() {
  job.Resources.QueueName = "q1";

  Arc::Broker b(usercfg, job, "TEST");
  CPPUNIT_ASSERT(b.isValid());

  CPPUNIT_ASSERT(!b.match(etl.front()));
  etl.front().ComputingShare->Name = "q1";
  CPPUNIT_ASSERT(b.match(etl.front()));
  
  job.Resources.QueueName = "q2";
  CPPUNIT_ASSERT(!b.match(etl.front()));

  job.Resources.QueueName = "";
  job.OtherAttributes["nordugrid:broker;reject_queue"] = "q1";
  CPPUNIT_ASSERT(!b.match(etl.front()));
  job.OtherAttributes["nordugrid:broker;reject_queue"] = "q2";
  CPPUNIT_ASSERT(b.match(etl.front()));
  
  etl.front().ComputingShare->Name = "";
  CPPUNIT_ASSERT(!b.match(etl.front()));
  job.OtherAttributes.erase("nordugrid:broker;reject_queue");
}

void BrokerTest::CPUWallTimeTest() {
  Arc::Broker b(usercfg, job, "TEST");
  CPPUNIT_ASSERT(b.isValid());

  etl.front().ComputingShare->MaxCPUTime = 100;
  job.Resources.IndividualCPUTime.range.max = 110;
  CPPUNIT_ASSERT(!b.match(etl.front()));
  job.Resources.IndividualCPUTime.range.max = 100; 
  CPPUNIT_ASSERT(b.match(etl.front()));
  job.Resources.IndividualCPUTime.range.max = 90;
  CPPUNIT_ASSERT(b.match(etl.front()));
  etl.front().ComputingShare->MaxCPUTime = -1;
  job.Resources.IndividualCPUTime.range.max = -1;

  etl.front().ComputingShare->MinCPUTime = 10;
  job.Resources.IndividualCPUTime.range.min = 5;
  CPPUNIT_ASSERT(!b.match(etl.front()));
  job.Resources.IndividualCPUTime.range.min = 10;
  CPPUNIT_ASSERT(b.match(etl.front()));
  job.Resources.IndividualCPUTime.range.min = 15;
  CPPUNIT_ASSERT(b.match(etl.front()));
  etl.front().ComputingShare->MinCPUTime = -1;
  job.Resources.IndividualCPUTime.range.min = -1;

  etl.front().ComputingShare->MaxWallTime = 100;
  job.Resources.IndividualWallTime.range.max = 110;
  CPPUNIT_ASSERT(!b.match(etl.front()));
  job.Resources.IndividualWallTime.range.max = 100;
  CPPUNIT_ASSERT(b.match(etl.front()));
  job.Resources.IndividualWallTime.range.max = 90;
  CPPUNIT_ASSERT(b.match(etl.front()));
  etl.front().ComputingShare->MaxWallTime = -1;
  job.Resources.IndividualWallTime.range.max = -1;

  etl.front().ComputingShare->MinWallTime = 10;
  job.Resources.IndividualWallTime.range.min = 5;
  CPPUNIT_ASSERT(!b.match(etl.front()));
  job.Resources.IndividualWallTime.range.min = 10;
  CPPUNIT_ASSERT(b.match(etl.front()));
  job.Resources.IndividualWallTime.range.min = 15;
  CPPUNIT_ASSERT(b.match(etl.front()));
  etl.front().ComputingShare->MinWallTime = -1;
  job.Resources.IndividualWallTime.range.min = -1;
}

void BrokerTest::BenckmarkCPUWallTimeTest() {
  Arc::Broker b(usercfg, job, "TEST");
  CPPUNIT_ASSERT(b.isValid());

  (*etl.front().Benchmarks)["TestBenchmark"] = 100.;

  job.Resources.IndividualCPUTime.benchmark = std::pair<std::string, double>("TestBenchmark", 50.);
  etl.front().ComputingShare->MaxCPUTime = 100;
  job.Resources.IndividualCPUTime.range.max = 210;
  CPPUNIT_ASSERT(!b.match(etl.front()));
  job.Resources.IndividualCPUTime.range.max = 200;
  CPPUNIT_ASSERT(b.match(etl.front()));
  job.Resources.IndividualCPUTime.range.max = 190;
  CPPUNIT_ASSERT(b.match(etl.front()));
  etl.front().ComputingShare->MaxCPUTime = -1;
  job.Resources.IndividualCPUTime.range.max = -1;

  etl.front().ComputingShare->MinCPUTime = 10;
  job.Resources.IndividualCPUTime.range.min = 10;
  CPPUNIT_ASSERT(!b.match(etl.front()));
  job.Resources.IndividualCPUTime.range.min = 20;
  CPPUNIT_ASSERT(b.match(etl.front()));
  job.Resources.IndividualCPUTime.range.min = 30;
  CPPUNIT_ASSERT(b.match(etl.front()));
  etl.front().ComputingShare->MinCPUTime = -1;
  job.Resources.IndividualCPUTime.range.min = -1;
  job.Resources.IndividualCPUTime.benchmark = std::pair<std::string, double>("", -1.);

  job.Resources.IndividualWallTime.benchmark = std::pair<std::string, double>("TestBenchmark", 50.);
  etl.front().ComputingShare->MaxWallTime = 100;
  job.Resources.IndividualWallTime.range.max = 210;
  CPPUNIT_ASSERT(!b.match(etl.front()));
  job.Resources.IndividualWallTime.range.max = 200;
  CPPUNIT_ASSERT(b.match(etl.front()));
  job.Resources.IndividualWallTime.range.max = 190;
  CPPUNIT_ASSERT(b.match(etl.front()));
  etl.front().ComputingShare->MaxWallTime = -1;
  job.Resources.IndividualWallTime.range.max = -1;

  etl.front().ComputingShare->MinWallTime = 10;
  job.Resources.IndividualWallTime.range.min = 10;
  CPPUNIT_ASSERT(!b.match(etl.front()));
  job.Resources.IndividualWallTime.range.min = 20;
  CPPUNIT_ASSERT(b.match(etl.front()));
  job.Resources.IndividualWallTime.range.min = 30;
  CPPUNIT_ASSERT(b.match(etl.front()));
  etl.front().ComputingShare->MinWallTime = -1;
  job.Resources.IndividualWallTime.range.min = -1;
  job.Resources.IndividualWallTime.benchmark = std::pair<std::string, double>("", -1.);

  etl.front().ExecutionEnvironment->CPUClockSpeed = 2500;
  job.Resources.IndividualCPUTime.benchmark = std::pair<std::string, double>("clock rate", 1000.);
  etl.front().ComputingShare->MaxCPUTime = 100;
  job.Resources.IndividualCPUTime.range.max = 300;
  CPPUNIT_ASSERT(!b.match(etl.front()));
  job.Resources.IndividualCPUTime.range.max = 250;
  CPPUNIT_ASSERT(b.match(etl.front()));
  job.Resources.IndividualCPUTime.range.max = 200;
  CPPUNIT_ASSERT(b.match(etl.front()));
  etl.front().ExecutionEnvironment->CPUClockSpeed = -1;
  etl.front().ComputingShare->MaxCPUTime = -1;
  job.Resources.IndividualCPUTime.range.max = -1;
  job.Resources.IndividualCPUTime.benchmark = std::pair<std::string, double>("", -1.);
}

void BrokerTest::RegresssionTestMultipleDifferentJobDescriptions() {
  job.Resources.QueueName = "front";

  Arc::Broker b(usercfg, job, "TEST");
  CPPUNIT_ASSERT(b.isValid());

  /* When prefiltered by the broker, each JobDescription object "correspond" to
   * a (list of) ExecutionTarget object(s).
   */

  Arc::ExecutionTargetSorter ets(b);
  Arc::ExecutionTarget aET, bET;
  aET.ComputingEndpoint->URLString = "http://localhost/test";
  aET.ComputingEndpoint->HealthState = "ok";
  aET.ComputingShare->Name = "front";
  ets.addEntity(aET);
  bET.ComputingEndpoint->URLString = "http://localhost/test";
  bET.ComputingEndpoint->HealthState = "ok";
  bET.ComputingShare->Name = "back";
  ets.addEntity(bET);
  ets.reset();
  
  CPPUNIT_ASSERT_EQUAL(1, (int)ets.getMatchingTargets().size());
  CPPUNIT_ASSERT_EQUAL((std::string)"front", ets->ComputingShare->Name);

  job.Resources.QueueName = "back";
  ets.set(job);
  CPPUNIT_ASSERT_EQUAL(1, (int)ets.getMatchingTargets().size());
  CPPUNIT_ASSERT_EQUAL((std::string)"back", ets->ComputingShare->Name);
}

void BrokerTest::RejectTargetsTest() {
  job.Application.Executable.Path = "executable";

  Arc::Broker b(usercfg, job, "TEST");
  CPPUNIT_ASSERT(b.isValid());


  {
  // Rejecting no targets.
  Arc::ExecutionTarget target;
  target.ComputingEndpoint->HealthState = "ok";
  Arc::ExecutionTargetSorter ets(b);
  target.ComputingEndpoint->URLString = "http://localhost/test1"; ets.addEntity(target);
  target.ComputingEndpoint->URLString = "http://localhost/test2"; ets.addEntity(target);
  CPPUNIT_ASSERT_EQUAL(2, (int)ets.getMatchingTargets().size());
  }

  {
  // Reject test1 target.
  std::list<Arc::URL> rejectTargets;
  rejectTargets.push_back(Arc::URL("http://localhost/test1"));
  Arc::ExecutionTarget aET, bET;
  aET.ComputingEndpoint->HealthState = "ok";
  aET.ComputingEndpoint->URLString = "http://localhost/test1"; 
  bET.ComputingEndpoint->HealthState = "ok";
  bET.ComputingEndpoint->URLString = "http://localhost/test2";
  Arc::ExecutionTargetSorter ets(b, rejectTargets);
  ets.addEntity(aET); ets.addEntity(bET);
  ets.reset();
  CPPUNIT_ASSERT_EQUAL(1, (int)ets.getMatchingTargets().size());
  CPPUNIT_ASSERT_EQUAL((std::string)"http://localhost/test2", ets->ComputingEndpoint->URLString);
  }

  {
  // Reject both targets.
  std::list<Arc::URL> rejectTargets;
  rejectTargets.push_back(Arc::URL("http://localhost/test1"));
  rejectTargets.push_back(Arc::URL("http://localhost/test2"));
  Arc::ExecutionTarget aET, bET;
  aET.ComputingEndpoint->HealthState = "ok";
  aET.ComputingEndpoint->URLString = "http://localhost/test1"; 
  bET.ComputingEndpoint->HealthState = "ok";
  bET.ComputingEndpoint->URLString = "http://localhost/test2";
  Arc::ExecutionTargetSorter ets(b, rejectTargets);
  ets.addEntity(aET); ets.addEntity(bET);
  ets.reset();
  CPPUNIT_ASSERT_EQUAL(0, (int)ets.getMatchingTargets().size());
  }
}

CPPUNIT_TEST_SUITE_REGISTRATION(BrokerTest);
