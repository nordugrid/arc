#include <cppunit/extensions/HelperMacros.h>

#include <arc/UserConfig.h>

#include "../ServiceEndpointRetriever.h"
#include "../ServiceEndpointRetrieverPlugin.h"

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
  Arc::RegistryEndpointStatus sInitial(Arc::SER_SUCCESSFUL);

  Arc::ServiceEndpointRetrieverTESTControl::delay = 1;
  Arc::ServiceEndpointRetrieverTESTControl::status = sInitial;

  Arc::ServiceEndpointRetrieverPluginLoader l;
  Arc::ServiceEndpointRetrieverPlugin* p = l.load("TEST");
  CPPUNIT_ASSERT(p != NULL);

  Arc::UserConfig uc;
  Arc::RegistryEndpoint registry;
  std::list<Arc::ServiceEndpoint> endpoints;
  Arc::RegistryEndpointStatus sReturned = p->Query(uc, registry, endpoints);
  CPPUNIT_ASSERT(sReturned.status == Arc::SER_SUCCESSFUL);
}



CPPUNIT_TEST_SUITE_REGISTRATION(ServiceEndpointRetrieverTest);
