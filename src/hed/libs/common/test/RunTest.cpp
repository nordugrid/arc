// -*- indent-tabs-mode: nil -*-

#include <string>

#include <cppunit/extensions/HelperMacros.h>

#include <arc/Run.h>

class RunTest
  : public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(RunTest);
  CPPUNIT_TEST(TestRun0);
  CPPUNIT_TEST(TestRun255);
  CPPUNIT_TEST_SUITE_END();

public:
  void setUp();
  void tearDown();
  void TestRun0();
  void TestRun255();

private:
};


void RunTest::setUp() {
}


void RunTest::tearDown() {
}

void RunTest::TestRun0() {
  Arc::Run run("./rcode 0");
  CPPUNIT_ASSERT((bool)run);
  CPPUNIT_ASSERT(run.Start());
  CPPUNIT_ASSERT(run.Wait(10));
  CPPUNIT_ASSERT_EQUAL(0, run.Result());
}

void RunTest::TestRun255() {
  Arc::Run run("./rcode 255");
  CPPUNIT_ASSERT((bool)run);
  CPPUNIT_ASSERT(run.Start());
  CPPUNIT_ASSERT(run.Wait(10));
  CPPUNIT_ASSERT_EQUAL(255, run.Result());
}

CPPUNIT_TEST_SUITE_REGISTRATION(RunTest);

