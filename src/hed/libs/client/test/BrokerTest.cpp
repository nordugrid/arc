#include <iostream>

#include <cppunit/extensions/HelperMacros.h>

#include <arc/UserConfig.h>
#include <arc/Utils.h>
#include <arc/client/Broker.h>
#include <arc/client/ExecutionTarget.h>
#include <arc/client/JobDescription.h>
#include <arc/Thread.h>

#include <arc/client/TestACCControl.h>

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
  Arc::Broker b(usercfg, "TEST");
  CPPUNIT_ASSERT(b.isValid());

  job.Resources.QueueName = "q1";
  b.set(job);

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
  Arc::Broker b(usercfg, "TEST");
  CPPUNIT_ASSERT(b.isValid());
  b.set(job);

  etl.front().ComputingShare->MaxCPUTime = 100;
  job.Resources.TotalCPUTime.range.max = 110;
  CPPUNIT_ASSERT(!b.match(etl.front()));
  job.Resources.TotalCPUTime.range.max = 100; 
  CPPUNIT_ASSERT(b.match(etl.front()));
  job.Resources.TotalCPUTime.range.max = 90;
  CPPUNIT_ASSERT(b.match(etl.front()));
  etl.front().ComputingShare->MaxCPUTime = -1;
  job.Resources.TotalCPUTime.range.max = -1;

  etl.front().ComputingShare->MinCPUTime = 10;
  job.Resources.TotalCPUTime.range.min = 5;
  CPPUNIT_ASSERT(!b.match(etl.front()));
  job.Resources.TotalCPUTime.range.min = 10;
  CPPUNIT_ASSERT(b.match(etl.front()));
  job.Resources.TotalCPUTime.range.min = 15;
  CPPUNIT_ASSERT(b.match(etl.front()));
  etl.front().ComputingShare->MinCPUTime = -1;
  job.Resources.TotalCPUTime.range.min = -1;

  etl.front().ComputingShare->MaxWallTime = 100;
  job.Resources.TotalWallTime.range.max = 110;
  CPPUNIT_ASSERT(!b.match(etl.front()));
  job.Resources.TotalWallTime.range.max = 100;
  CPPUNIT_ASSERT(b.match(etl.front()));
  job.Resources.TotalWallTime.range.max = 90;
  CPPUNIT_ASSERT(b.match(etl.front()));
  etl.front().ComputingShare->MaxWallTime = -1;
  job.Resources.TotalWallTime.range.max = -1;

  etl.front().ComputingShare->MinWallTime = 10;
  job.Resources.TotalWallTime.range.min = 5;
  CPPUNIT_ASSERT(!b.match(etl.front()));
  job.Resources.TotalWallTime.range.min = 10;
  CPPUNIT_ASSERT(b.match(etl.front()));
  job.Resources.TotalWallTime.range.min = 15;
  CPPUNIT_ASSERT(b.match(etl.front()));
  etl.front().ComputingShare->MinWallTime = -1;
  job.Resources.TotalWallTime.range.min = -1;
}

