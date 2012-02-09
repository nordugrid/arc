#include <cppunit/extensions/HelperMacros.h>

#include <arc/UserConfig.h>

#include "../ServiceEndpointRetriever.h"

//static Arc::Logger testLogger(Arc::Logger::getRootLogger(), "TargetRetrieverARC1Test");

class ServiceEndpointRetrieverTest
  : public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(ServiceEndpointRetrieverTest);
  CPPUNIT_TEST(PluginLoading);
  CPPUNIT_TEST(QueryTest);
  CPPUNIT_TEST_SUITE_END();

public:
  ServiceEndpointRetrieverTest() {};

  void setUp() {}
  void tearDown() {}

  void PluginLoading();
  void QueryTest();
};

void ServiceEndpointRetrieverTest::PluginLoading() {
  Arc::ServiceEndpointRetrieverPluginLoader l;
  Arc::ServiceEndpointRetrieverPlugin* p = l.load("TEST");
  CPPUNIT_ASSERT(p != NULL);
}


void ServiceEndpointRetrieverTest::QueryTest() {
  int timeout = 5;
  Arc::ServiceEndpointStatus sInitial;
  sInitial.status = true;

  Arc::ServiceEndpointRetrieverTESTControl::tcTimeout  = &timeout;
  Arc::ServiceEndpointRetrieverTESTControl::tcStatus   = &sInitial;

  Arc::ServiceEndpointRetrieverPluginLoader l;
  Arc::ServiceEndpointRetrieverPlugin* p = l.load("TEST");
  CPPUNIT_ASSERT(p != NULL);

  Arc::UserConfig uc;
  Arc::RegistryEndpoint re;
  Arc::ServiceEndpointContainer cReturned;
  Arc::ServiceEndpointStatus sReturned = p->Query(uc, re, cReturned);
  CPPUNIT_ASSERT(sReturned.status);
}



CPPUNIT_TEST_SUITE_REGISTRATION(ServiceEndpointRetrieverTest);
