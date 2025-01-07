// -*- indent-tabs-mode: nil -*-
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif


#include <string>

#include <cppunit/extensions/HelperMacros.h>

#include "../rest.cpp"

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
  char const * jsonStr = 
    R"({"job":[
            {"status-code":"201","reason":"Created","id":"id0","state":"ACCEPTING"},
            {"status-code":"201","reason":"Created","id":"id1","state":"ACCEPTING"},
            {"status-code":"201","reason":"Created","id":"id2","state":"ACCEPTING"}
        ]})";
  Arc::XMLNode xml("<jobs/>");
  char const * end = ParseFromJson(xml, jsonStr);
  CPPUNIT_ASSERT_EQUAL((jsonStr+strlen(jsonStr)), end);
  XMLNode jobXml = xml["job"];
  for(int n = 0; n < 3; ++n) {
    CPPUNIT_ASSERT_EQUAL(std::string("201"), static_cast<std::string>(jobXml[n]["status-code"]));
    CPPUNIT_ASSERT_EQUAL(std::string("Created"), static_cast<std::string>(jobXml[n]["reason"]));
    CPPUNIT_ASSERT_EQUAL(std::string("ACCEPTING"), static_cast<std::string>(jobXml[n]["state"]));
    CPPUNIT_ASSERT_EQUAL(std::string("id")+Arc::tostring(n), static_cast<std::string>(jobXml[n]["id"]));
  }
}

CPPUNIT_TEST_SUITE_REGISTRATION(RESTTest);