void BrokerTest::BenckmarkCPUWallTimeTest() {
  Arc::Broker b(usercfg, "TEST");
  CPPUNIT_ASSERT(b.isValid());
  b.set(job);

  (*etl.front().Benchmarks)["TestBenchmark"] = 100.;

  job.Resources.TotalCPUTime.benchmark = std::pair<std::string, double>("TestBenchmark", 50.);
  etl.front().ComputingShare->MaxCPUTime = 100;
  job.Resources.TotalCPUTime.range.max = 210;
  CPPUNIT_ASSERT(!b.match(etl.front()));
  job.Resources.TotalCPUTime.range.max = 200;
  CPPUNIT_ASSERT(b.match(etl.front()));
  job.Resources.TotalCPUTime.range.max = 190;
  CPPUNIT_ASSERT(b.match(etl.front()));
  etl.front().ComputingShare->MaxCPUTime = -1;
  job.Resources.TotalCPUTime.range.max = -1;

  etl.front().ComputingShare->MinCPUTime = 10;
  job.Resources.TotalCPUTime.range.min = 10;
  CPPUNIT_ASSERT(!b.match(etl.front()));
  job.Resources.TotalCPUTime.range.min = 20;
  CPPUNIT_ASSERT(b.match(etl.front()));
  job.Resources.TotalCPUTime.range.min = 30;
  CPPUNIT_ASSERT(b.match(etl.front()));
  etl.front().ComputingShare->MinCPUTime = -1;
  job.Resources.TotalCPUTime.range.min = -1;
  job.Resources.TotalCPUTime.benchmark = std::pair<std::string, double>("", -1.);

  job.Resources.TotalWallTime.benchmark = std::pair<std::string, double>("TestBenchmark", 50.);
  etl.front().ComputingShare->MaxWallTime = 100;
  job.Resources.TotalWallTime.range.max = 210;
  CPPUNIT_ASSERT(!b.match(etl.front()));
  job.Resources.TotalWallTime.range.max = 200;
  CPPUNIT_ASSERT(b.match(etl.front()));
  job.Resources.TotalWallTime.range.max = 190;
  CPPUNIT_ASSERT(b.match(etl.front()));
  etl.front().ComputingShare->MaxWallTime = -1;
  job.Resources.TotalWallTime.range.max = -1;

  etl.front().ComputingShare->MinWallTime = 10;
  job.Resources.TotalWallTime.range.min = 10;
  CPPUNIT_ASSERT(!b.match(etl.front()));
  job.Resources.TotalWallTime.range.min = 20;
  CPPUNIT_ASSERT(b.match(etl.front()));
  job.Resources.TotalWallTime.range.min = 30;
  CPPUNIT_ASSERT(b.match(etl.front()));
  etl.front().ComputingShare->MinWallTime = -1;
  job.Resources.TotalWallTime.range.min = -1;
  job.Resources.TotalWallTime.benchmark = std::pair<std::string, double>("", -1.);

  etl.front().ExecutionEnvironment->CPUClockSpeed = 2500;
  job.Resources.TotalCPUTime.benchmark = std::pair<std::string, double>("clock rate", 1000.);
  etl.front().ComputingShare->MaxCPUTime = 100;
  job.Resources.TotalCPUTime.range.max = 300;
  CPPUNIT_ASSERT(!b.match(etl.front()));
  job.Resources.TotalCPUTime.range.max = 250;
  CPPUNIT_ASSERT(b.match(etl.front()));
  job.Resources.TotalCPUTime.range.max = 200;
  CPPUNIT_ASSERT(b.match(etl.front()));
  etl.front().ExecutionEnvironment->CPUClockSpeed = -1;
  etl.front().ComputingShare->MaxCPUTime = -1;
  job.Resources.TotalCPUTime.range.max = -1;
  job.Resources.TotalCPUTime.benchmark = std::pair<std::string, double>("", -1.);
}

void BrokerTest::RegresssionTestMultipleDifferentJobDescriptions() {
  job.Resources.QueueName = "front";

  Arc::Broker b(usercfg, job, "TEST");
  CPPUNIT_ASSERT(b.isValid());

  /* When prefiltered by the broker, each JobDescription object "correspond" to
   * a (list of) ExecutionTarget object(s).
   */

  Arc::ExecutionTargetSet ets(b);
  Arc::ExecutionTarget aET, bET;
  aET.ComputingEndpoint->URLString = "http://localhost/test";
  aET.ComputingEndpoint->HealthState = "ok";
  aET.ComputingShare->Name = "front";
  ets.insert(aET);
  bET.ComputingEndpoint->URLString = "http://localhost/test";
  bET.ComputingEndpoint->HealthState = "ok";
  bET.ComputingShare->Name = "back";
  ets.insert(bET);
  
  CPPUNIT_ASSERT_EQUAL(1, (int)ets.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"front", ets.begin()->ComputingShare->Name);

  job.Resources.QueueName = "back";
  ets.set(job);
  ets.insert(aET); ets.insert(bET);
  CPPUNIT_ASSERT_EQUAL(1, (int)ets.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"back", ets.begin()->ComputingShare->Name);
}

void BrokerTest::RejectTargetsTest() {
  job.Application.Executable.Path = "executable";

  Arc::Broker b(usercfg, job, "TEST");
  CPPUNIT_ASSERT(b.isValid());


  {
  // Rejecting no targets.
  Arc::ExecutionTarget target;
  target.ComputingEndpoint->HealthState = "ok";
  Arc::ExecutionTargetSet ets(b);
  target.ComputingEndpoint->URLString = "http://localhost/test1"; ets.insert(target);
  target.ComputingEndpoint->URLString = "http://localhost/test2"; ets.insert(target);
  CPPUNIT_ASSERT_EQUAL(2, (int)ets.size());
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
  Arc::ExecutionTargetSet ets(b, rejectTargets);
  ets.insert(aET); ets.insert(bET);
  CPPUNIT_ASSERT_EQUAL(1, (int)ets.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"http://localhost/test2", ets.begin()->ComputingEndpoint->URLString);
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
  Arc::ExecutionTargetSet ets(b, rejectTargets);
  ets.insert(aET); ets.insert(bET);
  CPPUNIT_ASSERT_EQUAL(0, (int)ets.size());
  }
}

CPPUNIT_TEST_SUITE_REGISTRATION(BrokerTest);
