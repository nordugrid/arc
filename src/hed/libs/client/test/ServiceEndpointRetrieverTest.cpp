#include <cppunit/extensions/HelperMacros.h>

#include <arc/client/Endpoint.h>
#include <arc/UserConfig.h>
#include <arc/client/EndpointRetriever.h>
#include <arc/client/TestACCControl.h>

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
  Arc::EndpointRetrieverPluginLoader<Arc::RegistryEndpoint, Arc::ServiceEndpoint> l;
  Arc::EndpointRetrieverPlugin<Arc::RegistryEndpoint, Arc::ServiceEndpoint>* p = l.load("TEST");
  CPPUNIT_ASSERT(p != NULL);
}


void ServiceEndpointRetrieverTest::QueryTest() {
  Arc::EndpointQueryingStatus sInitial(Arc::EndpointQueryingStatus::SUCCESSFUL);

  Arc::ServiceEndpointRetrieverPluginTESTControl::delay = 1;
  Arc::ServiceEndpointRetrieverPluginTESTControl::status = sInitial;

  Arc::EndpointRetrieverPluginLoader<Arc::RegistryEndpoint, Arc::ServiceEndpoint> l;
  Arc::EndpointRetrieverPlugin<Arc::RegistryEndpoint, Arc::ServiceEndpoint>* p = l.load("TEST");
  CPPUNIT_ASSERT(p != NULL);

  Arc::UserConfig uc;
  Arc::RegistryEndpoint registry;
  std::list<Arc::ServiceEndpoint> endpoints;
  Arc::EndpointQueryingStatus sReturned = p->Query(uc, registry, endpoints, Arc::EndpointQueryOptions<Arc::ServiceEndpoint>());
  CPPUNIT_ASSERT(sReturned == Arc::EndpointQueryingStatus::SUCCESSFUL);
}

void ServiceEndpointRetrieverTest::BasicServiceRetrieverTest() {
  Arc::UserConfig uc;
  Arc::ServiceEndpointRetriever retriever(uc);

  Arc::EndpointContainer<Arc::ServiceEndpoint> container;
  retriever.addConsumer(container);
  CPPUNIT_ASSERT(container.empty());

  Arc::ServiceEndpointRetrieverPluginTESTControl::delay = 0;
  Arc::ServiceEndpointRetrieverPluginTESTControl::endpoints.push_back(Arc::ServiceEndpoint());
  Arc::ServiceEndpointRetrieverPluginTESTControl::status = Arc::EndpointQueryingStatus(Arc::EndpointQueryingStatus::SUCCESSFUL);
  Arc::RegistryEndpoint registry("test.nordugrid.org", "org.nordugrid.sertest");
  retriever.addEndpoint(registry);
  retriever.wait();

  CPPUNIT_ASSERT_EQUAL(1, (int)container.size());
}

CPPUNIT_TEST_SUITE_REGISTRATION(ServiceEndpointRetrieverTest);
