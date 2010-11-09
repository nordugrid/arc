#include <cppunit/extensions/HelperMacros.h>

// This define is needed to have maximal values for types with fixed size
#define __STDC_LIMIT_MACROS
#include <stdlib.h>

#include <arc/client/ClientInterface.h>
#include <arc/message/MCC.h>

#include "../AREXClient.h"
#include "AREXClientTest.h"

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

  static std::string action;
  static Arc::PayloadSOAP  request;
  static Arc::PayloadSOAP* response;
  static Arc::MCC_Status status;

private:
  Arc::AREXClient ac;
};

std::string       AREXClientTest::action;
Arc::PayloadSOAP  AREXClientTest::request  = Arc::PayloadSOAP(Arc::NS());
Arc::PayloadSOAP* AREXClientTest::response = NULL;
Arc::MCC_Status   AREXClientTest::status(Arc::GENERIC_ERROR);

Arc::MCC_Status Arc::ClientSOAPTest::process(Arc::PayloadSOAP *request, Arc::PayloadSOAP **response) {
  AREXClientTest::request = Arc::PayloadSOAP(*request);
  *response = AREXClientTest::response;
  return AREXClientTest::status;
}

Arc::MCC_Status Arc::ClientSOAPTest::process(const std::string& action, Arc::PayloadSOAP *request, Arc::PayloadSOAP **response) {
  AREXClientTest::action = action;
  AREXClientTest::request = Arc::PayloadSOAP(*request);
  *response = AREXClientTest::response;
  return AREXClientTest::status;
}


void AREXClientTest::SubmitTest()
{
  const std::string jobdesc = "";
  std::string jobid = "";

  CPPUNIT_ASSERT(!ac.submit(jobdesc, jobid, false));
  CPPUNIT_ASSERT_EQUAL((std::string)"", jobid);
  CPPUNIT_ASSERT(request["CreateActivity"]);
  CPPUNIT_ASSERT(request["CreateActivity"]["ActivityDocument"]);
  CPPUNIT_ASSERT_EQUAL(jobdesc, (std::string)request["CreateActivity"]["ActivityDocument"]);
}

void AREXClientTest::ServiceStatTest() {
  const std::string value = "Test value";
  const std::string query = "QueryResourcePropertiesResponse";

  AREXClientTest::response = new Arc::PayloadSOAP(Arc::NS("", "http://www.nordugrid.org/schemas/isis/2007/06"));
  AREXClientTest::response->NewChild(query) = value;

  //AREXClientTest::status = STATUS_UNDEFINED;
  AREXClientTest::status = Arc::STATUS_OK;

  //Running Check
  Arc::XMLNode status;
  CPPUNIT_ASSERT(ac.sstat(status));

  //Response Check
  CPPUNIT_ASSERT(status);
  CPPUNIT_ASSERT_EQUAL(query, status.Name());
  CPPUNIT_ASSERT_EQUAL(value, (std::string)status);
}

CPPUNIT_TEST_SUITE_REGISTRATION(AREXClientTest);
