// -*- indent-tabs-mode: nil -*-
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif


#include <string>
#include <unistd.h>

#include <cppunit/extensions/HelperMacros.h>

#include <arc/Watchdog.h>

class WatchdogTest
  : public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(WatchdogTest);
  CPPUNIT_TEST(TestWatchdog);
  CPPUNIT_TEST_SUITE_END();

public:
  void setUp();
  void tearDown();
  void TestWatchdog();

};

void WatchdogTest::setUp() {
}


void WatchdogTest::tearDown() {
}

void WatchdogTest::TestWatchdog() {
  Arc::WatchdogListener l;
  Arc::WatchdogChannel c(20);
  CPPUNIT_ASSERT_EQUAL(true,l.Listen());
}

CPPUNIT_TEST_SUITE_REGISTRATION(WatchdogTest);

