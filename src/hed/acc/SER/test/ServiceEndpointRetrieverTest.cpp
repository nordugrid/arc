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
  CPPUNIT_TEST(BasicServiceRetrieverTest);
  CPPUNIT_TEST_SUITE_END();

public:
  ServiceEndpointRetrieverTest() {};

  void setUp() {}
  void tearDown() {}

  void PluginLoading();
  void QueryTest();
  void BasicServiceRetrieverTest();
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

void ServiceEndpointRetrieverTest::BasicServiceRetrieverTest() {
  Arc::UserConfig uc;
  Arc::ServiceEndpointRetrieverTESTControl::delay = 0;
  Arc::ServiceEndpointRetrieverTESTControl::endpoints.push_back(Arc::ServiceEndpoint());
  Arc::ServiceEndpointRetrieverTESTControl::status = Arc::RegistryEndpointStatus(Arc::SER_SUCCESSFUL);

  Arc::ServiceEndpointContainer container;

  CPPUNIT_ASSERT(container.endpoints.empty());
  std::list<Arc::RegistryEndpoint> registries(1, Arc::RegistryEndpoint("test.nordugrid.org", "org.nordugrid.sertest"));
  Arc::ServiceEndpointRetriever retriever(uc, registries, container);
  retriever.wait();
  CPPUNIT_ASSERT_EQUAL(1, (int)container.endpoints.size());
}

CPPUNIT_TEST_SUITE_REGISTRATION(ServiceEndpointRetrieverTest);
