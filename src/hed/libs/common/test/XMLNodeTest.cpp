// -*- indent-tabs-mode: nil -*-
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif


#include <string>

#include <cppunit/extensions/HelperMacros.h>

#include <arc/XMLNode.h>

class XMLNodeTest
  : public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(XMLNodeTest);
  CPPUNIT_TEST(TestParsing);
  CPPUNIT_TEST(TestExchange);
  CPPUNIT_TEST(TestMove);
  CPPUNIT_TEST(TestQuery);
  CPPUNIT_TEST_SUITE_END();

public:
  void setUp();
  void tearDown();
  void TestParsing();
  void TestExchange();
  void TestMove();
  void TestQuery();

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
     "<ns1:root1 xmlns:ns1=\"uri:ns1\" xmlns:ns2=\"uri:ns2\">"
      "<ns1:child1>value1</ns1:child1>"
      "<ns2:child2>"
       "<ns2:child3>value3</ns2:child3>"
      "</ns2:child2>"
     "</ns1:root1>"
  );
  std::string xml2_str(
     "<ns3:root2 xmlns:ns3=\"uri:ns3\" xmlns:ns4=\"uri:ns4\">"
      "<ns3:child4>value4</ns3:child4>"
      "<ns4:child5>"
       "<ns4:child6>value6</ns4:child6>"
      "</ns4:child5>"
     "</ns3:root2>"
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
  CPPUNIT_ASSERT_EQUAL(std::string("value6"),(std::string)((*xml1)["ns4:child5"]["ns4:child6"]));
  delete xml1;
  delete xml2;

  // Exchanging root nodes - operation must fail
  xml1 = new Arc::XMLNode(xml1_str);
  xml2 = new Arc::XMLNode(xml2_str);
  node1 = (*xml1);
  node2 = (*xml2);
  node1.Exchange(node2);
  delete xml2; xml2 = new Arc::XMLNode(xml1_str);
  CPPUNIT_ASSERT_EQUAL(std::string("ns1:root1"),(*xml1).FullName());
  delete xml1;
  delete xml2;

  // Exchanging documents
  xml1 = new Arc::XMLNode(xml1_str);
  xml2 = new Arc::XMLNode(xml2_str);
  xml1->Exchange(*xml2);
  CPPUNIT_ASSERT_EQUAL(std::string("ns3:root2"),(*xml1).FullName());
  CPPUNIT_ASSERT_EQUAL(std::string("ns1:root1"),(*xml2).FullName());
  delete xml1;
  delete xml2;

  // Exchanging document and empty node
  xml1 = new Arc::XMLNode(xml1_str);
  xml2 = new Arc::XMLNode;
  xml1->Exchange(*xml2);
  CPPUNIT_ASSERT_EQUAL(std::string("ns1:root1"),(*xml2).FullName());
  CPPUNIT_ASSERT(!(*xml1));
  delete xml1;
  delete xml2;

  // Exchanging ordinary and empty node
  xml1 = new Arc::XMLNode(xml1_str);
  xml2 = new Arc::XMLNode;
  node1 = (*xml1)["ns2:child2"];
  node1.Exchange(*xml2);
  CPPUNIT_ASSERT_EQUAL(std::string("ns2:child2"),xml2->FullName());
  CPPUNIT_ASSERT(!node1);
  delete xml1;
  delete xml2;

}

