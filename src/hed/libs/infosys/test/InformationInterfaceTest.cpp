#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include <string>

#include <cppunit/extensions/HelperMacros.h>

#include <arc/infosys/InformationInterface.h>


class InformationInterfaceTest
  : public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(InformationInterfaceTest);
  CPPUNIT_TEST(TestInformationInterface);
  CPPUNIT_TEST_SUITE_END();

public:
  void setUp();
  void tearDown();
  void TestInformationInterface();
};


void InformationInterfaceTest::setUp() {
}


void InformationInterfaceTest::tearDown() {
}


void InformationInterfaceTest::TestInformationInterface() {

  // Service description document
  Arc::XMLNode infodoc("\
<?xml version=\"1.0\"?>\n\
<InfoDoc xmlns=\"urn:info\">\n\
  <Resource>\n\
    <Memory>A lot</Memory>\n\
    <Performance>Turltle-like</Performance>\n\
  </Resource>\n\
  <Owner>\n\
      <Name>Unknown</Name>\n\
  </Owner>\n\
  <Resource>\n\
      <Memory>640kb enough for everyone</Memory>\n\
      <Performance>Quantum computer</Performance>\n\
  </Resource>\n\
</InfoDoc>\n");

  // Creating service container
  Arc::InformationContainer container(infodoc);
  //std::cout<<"Document:\n"<<doc<<std::endl;

  // Creating client request
  std::list<std::string> name;
  name.push_back("Resource");
  //std::cout<<"Request for elements: "<<*(name.begin())<<std::endl;
  Arc::InformationRequest request(name);
  CPPUNIT_ASSERT((bool)request);

  // Processing request at server side
  Arc::SOAPEnvelope* response = container.Process(*(request.SOAP()));
  CPPUNIT_ASSERT(response);
  CPPUNIT_ASSERT((bool)*response);

  // Extracting result at client
  Arc::InformationResponse res(*response);
  CPPUNIT_ASSERT((bool)res);

  std::list<Arc::XMLNode> results = res.Result();
  CPPUNIT_ASSERT_EQUAL((int)results.size(), int(2));
  std::list<Arc::XMLNode>::iterator r = results.begin();
  CPPUNIT_ASSERT_EQUAL((std::string)(*r)["Memory"], std::string("A lot"));
  ++r;
  CPPUNIT_ASSERT_EQUAL((std::string)(*r)["Performance"], std::string("Quantum computer"));
}

CPPUNIT_TEST_SUITE_REGISTRATION(InformationInterfaceTest);

