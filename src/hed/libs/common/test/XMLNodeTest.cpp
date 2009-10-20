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
  std::string xml_str(
     "<!-- comment must not be visible -->\n"
     "<root>\n"
     "  <child1>value1</child1>\n"
     "  <child2>value2</child2>\n"
     "</root>"
  );
  Arc::XMLNode xml(xml_str);
  CPPUNIT_ASSERT((bool)xml);
  CPPUNIT_ASSERT_EQUAL(std::string("root"), xml.Name());
  CPPUNIT_ASSERT_EQUAL(std::string("value1"), (std::string)xml["child1"]);
  CPPUNIT_ASSERT_EQUAL(std::string("value2"), (std::string)xml["child2"]);
  std::string s;
  xml.GetXML(s);
  CPPUNIT_ASSERT_EQUAL(std::string("<root>\n  <child1>value1</child1>\n  <child2>value2</child2>\n</root>"),s);
  xml.GetDoc(s);
  CPPUNIT_ASSERT_EQUAL("<?xml version=\"1.0\"?>\n"+xml_str+"\n",s);
}

CPPUNIT_TEST_SUITE_REGISTRATION(XMLNodeTest);
