#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>

#include <cppunit/extensions/HelperMacros.h>

#include "../BenchmarkBrokerPlugin.cpp"
#include <arc/client/ExecutionTarget.h>

class BenchmarkBrokerTest
  : public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(BenchmarkBrokerTest);
  CPPUNIT_TEST(TestComparePerformance);
  CPPUNIT_TEST_SUITE_END();

public:
  BenchmarkBrokerTest() : COMPARE_TEST_BENCHMARK(NULL), COMPARE_OTHER_BENCHMARK(NULL), COMPARE_ANOTHER_BENCHMARK(NULL) {}

  void setUp();
  void tearDown();

  void TestComparePerformance();

private:
  Arc::ExecutionTarget UNKNOWN_BENCHMARK_TARGET;
  Arc::ExecutionTarget SLOW_TARGET;
  Arc::ExecutionTarget FAST_TARGET;
  Arc::ExecutionTarget SUPERFAST_TARGET;
  Arc::ExecutionTarget OTHER_BENCHMARK_TARGET;
  Arc::ExecutionTarget MULTI_BENCHMARK_TARGET;
  Arc::BenchmarkBrokerPlugin *COMPARE_TEST_BENCHMARK;
  Arc::BenchmarkBrokerPlugin *COMPARE_OTHER_BENCHMARK;
  Arc::BenchmarkBrokerPlugin *COMPARE_ANOTHER_BENCHMARK;
};

void BenchmarkBrokerTest::setUp() {
  (*SLOW_TARGET.Benchmarks)["test"] = 10;
  (*FAST_TARGET.Benchmarks)["test"] = 100;
  (*SUPERFAST_TARGET.Benchmarks)["test"] = 1000;
  (*OTHER_BENCHMARK_TARGET.Benchmarks)["other"] = 43;
  (*MULTI_BENCHMARK_TARGET.Benchmarks)["test"] = 42;
  (*MULTI_BENCHMARK_TARGET.Benchmarks)["other"] = 41;
}

void BenchmarkBrokerTest::tearDown() {
}

void BenchmarkBrokerTest::TestComparePerformance() {
  Arc::UserConfig uc;
  uc.Broker("", "test");
  Arc::BrokerPluginArgument arg(uc);
  COMPARE_TEST_BENCHMARK = new Arc::BenchmarkBrokerPlugin(&arg);
  CPPUNIT_ASSERT_MESSAGE("FAST should be faster than SLOW",
    (*COMPARE_TEST_BENCHMARK)(FAST_TARGET, SLOW_TARGET));
  CPPUNIT_ASSERT_MESSAGE("SLOW should not be faster than FAST",
    !(*COMPARE_TEST_BENCHMARK)(SLOW_TARGET, FAST_TARGET));

  CPPUNIT_ASSERT_MESSAGE("SUPERFAST should be faster than FAST",
    (*COMPARE_TEST_BENCHMARK)(SUPERFAST_TARGET, FAST_TARGET));
  CPPUNIT_ASSERT_MESSAGE("FAST should not be faster than SUPERFAST",
    !(*COMPARE_TEST_BENCHMARK)(FAST_TARGET, SUPERFAST_TARGET));

  CPPUNIT_ASSERT_MESSAGE("FAST should be faster than MULTI_BENCHMARK",
    (*COMPARE_TEST_BENCHMARK)(FAST_TARGET, MULTI_BENCHMARK_TARGET));
  CPPUNIT_ASSERT_MESSAGE("MULTI_BENCHMARK should not be faster than FAST",
    !(*COMPARE_TEST_BENCHMARK)(MULTI_BENCHMARK_TARGET, FAST_TARGET));

  uc.Broker("", "other");
  COMPARE_OTHER_BENCHMARK = new Arc::BenchmarkBrokerPlugin(&arg);
  CPPUNIT_ASSERT_MESSAGE("OTHER_BENCHMARK should be faster than MULTI_BENCHMARK if the 'other' benchmark is used",
    (*COMPARE_OTHER_BENCHMARK)(OTHER_BENCHMARK_TARGET, MULTI_BENCHMARK_TARGET));
  CPPUNIT_ASSERT_MESSAGE("MULTI_BENCHMARK should be faster than OTHER_BENCHMARK if the 'test' benchmark is used",
    (*COMPARE_TEST_BENCHMARK)(MULTI_BENCHMARK_TARGET, OTHER_BENCHMARK_TARGET));

  CPPUNIT_ASSERT_MESSAGE("SLOW should be faster than UNKNOWN_BENCHMARK",
    (*COMPARE_TEST_BENCHMARK)(SLOW_TARGET, UNKNOWN_BENCHMARK_TARGET));
  uc.Broker("", "another");
  COMPARE_ANOTHER_BENCHMARK = new Arc::BenchmarkBrokerPlugin(&arg);
  CPPUNIT_ASSERT_MESSAGE("if none of the targets has the used benchmark, this should be always false",
    !(*COMPARE_ANOTHER_BENCHMARK)(FAST_TARGET, SLOW_TARGET));
    
  delete COMPARE_TEST_BENCHMARK;
  delete COMPARE_OTHER_BENCHMARK;
  delete COMPARE_ANOTHER_BENCHMARK;
}

CPPUNIT_TEST_SUITE_REGISTRATION(BenchmarkBrokerTest);
