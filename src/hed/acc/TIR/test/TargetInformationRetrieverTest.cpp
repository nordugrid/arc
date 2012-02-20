#include <cppunit/extensions/HelperMacros.h>

#include <arc/UserConfig.h>
#include <arc/client/ExecutionTarget.h>
#include <arc/client/EndpointQueryingStatus.h>

#include "../TargetInformationRetriever.h"
#include "../TargetInformationRetrieverPlugin.h"

//static Arc::Logger testLogger(Arc::Logger::getRootLogger(), "TargetRetrieverARC1Test");

class TargetInformationRetrieverTest
  : public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(TargetInformationRetrieverTest);
  CPPUNIT_TEST(PluginLoading);
  CPPUNIT_TEST(QueryTest);
  CPPUNIT_TEST_SUITE_END();

public:
  TargetInformationRetrieverTest() {};

  void setUp() {}
  void tearDown() {}

  void PluginLoading();
  void QueryTest();
};

void TargetInformationRetrieverTest::PluginLoading() {
  Arc::TargetInformationRetrieverPluginLoader l;
  Arc::TargetInformationRetrieverPlugin* p = l.load("TEST");
  CPPUNIT_ASSERT(p != NULL);
}


void TargetInformationRetrieverTest::QueryTest() {
  Arc::EndpointQueryingStatus sInitial(Arc::EndpointQueryingStatus::SUCCESSFUL);

  Arc::TargetInformationRetrieverPluginTESTControl::delay = 1;
  Arc::TargetInformationRetrieverPluginTESTControl::status = sInitial;

  Arc::TargetInformationRetrieverPluginLoader l;
  Arc::TargetInformationRetrieverPlugin* p = l.load("TEST");
  CPPUNIT_ASSERT(p != NULL);

  Arc::UserConfig uc;
  Arc::ComputingInfoEndpoint endpoint;
  std::list<Arc::ExecutionTarget> etList;
  Arc::EndpointQueryingStatus sReturned = p->Query(uc, endpoint, etList);
  CPPUNIT_ASSERT(sReturned == Arc::EndpointQueryingStatus::SUCCESSFUL);
}

CPPUNIT_TEST_SUITE_REGISTRATION(TargetInformationRetrieverTest);
