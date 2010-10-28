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
  CPPUNIT_TEST(CPUWallTimeTest);
  CPPUNIT_TEST(BenckmarkCPUWallTimeTest);
  CPPUNIT_TEST_SUITE_END();

public:
  BrokerTest();
  void setUp();
  void tearDown();
  void CPUWallTimeTest();
  void BenckmarkCPUWallTimeTest();

  class TestBroker
    : public Arc::Broker {
  public:
    TestBroker(const Arc::UserConfig& usercfg) : Arc::Broker(usercfg) {}
    void SortTargets() {}
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

void BrokerTest::CPUWallTimeTest() {
  etl.front().MaxCPUTime = 100;
  job.Resources.TotalCPUTime.range.max = 110; CPPASSERT(0)
  job.Resources.TotalCPUTime.range.max = 100; CPPASSERT(1)
  job.Resources.TotalCPUTime.range.max = 90;  CPPASSERT(1)

  etl.front().MinCPUTime = 10;
  job.Resources.TotalCPUTime.range.min = 5; CPPASSERT(0)
  job.Resources.TotalCPUTime.range.min = 10; CPPASSERT(1)
  job.Resources.TotalCPUTime.range.min = 15; CPPASSERT(1)

  etl.front().MaxWallTime = 100;
  job.Resources.TotalWallTime.range.max = 110; CPPASSERT(0)
  job.Resources.TotalWallTime.range.max = 100; CPPASSERT(1)
  job.Resources.TotalWallTime.range.max = 90;  CPPASSERT(1)

  etl.front().MinWallTime = 10;
  job.Resources.TotalWallTime.range.min = 5;  CPPASSERT(0)
  job.Resources.TotalWallTime.range.min = 10; CPPASSERT(1)
  job.Resources.TotalWallTime.range.min = 15; CPPASSERT(1)
}

void BrokerTest::BenckmarkCPUWallTimeTest() {
  etl.front().Benchmarks["TestBenchmark"] = 100.;

  job.Resources.TotalCPUTime.benchmark = std::pair<std::string, double>("TestBenchmark", 50.);
  etl.front().MaxCPUTime = 100;
  job.Resources.TotalCPUTime.range.max = 210; CPPASSERT(0)
  job.Resources.TotalCPUTime.range.max = 200; CPPASSERT(1)
  job.Resources.TotalCPUTime.range.max = 190;  CPPASSERT(1)

  etl.front().MinCPUTime = 10;
  job.Resources.TotalCPUTime.range.min = 10; CPPASSERT(0)
  job.Resources.TotalCPUTime.range.min = 20; CPPASSERT(1)
  job.Resources.TotalCPUTime.range.min = 30; CPPASSERT(1)

  job.Resources.TotalWallTime.benchmark = std::pair<std::string, double>("TestBenchmark", 50.);
  etl.front().MaxWallTime = 100;
  job.Resources.TotalWallTime.range.max = 210; CPPASSERT(0)
  job.Resources.TotalWallTime.range.max = 200; CPPASSERT(1)
  job.Resources.TotalWallTime.range.max = 190;  CPPASSERT(1)

  etl.front().MinWallTime = 10;
  job.Resources.TotalWallTime.range.min = 10;  CPPASSERT(0)
  job.Resources.TotalWallTime.range.min = 20; CPPASSERT(1)
  job.Resources.TotalWallTime.range.min = 30; CPPASSERT(1)

  etl.front().CPUClockSpeed = 2500;
  job.Resources.TotalCPUTime.benchmark = std::pair<std::string, double>("ARC-clockrate", 1000.);
  etl.front().MaxCPUTime = 100;
  job.Resources.TotalCPUTime.range.max = 300; CPPASSERT(0)
  job.Resources.TotalCPUTime.range.max = 250; CPPASSERT(1)
  job.Resources.TotalCPUTime.range.max = 200;  CPPASSERT(1)

  etl.front().Benchmarks.erase("ARC-clockrate");
  etl.front().CPUClockSpeed = 5600;
  etl.front().MaxCPUTime = 200;
  job.Resources.TotalCPUTime.range.max = -1;
  job.Resources.TotalCPUTime.benchmark = std::pair<std::string, double>("", -1);
  std::string xrsl = "&(executable=/bin/echo)(gridtime=600s)";
  CPPUNIT_ASSERT(job.Parse(xrsl)); CPPASSERT(0)
  xrsl = "&(executable=/bin/echo)(gridtime=400s)";
  CPPUNIT_ASSERT(job.Parse(xrsl)); CPPASSERT(1)
  xrsl = "&(executable=/bin/echo)(gridtime=200s)";
  CPPUNIT_ASSERT(job.Parse(xrsl)); CPPASSERT(1)
}

CPPUNIT_TEST_SUITE_REGISTRATION(BrokerTest);
