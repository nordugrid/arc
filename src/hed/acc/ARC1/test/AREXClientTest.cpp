#include <cppunit/extensions/HelperMacros.h>

// This define is needed to have maximal values for types with fixed size
#define __STDC_LIMIT_MACROS
#include <stdlib.h>

#include <arc/client/ClientInterface.h>
#include <arc/message/MCC.h>

#include "../AREXClient.h"
#include "../../../libs/client/test/SimulatorClasses.h"

class AREXClientTest
  : public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(AREXClientTest);
  CPPUNIT_TEST(SubmitTest);
  CPPUNIT_TEST(ServiceStatTest);
  CPPUNIT_TEST_SUITE_END();

public:
  AREXClientTest() : ac(Arc::URL("test://AREXClientTest.now"), Arc::MCCConfig(), -1, true) {}

  void setUp() {}
  void tearDown() {}

  void SubmitTest();
  void ServiceStatTest();

private:
  Arc::AREXClient ac;
};


void AREXClientTest::SubmitTest()
{
  const std::string jobdesc = "";
  std::string jobid = "";

  CPPUNIT_ASSERT(!ac.submit(jobdesc, jobid, false));
  CPPUNIT_ASSERT_EQUAL((std::string)"", jobid);
  CPPUNIT_ASSERT(Arc::ClientSOAPTest::request["CreateActivity"]);
  CPPUNIT_ASSERT(Arc::ClientSOAPTest::request["CreateActivity"]["ActivityDocument"]);
  CPPUNIT_ASSERT_EQUAL(jobdesc, (std::string)Arc::ClientSOAPTest::request["CreateActivity"]["ActivityDocument"]);
}

void AREXClientTest::ServiceStatTest() {
  const std::string value = "Test value";
  const std::string query = "QueryResourcePropertiesResponse";

  Arc::ClientSOAPTest::response = new Arc::PayloadSOAP(Arc::NS("", "http://www.nordugrid.org/schemas/isis/2007/06"));
  Arc::ClientSOAPTest::response->NewChild(query) = value;

  //AREXClientTest::status = STATUS_UNDEFINED;
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
