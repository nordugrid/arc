// -*- indent-tabs-mode: nil -*-

#include <string>

#include <cppunit/extensions/HelperMacros.h>

#include <arc/XMLNode.h>

class XMLNodeTest
  : public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(XMLNodeTest);
  CPPUNIT_TEST(TestParsing);
  CPPUNIT_TEST(TestExchange);
  CPPUNIT_TEST(TestMove);
  CPPUNIT_TEST_SUITE_END();

public:
  void setUp();
  void tearDown();
  void TestParsing();
  void TestExchange();
  void TestMove();

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

void XMLNodeTest::TestExchange() {
  std::string xml1_str(
     "<root1>"
      "<child1>value1</child1>"
      "<child2>"
       "<child3>value3</child3>"
      "</child2>"
     "</root1>"
  );
  std::string xml2_str(
     "<root2>"
      "<child4>value4</child4>"
      "<child5>"
       "<child6>value6</child6>"
      "</child5>"
     "</root2>"
  );
  Arc::XMLNode* xml1 = NULL;
  Arc::XMLNode* xml2 = NULL;
  Arc::XMLNode node1;
  Arc::XMLNode node2;

  // Exchanging ordinary nodes
  xml1 = new Arc::XMLNode(xml1_str);
  xml2 = new Arc::XMLNode(xml2_str);
  node1 = (*xml1)["child2"];
  node2 = (*xml2)["child5"];
  node1.Exchange(node2);
  delete xml2; xml2 = new Arc::XMLNode(xml1_str);
  CPPUNIT_ASSERT_EQUAL(std::string("value6"),(std::string)((*xml1)["child5"]["child6"]));
  delete xml1;
  delete xml2;

  // Exchanging root nodes - operation must fail
  xml1 = new Arc::XMLNode(xml1_str);
  xml2 = new Arc::XMLNode(xml2_str);
  node1 = (*xml1);
  node2 = (*xml2);
  node1.Exchange(node2);
  delete xml2; xml2 = new Arc::XMLNode(xml1_str);
  CPPUNIT_ASSERT_EQUAL(std::string("root1"),(*xml1).Name());
  delete xml1;
  delete xml2;

  // Exchanging documents
  xml1 = new Arc::XMLNode(xml1_str);
  xml2 = new Arc::XMLNode(xml2_str);
  xml1->Exchange(*xml2);
  CPPUNIT_ASSERT_EQUAL(std::string("root2"),(*xml1).Name());
  CPPUNIT_ASSERT_EQUAL(std::string("root1"),(*xml2).Name());
  delete xml1;
  delete xml2;

  // Exchanging document and empty node
  xml1 = new Arc::XMLNode(xml1_str);
  xml2 = new Arc::XMLNode;
  xml1->Exchange(*xml2);
  CPPUNIT_ASSERT_EQUAL(std::string("root1"),(*xml2).Name());
  CPPUNIT_ASSERT(!(*xml1));
  delete xml1;
  delete xml2;

}

void XMLNodeTest::TestMove() {
  std::string xml_str(
     "<root1>"
      "<child1>value1</child1>"
      "<child2>"
       "<child3>value3</child3>"
      "</child2>"
     "</root1>"
  );
  Arc::XMLNode xml1(xml_str);
  Arc::XMLNode xml2;
  Arc::XMLNode node = xml1["child2"];
  node.Move(xml2);
  CPPUNIT_ASSERT_EQUAL(std::string("child2"),xml2.Name());
  CPPUNIT_ASSERT_EQUAL(std::string("value3"),(std::string)(xml2["child3"]));
  CPPUNIT_ASSERT_EQUAL(std::string("root1"),xml1.Name());
  CPPUNIT_ASSERT_EQUAL(std::string("value1"),(std::string)(xml1["child1"]));
  CPPUNIT_ASSERT(!(xml1["child2"]));
}

CPPUNIT_TEST_SUITE_REGISTRATION(XMLNodeTest);
