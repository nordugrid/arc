// -*- indent-tabs-mode: nil -*-

#include <string>

#include <cppunit/extensions/HelperMacros.h>

#include <arc/XMLNode.h>

class XMLNodeTest
  : public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(XMLNodeTest);
  CPPUNIT_TEST(TestParsing);
  CPPUNIT_TEST_SUITE_END();

public:
  void setUp();
  void tearDown();
  void TestParsing();

};


void XMLNodeTest::setUp() {
}


void XMLNodeTest::tearDown() {
}


void XMLNodeTest::TestParsing() {
  Arc::XMLNode xml(
     "<!-- comment must not be visible -->\n"
     "<root>\n"
     "  <child1>value1</child1>\n"
     "  <child2>value2</child2>\n"
     "</root>"
  );
  CPPUNIT_ASSERT((bool)xml);
  CPPUNIT_ASSERT_EQUAL(std::string("root"), xml.Name());
  CPPUNIT_ASSERT_EQUAL(std::string("value1"), (std::string)xml["child1"]);
  CPPUNIT_ASSERT_EQUAL(std::string("value2"), (std::string)xml["child2"]);
}

CPPUNIT_TEST_SUITE_REGISTRATION(XMLNodeTest);
