// -*- indent-tabs-mode: nil -*-
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif


#include <string>

#include <cppunit/extensions/HelperMacros.h>

// #include "../rest.cpp"

class RESTTest
  : public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(RESTTest);
  CPPUNIT_TEST(TestJsonParse);
  CPPUNIT_TEST_SUITE_END();

public:
  void setUp();
  void tearDown();
  void TestJsonParse();
};


void RESTTest::setUp() {
}


void RESTTest::tearDown() {
}


void RESTTest::TestJsonParse() {
  // TODO: Implement
}

CPPUNIT_TEST_SUITE_REGISTRATION(RESTTest);
