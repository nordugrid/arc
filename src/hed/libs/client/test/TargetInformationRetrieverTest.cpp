#include <cppunit/extensions/HelperMacros.h>

#include <arc/UserConfig.h>
#include <arc/client/ExecutionTarget.h>

#include <arc/client/TargetInformationRetriever.h>

//static Arc::Logger testLogger(Arc::Logger::getRootLogger(), "TargetInformationRetrieverTest");

class TargetInformationRetrieverTest
  : public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(TargetInformationRetrieverTest);
  CPPUNIT_TEST(PluginLoading);
  CPPUNIT_TEST(QueryTest);
  // This test abandons some threads, and sometimes crashes because of this:
  // CPPUNIT_TEST(GettingStatusFromUnspecifiedCE);
  // CPPUNIT_TEST(LDAPGLUE2Test);
  CPPUNIT_TEST_SUITE_END();

public:
  TargetInformationRetrieverTest() {};

  void setUp() {}
  void tearDown() {}

  void PluginLoading();
  void QueryTest();
  void GettingStatusFromUnspecifiedCE();
  void LDAPGLUE2Test();
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
  Arc::EndpointQueryingStatus sReturned = p->Query(uc, endpoint, etList, Arc::EndpointFilter<Arc::ExecutionTarget>());
  CPPUNIT_ASSERT(sReturned == Arc::EndpointQueryingStatus::SUCCESSFUL);
}

void TargetInformationRetrieverTest::GettingStatusFromUnspecifiedCE() {
  Arc::EndpointQueryingStatus sInitial(Arc::EndpointQueryingStatus::SUCCESSFUL);

  Arc::TargetInformationRetrieverPluginTESTControl::delay = 0;
  Arc::TargetInformationRetrieverPluginTESTControl::status = sInitial;

  Arc::UserConfig uc;
  Arc::TargetInformationRetriever retriever(uc);

  Arc::ComputingInfoEndpoint ce;
  ce.Endpoint = "test.nordugrid.org";
  retriever.addEndpoint(ce);
  retriever.wait();
  Arc::EndpointQueryingStatus status = retriever.getStatusOfEndpoint(ce);
  CPPUNIT_ASSERT(status == Arc::EndpointQueryingStatus::SUCCESSFUL);
}

void TargetInformationRetrieverTest::LDAPGLUE2Test() {
  Arc::LogStream logcerr(std::cerr);
  logcerr.setFormat(Arc::ShortFormat);
  Arc::Logger::getRootLogger().addDestination(logcerr);
  Arc::Logger::getRootLogger().setThreshold(Arc::DEBUG);  
  
  Arc::UserConfig uc;

  Arc::TargetInformationRetrieverPluginLoader l;
  Arc::TargetInformationRetrieverPlugin* p = l.load("LDAPGLUE2");
  CPPUNIT_ASSERT(p != NULL);

  Arc::ComputingInfoEndpoint ce;
  ce.Endpoint = "ldap://piff.hep.lu.se:2135/o=glue";
  ce.InterfaceName = "org.nordugrid.ldapglue2";
  std::list<Arc::ExecutionTarget> targets;
  Arc::EndpointQueryingStatus status = p->Query(uc, ce, targets, Arc::EndpointFilter<Arc::ExecutionTarget>());
  CPPUNIT_ASSERT(targets.size() > 0);
}


CPPUNIT_TEST_SUITE_REGISTRATION(TargetInformationRetrieverTest);
