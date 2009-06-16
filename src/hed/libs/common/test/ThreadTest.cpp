// -*- indent-tabs-mode: nil -*-

#include <string>

#include <cppunit/extensions/HelperMacros.h>

#include <arc/Thread.h>

class ThreadTest
  : public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(ThreadTest);
  CPPUNIT_TEST(TestThread);
  CPPUNIT_TEST_SUITE_END();

public:
  void setUp();
  void tearDown();
  void TestThread();

private:
  static void func(void*);
  static int counter;
};


void ThreadTest::setUp() {
  counter = 0;
}


void ThreadTest::tearDown() {
}

void ThreadTest::TestThread() {
  // Simply run 500 threads and see if executable crashes
  for(int n = 0;n<500;++n) {
    CPPUNIT_ASSERT(Arc::CreateThreadFunction(&func,NULL));
  }
  sleep(30);
  CPPUNIT_ASSERT_EQUAL(500,counter);
}

void ThreadTest::func(void*) {
  sleep(1);
  ++counter;
}

int ThreadTest::counter = 0;

CPPUNIT_TEST_SUITE_REGISTRATION(ThreadTest);

