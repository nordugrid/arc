#include <cppunit/extensions/HelperMacros.h>

// This define is needed to have maximal values for types with fixed size
#define __STDC_LIMIT_MACROS
#include <stdlib.h>

#include <arc/client/ClientInterface.h>
#include <arc/message/MCC.h>

#include "../AREXClient.h"
#include "../../../libs/client/test/SimulatorClasses.h"

Arc::MCCConfig config;

class AREXClientTest
  : public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(AREXClientTest);
  CPPUNIT_TEST(SubmitTest);
  CPPUNIT_TEST(SubmitTestwithDelegation);
  CPPUNIT_TEST(ServiceStatTest);
  CPPUNIT_TEST_SUITE_END();

public:
  AREXClientTest() : ac(Arc::URL("test://AREXClientTest.now"), config, -1, true) { config.AddProxy("Proxy value");}

  void setUp() {}
  void tearDown() {}

  void SubmitTest();
  void SubmitTestwithDelegation();
  void ServiceStatTest();

private:
  Arc::AREXClient ac;
};


void AREXClientTest::SubmitTest()
{
  const std::string jobdesc = "";
  std::string jobid = "";

  const std::string value = "Test value";
  const std::string query = "CreateActivityResponse";

  Arc::ClientSOAPTest::response = new Arc::PayloadSOAP(Arc::NS("", "http://www.nordugrid.org/schemas/isis/2007/06"));
  Arc::ClientSOAPTest::response->NewChild(query).NewChild("ActivityIdentifier")  = value;

  Arc::ClientSOAPTest::status = Arc::STATUS_OK;

  //Running check
  CPPUNIT_ASSERT(ac.submit(jobdesc, jobid, false));

  //Response Check
  CPPUNIT_ASSERT_EQUAL((std::string)"<?xml version=\"1.0\"?>\n<ActivityIdentifier>Test value</ActivityIdentifier>\n", jobid);
  CPPUNIT_ASSERT(Arc::ClientSOAPTest::request["CreateActivity"]);
  CPPUNIT_ASSERT(Arc::ClientSOAPTest::request["CreateActivity"]["ActivityDocument"]);
  CPPUNIT_ASSERT_EQUAL(jobdesc, (std::string)Arc::ClientSOAPTest::request["CreateActivity"]["ActivityDocument"]);
}

void AREXClientTest::SubmitTestwithDelegation()
{
  const std::string jobdesc = "";
  std::string jobid = "";
  
  const std::string value = "Test value";
  const std::string query = "CreateActivityResponse";

  Arc::ClientSOAPTest::response = new Arc::PayloadSOAP(Arc::NS("", "http://www.nordugrid.org/schemas/isis/2007/06"));
  Arc::ClientSOAPTest::response->NewChild(query).NewChild("ActivityIdentifier")  = value;

  Arc::ClientSOAPTest::status = Arc::STATUS_OK;

  //Running Check
  CPPUNIT_ASSERT(ac.submit(jobdesc, jobid, true));

  //Response Check
  CPPUNIT_ASSERT_EQUAL((std::string)"<?xml version=\"1.0\"?>\n<ActivityIdentifier>Test value</ActivityIdentifier>\n", jobid);
  CPPUNIT_ASSERT(Arc::ClientSOAPTest::request["CreateActivity"]);
  CPPUNIT_ASSERT(Arc::ClientSOAPTest::request["CreateActivity"]["ActivityDocument"]);
  CPPUNIT_ASSERT_EQUAL(jobdesc, (std::string)Arc::ClientSOAPTest::request["CreateActivity"]["ActivityDocument"]);

  //Delegation part
  const std::string id = "id";
  const std::string dvalue = "delegation";
  const std::string attribute = "x509";
  Arc::XMLNode delegation = Arc::ClientSOAPTest::request["CreateActivity"]["DelegatedToken"];

  CPPUNIT_ASSERT(delegation);
  CPPUNIT_ASSERT_EQUAL(attribute, (std::string)delegation.Attribute("Format"));
  CPPUNIT_ASSERT(delegation["Id"]);
  CPPUNIT_ASSERT_EQUAL(id, (std::string)delegation["Id"]);
  CPPUNIT_ASSERT(delegation["Value"]);
  CPPUNIT_ASSERT_EQUAL(dvalue, (std::string)delegation["Value"]);
}


void AREXClientTest::ServiceStatTest() {
  const std::string value = "Test value";
  const std::string query = "QueryResourcePropertiesResponse";

  Arc::ClientSOAPTest::response = new Arc::PayloadSOAP(Arc::NS("", "http://www.nordugrid.org/schemas/isis/2007/06"));
  Arc::ClientSOAPTest::response->NewChild(query) = value;

  Arc::ClientSOAPTest::status = Arc::STATUS_OK;

  //Running Check
  Arc::XMLNode status;
  CPPUNIT_ASSERT(ac.sstat(status));

  //Response Check
  CPPUNIT_ASSERT(status);
  CPPUNIT_ASSERT_EQUAL(query, status.Name());
  CPPUNIT_ASSERT_EQUAL(value, (std::string)status);
}

CPPUNIT_TEST_SUITE_REGISTRATION(AREXClientTest);
