// -*- indent-tabs-mode: nil -*-

#include <string>

#include <cppunit/extensions/HelperMacros.h>

#include <arc/Run.h>
#include <arc/User.h>
#include <arc/Utils.h>

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
  std::string srcdir;
};


void RunTest::setUp() {
  srcdir = Arc::GetEnv("srcdir");
  if (srcdir.length() == 0)
    srcdir = ".";
}


void RunTest::tearDown() {
}

static void initializer_func(void* arg) {
  Arc::UserSwitch u(0,0);
  CPPUNIT_ASSERT(arg == (void*)1);
}

void RunTest::TestRun0() {
  std::string outstr;
  std::string errstr;
  Arc::Run run(srcdir + "/rcode 0");
  run.AssignStdout(outstr);
  run.AssignStderr(errstr);
  run.AssignInitializer(&initializer_func,(void*)1);
  CPPUNIT_ASSERT((bool)run);
  CPPUNIT_ASSERT(run.Start());
  CPPUNIT_ASSERT(run.Wait(10));
  CPPUNIT_ASSERT_EQUAL(0, run.Result());
  CPPUNIT_ASSERT_EQUAL(std::string("STDOUT"), outstr);
  CPPUNIT_ASSERT_EQUAL(std::string("STDERR"), errstr);
}

void RunTest::TestRun255() {
  std::string outstr;
  std::string errstr;
  Arc::Run run(srcdir + "/rcode 255");
  run.AssignStdout(outstr);
  run.AssignStderr(errstr);
  CPPUNIT_ASSERT((bool)run);
  CPPUNIT_ASSERT(run.Start());
  CPPUNIT_ASSERT(run.Wait(10));
  CPPUNIT_ASSERT_EQUAL(255, run.Result());
}

CPPUNIT_TEST_SUITE_REGISTRATION(RunTest);