void XMLNodeTest::TestMove() {
  std::string xml1_str(
     "<ns1:root1 xmlns:ns1=\"uri:ns1\" xmlns:ns2=\"uri:ns2\">"
      "<ns1:child1>value1</ns1:child1>"
      "<ns2:child2>"
       "<ns2:child3>value3</ns2:child3>"
      "</ns2:child2>"
     "</ns1:root1>"
  );
  std::string xml2_str(
     "<ns3:root2 xmlns:ns3=\"uri:ns3\" xmlns:ns4=\"uri:ns4\">"
      "<ns3:child4>value4</ns3:child4>"
      "<ns4:child5>"
       "<ns4:child6>value6</ns4:child6>"
      "</ns4:child5>"
     "</ns3:root2>"
  );
  Arc::XMLNode* xml1 = NULL;
  Arc::XMLNode* xml2 = NULL;
  Arc::XMLNode node1;
  Arc::XMLNode node2;

  // Moving ordinary to ordinary node
  xml1 = new Arc::XMLNode(xml1_str);
  xml2 = new Arc::XMLNode(xml2_str);
  node1 = (*xml1)["child2"];
  node2 = (*xml2)["child5"];
  node1.Move(node2);
  CPPUNIT_ASSERT_EQUAL(std::string("ns2:child2"),node2.FullName());
  CPPUNIT_ASSERT(!node1);
  CPPUNIT_ASSERT((*xml2)["child5"]);
  CPPUNIT_ASSERT(!(*xml1)["child2"]);
  delete xml1;
  delete xml2;

  // Moving ordinary to empty node
  xml1 = new Arc::XMLNode(xml1_str);
  xml2 = new Arc::XMLNode;
  node1 = (*xml1)["child2"];
  node1.Move(*xml2);
  CPPUNIT_ASSERT_EQUAL(std::string("ns2:child2"),xml2->FullName());
  CPPUNIT_ASSERT(!node1);
  delete xml1;
  delete xml2;

  // Moving document to empty node
  xml1 = new Arc::XMLNode(xml1_str);
  xml2 = new Arc::XMLNode;
  xml1->Move(*xml2);
  CPPUNIT_ASSERT_EQUAL(std::string("ns1:root1"),xml2->FullName());
  CPPUNIT_ASSERT(!*xml1);
  delete xml1;
  delete xml2;
}

void XMLNodeTest::TestQuery() {
  std::string xml_str1(
     "<root1>"
      "<child1>value1</child1>"
      "<child2>"
       "<child3>value3</child3>"
      "</child2>"
     "</root1>"
  );
  std::string xml_str2(
     "<ns1:root1 xmlns:ns1=\"http://host/path1\" xmlns:ns2=\"http://host/path2\" >"
      "<ns1:child1>value1</ns1:child1>"
      "<ns2:child2>"
       "<ns2:child3>value3</ns2:child3>"
      "</ns2:child2>"
     "</ns1:root1>"
  );
  Arc::XMLNode xml1(xml_str1);
  Arc::XMLNode xml2(xml_str2);
  Arc::NS ns;
  ns["ns1"] = "http://host/path1";
  ns["ns2"] = "http://host/path2";
  Arc::XMLNodeList list1 = xml1.XPathLookup("/root1/child2",Arc::NS());
  CPPUNIT_ASSERT_EQUAL(1,(int)list1.size());
  
  Arc::XMLNodeList list2 = xml1.XPathLookup("/ns1:root1/ns2:child2",ns);
  CPPUNIT_ASSERT_EQUAL(0,(int)list2.size());

  Arc::XMLNodeList list3 = xml2.XPathLookup("/root1/child2",Arc::NS());
  CPPUNIT_ASSERT_EQUAL(0,(int)list3.size());

  Arc::XMLNodeList list4 = xml2.XPathLookup("/ns1:root1/ns2:child2",ns);
  CPPUNIT_ASSERT_EQUAL(1,(int)list4.size());

  Arc::XMLNodeList list5 = xml2.XPathLookup("/ns1:root1/ns1:child1",ns);
  CPPUNIT_ASSERT_EQUAL(1,(int)list5.size());

  xml2.StripNamespace(-1);
  Arc::XMLNodeList list6 = xml2.XPathLookup("/root1/child1",Arc::NS());
  CPPUNIT_ASSERT_EQUAL(1,(int)list6.size());

}

CPPUNIT_TEST_SUITE_REGISTRATION(XMLNodeTest);
