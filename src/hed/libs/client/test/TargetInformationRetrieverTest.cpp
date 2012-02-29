#include <cppunit/extensions/HelperMacros.h>

#include <arc/UserConfig.h>
#include <arc/client/Endpoint.h>
#include <arc/client/EndpointRetriever.h>
#include <arc/client/ExecutionTarget.h>
#include <arc/client/TestACCControl.h>

//static Arc::Logger testLogger(Arc::Logger::getRootLogger(), "TargetInformationRetrieverTest");

class TargetInformationRetrieverTest
  : public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(TargetInformationRetrieverTest);
  CPPUNIT_TEST(PluginLoading);
  CPPUNIT_TEST(QueryTest);
  // This test abandons some threads, and sometimes crashes because of this:
  // CPPUNIT_TEST(GettingStatusFromUnspecifiedCE);
  CPPUNIT_TEST_SUITE_END();

public:
  TargetInformationRetrieverTest() {};

  void setUp() {}
  void tearDown() {}

  void PluginLoading();
  void QueryTest();
  void GettingStatusFromUnspecifiedCE();
};

void TargetInformationRetrieverTest::PluginLoading() {
  Arc::EndpointRetrieverPluginLoader<Arc::ComputingInfoEndpoint, Arc::ExecutionTarget> l;
  Arc::EndpointRetrieverPlugin<Arc::ComputingInfoEndpoint, Arc::ExecutionTarget>* p = l.load("TEST");
  CPPUNIT_ASSERT(p != NULL);
}


void TargetInformationRetrieverTest::QueryTest() {
  Arc::EndpointQueryingStatus sInitial(Arc::EndpointQueryingStatus::SUCCESSFUL);

  Arc::TargetInformationRetrieverPluginTESTControl::delay = 1;
  Arc::TargetInformationRetrieverPluginTESTControl::status = sInitial;

  Arc::EndpointRetrieverPluginLoader<Arc::ComputingInfoEndpoint, Arc::ExecutionTarget> l;
  Arc::EndpointRetrieverPlugin<Arc::ComputingInfoEndpoint, Arc::ExecutionTarget>* p = l.load("TEST");
  CPPUNIT_ASSERT(p != NULL);

  Arc::UserConfig uc;
  Arc::ComputingInfoEndpoint endpoint;
  std::list<Arc::ExecutionTarget> etList;
  Arc::EndpointQueryingStatus sReturned = p->Query(uc, endpoint, etList, Arc::EndpointQueryOptions<Arc::ExecutionTarget>());
  CPPUNIT_ASSERT(sReturned == Arc::EndpointQueryingStatus::SUCCESSFUL);
}

void TargetInformationRetrieverTest::GettingStatusFromUnspecifiedCE() {
  Arc::EndpointQueryingStatus sInitial(Arc::EndpointQueryingStatus::SUCCESSFUL);

  Arc::TargetInformationRetrieverPluginTESTControl::delay = 0;
  Arc::TargetInformationRetrieverPluginTESTControl::status = sInitial;

  Arc::UserConfig uc;
  Arc::TargetInformationRetriever retriever(uc, Arc::EndpointQueryOptions<Arc::ExecutionTarget>());

  Arc::ComputingInfoEndpoint ce;
  ce.URLString = "test.nordugrid.org";
  retriever.addEndpoint(ce);
  retriever.wait();
  Arc::EndpointQueryingStatus status = retriever.getStatusOfEndpoint(ce);
  CPPUNIT_ASSERT(status == Arc::EndpointQueryingStatus::SUCCESSFUL);
}

CPPUNIT_TEST_SUITE_REGISTRATION(TargetInformationRetrieverTest);
