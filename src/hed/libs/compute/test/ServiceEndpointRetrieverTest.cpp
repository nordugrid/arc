#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cppunit/extensions/HelperMacros.h>

#include <arc/compute/Endpoint.h>
#include <arc/UserConfig.h>
#include <arc/compute/EntityRetriever.h>
#include <arc/compute/TestACCControl.h>

class ServiceEndpointRetrieverTest
  : public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(ServiceEndpointRetrieverTest);
  CPPUNIT_TEST(PluginLoading);
  CPPUNIT_TEST(QueryTest);
  CPPUNIT_TEST(BasicServiceRetrieverTest);
  CPPUNIT_TEST(SuspendedEndpointTest);
  CPPUNIT_TEST_SUITE_END();

public:
  ServiceEndpointRetrieverTest() {};

  void setUp() {}
  void tearDown() {}

  void PluginLoading();
  void QueryTest();
  void BasicServiceRetrieverTest();
  void SuspendedEndpointTest();
};

void ServiceEndpointRetrieverTest::PluginLoading() {
  Arc::ServiceEndpointRetrieverPluginLoader l;
  Arc::ServiceEndpointRetrieverPlugin* p = (Arc::ServiceEndpointRetrieverPlugin*)l.load("TEST");
  CPPUNIT_ASSERT(p != NULL);
}


void ServiceEndpointRetrieverTest::QueryTest() {
  Arc::EndpointQueryingStatus sInitial(Arc::EndpointQueryingStatus::SUCCESSFUL);

  Arc::ServiceEndpointRetrieverPluginTESTControl::status.push_back(sInitial);

  Arc::ServiceEndpointRetrieverPluginLoader l;
  Arc::ServiceEndpointRetrieverPlugin* p = (Arc::ServiceEndpointRetrieverPlugin*)l.load("TEST");
  CPPUNIT_ASSERT(p != NULL);

  Arc::UserConfig uc;
  Arc::Endpoint registry;
  std::list<Arc::Endpoint> endpoints;
  Arc::EndpointQueryingStatus sReturned = p->Query(uc, registry, endpoints, Arc::EndpointQueryOptions<Arc::Endpoint>());
  CPPUNIT_ASSERT(sReturned == Arc::EndpointQueryingStatus::SUCCESSFUL);
}

void ServiceEndpointRetrieverTest::BasicServiceRetrieverTest() {
  Arc::UserConfig uc;
  Arc::ServiceEndpointRetriever retriever(uc);

  Arc::EntityContainer<Arc::Endpoint> container;
  retriever.addConsumer(container);
  CPPUNIT_ASSERT(container.empty());

  Arc::ServiceEndpointRetrieverPluginTESTControl::endpoints.push_back(std::list<Arc::Endpoint>(1, Arc::Endpoint()));
  Arc::ServiceEndpointRetrieverPluginTESTControl::status.push_back(Arc::EndpointQueryingStatus::SUCCESSFUL);
  Arc::Endpoint registry("test.nordugrid.org", Arc::Endpoint::REGISTRY, "org.nordugrid.sertest");
  retriever.addEndpoint(registry);
  retriever.wait();

  CPPUNIT_ASSERT_EQUAL(1, (int)container.size());
}

void ServiceEndpointRetrieverTest::SuspendedEndpointTest() {
  Arc::UserConfig uc;
  Arc::ServiceEndpointRetriever retriever(uc);

  Arc::SimpleCondition c;
  Arc::ServiceEndpointRetrieverPluginTESTControl::condition.push_back(&c); // Block the first instance of the ServiceEndpointRetrieverPluginTEST::Query method
  Arc::ServiceEndpointRetrieverPluginTESTControl::status.push_back(Arc::EndpointQueryingStatus::FAILED); // First invocation should fail
  Arc::ServiceEndpointRetrieverPluginTESTControl::status.push_back(Arc::EndpointQueryingStatus::SUCCESSFUL); // Second should succeed

  Arc::Endpoint e1("test1.nordugrid.org", Arc::Endpoint::REGISTRY, "org.nordugrid.sertest");
  e1.ServiceID = "1234567890";
  Arc::Endpoint e2("test2.nordugrid.org", Arc::Endpoint::REGISTRY, "org.nordugrid.sertest");
  e2.ServiceID = "1234567890";
  
  retriever.addEndpoint(e1);
  retriever.addEndpoint(e2);
  CPPUNIT_ASSERT(Arc::EndpointQueryingStatus(Arc::EndpointQueryingStatus::STARTED) == retriever.getStatusOfEndpoint(e1));
  CPPUNIT_ASSERT(Arc::EndpointQueryingStatus(Arc::EndpointQueryingStatus::SUSPENDED_NOTREQUIRED) == retriever.getStatusOfEndpoint(e2));

  c.signal(); // Remove block on the first query invocation
  retriever.wait();
  CPPUNIT_ASSERT(Arc::EndpointQueryingStatus(Arc::EndpointQueryingStatus::FAILED) == retriever.getStatusOfEndpoint(e1));
  CPPUNIT_ASSERT(Arc::EndpointQueryingStatus(Arc::EndpointQueryingStatus::SUCCESSFUL) == retriever.getStatusOfEndpoint(e2));
}

CPPUNIT_TEST_SUITE_REGISTRATION(ServiceEndpointRetrieverTest);
