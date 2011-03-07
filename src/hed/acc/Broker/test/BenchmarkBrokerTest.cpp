#include <string>

#include <cppunit/extensions/HelperMacros.h>

#include "../BenchmarkBroker.h"
#include "../BenchmarkBroker.cpp"
#include <arc/client/ExecutionTarget.h>

class BenchmarkBrokerTest
  : public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(BenchmarkBrokerTest);
  CPPUNIT_TEST(TestComparePerformance);
  CPPUNIT_TEST_SUITE_END();

public:
  BenchmarkBrokerTest() : COMPARE_TEST_BENCHMARK("test"), COMPARE_OTHER_BENCHMARK("other"), COMPARE_ANOTHER_BENCHMARK("another") {}

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
  Arc::cmp COMPARE_TEST_BENCHMARK;
  Arc::cmp COMPARE_OTHER_BENCHMARK;
  Arc::cmp COMPARE_ANOTHER_BENCHMARK;
};

void BenchmarkBrokerTest::setUp() {
  SLOW_TARGET.Benchmarks["test"] = 10;
  FAST_TARGET.Benchmarks["test"] = 100;
  SUPERFAST_TARGET.Benchmarks["test"] = 1000;
  OTHER_BENCHMARK_TARGET.Benchmarks["other"] = 43;
  MULTI_BENCHMARK_TARGET.Benchmarks["test"] = 42;
  MULTI_BENCHMARK_TARGET.Benchmarks["other"] = 41;
}

void BenchmarkBrokerTest::tearDown() {
}

void BenchmarkBrokerTest::TestComparePerformance() {
  CPPUNIT_ASSERT_MESSAGE("FAST should be faster then SLOW",
    COMPARE_TEST_BENCHMARK.ComparePerformance(&FAST_TARGET, &SLOW_TARGET));
  CPPUNIT_ASSERT_MESSAGE("SLOW should not be faster then FAST",
    !COMPARE_TEST_BENCHMARK.ComparePerformance(&SLOW_TARGET, &FAST_TARGET));
    
  CPPUNIT_ASSERT_MESSAGE("SUPERFAST should be faster than FAST",
    COMPARE_TEST_BENCHMARK.ComparePerformance(&SUPERFAST_TARGET, &FAST_TARGET));
  CPPUNIT_ASSERT_MESSAGE("FAST should not be faster than SUPERFAST",
    !COMPARE_TEST_BENCHMARK.ComparePerformance(&FAST_TARGET, &SUPERFAST_TARGET));
  
  CPPUNIT_ASSERT_MESSAGE("FAST should be faster than MULTI_BENCHMARK",
    COMPARE_TEST_BENCHMARK.ComparePerformance(&FAST_TARGET, &MULTI_BENCHMARK_TARGET));
  CPPUNIT_ASSERT_MESSAGE("MULTI_BENCHMARK should not be faster than FAST",
    !COMPARE_TEST_BENCHMARK.ComparePerformance(&MULTI_BENCHMARK_TARGET, &FAST_TARGET));
  
  CPPUNIT_ASSERT_MESSAGE("OTHER_BENCHMARK should be faster than MULTI_BENCHMARK if the 'other' benchmark is used",
    COMPARE_OTHER_BENCHMARK.ComparePerformance(&OTHER_BENCHMARK_TARGET, &MULTI_BENCHMARK_TARGET));
  CPPUNIT_ASSERT_MESSAGE("MULTI_BENCHMARK should be faster than OTHER_BENCHMARK if the 'test' benchmark is used",
    COMPARE_TEST_BENCHMARK.ComparePerformance(&MULTI_BENCHMARK_TARGET, &OTHER_BENCHMARK_TARGET));
  
  CPPUNIT_ASSERT_MESSAGE("SLOW should be faster than UNKNOWN_BENCHMARK",
    COMPARE_TEST_BENCHMARK.ComparePerformance(&SLOW_TARGET, &UNKNOWN_BENCHMARK_TARGET));
  CPPUNIT_ASSERT_MESSAGE("if none of the targets has the used benchmark, this should be always false",
    !COMPARE_ANOTHER_BENCHMARK.ComparePerformance(&FAST_TARGET, &SLOW_TARGET));
}

CPPUNIT_TEST_SUITE_REGISTRATION(BenchmarkBrokerTest);
